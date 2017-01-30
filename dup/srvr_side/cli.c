#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <dbus/dbus.h>
#include <unistd.h>
#include <errno.h>

#define NAMD_IP_ADDR		"172.22.69.156"
#define NAMD_IP_PORT		7891
#define ECU_SIM_SERVER_PORT	7892
#define ECU_NW_BCAST_IP_ADDR	"172.22.79.255"

/* dbus data */
DBusError 	derr;
DBusConnection	*dcon;
DBusMessage	*dmsg;
DBusMessageIter	dargs;
unsigned char *sigval;

/* socket data */
int ecu_s_socket, ecu_c_socket, nBytes;
unsigned char buffer[1024], *ipa;
struct sockaddr_in serverAddr, ecu_sim_bcast;
struct sockaddr_storage serverStorage;
socklen_t addr_size;

void wifi_monitor_init(void)
{
	dbus_error_init(&derr);
	dcon = dbus_bus_get(DBUS_BUS_SYSTEM, &derr);
	if (dbus_error_is_set(&derr))
	{
		fprintf(stderr, "DBus connection error: %s\n", derr.message);
		dbus_error_free(&derr);
	}
	if (dcon == NULL)
	{
		exit(1);
	}

	dbus_bus_add_match(dcon, "type='signal',interface='org.freedesktop.NetworkManager'", &derr);
	dbus_connection_flush(dcon);

	if (dbus_error_is_set(&derr))
	{
		fprintf(stderr, "DBus connection error: %s\n", derr.message);
		dbus_error_free(&derr);
	}
	return;
}

int wifi_message_rcvd()
{
	DBusMessageIter	diter, dict, dict_entry, prop_val;
	unsigned char 	*property;
	int		int_val = 0;

	dbus_connection_read_write(dcon, 0);
	dmsg = dbus_connection_pop_message(dcon);

	if (dmsg == NULL)
	{
		/* continue to loop */
		return 0;
	}

	if (dbus_message_is_signal(dmsg, "org.freedesktop.NetworkManager", "PropertiesChanged"))
	{
		printf("dbus signal org.freedesktop.NetworkManager.PropertiesChanged\n");
		dbus_message_iter_init(dmsg, &diter);
		if (dbus_message_iter_get_arg_type(&diter) != DBUS_TYPE_ARRAY)
		{
			fprintf(stderr, "dbus args not an array type\n");
			return 0;
		}
		dbus_message_iter_recurse(&diter, &dict);
		do
		{
			if (dbus_message_iter_get_arg_type(&dict) != DBUS_TYPE_DICT_ENTRY)
				return 0;

			printf("%s: dictionary entry found\n", __func__);
			dbus_message_iter_recurse(&dict, &dict_entry);
			
			if (dbus_message_iter_get_arg_type(&dict_entry) != DBUS_TYPE_STRING)
				return 0;

			dbus_message_iter_get_basic(&dict_entry, &property);
			printf("%s: got the dbus arg %s\n", __func__, property);

			if (strncmp(property, "WirelessEnabled", strlen("WirelessEnabled")))
				continue;	/* iter_next till WirelessEnabled is found */

			printf("string WirelessEnabled found\n");

			if (!dbus_message_iter_next(&dict_entry))
				return 0;

			if (dbus_message_iter_get_arg_type(&dict_entry) != DBUS_TYPE_VARIANT)
				return 0;

			printf("%s: dbus arg variant found for string\n", __func__);

			dbus_message_iter_recurse(&dict_entry, &prop_val);
			if (dbus_message_iter_get_arg_type(&prop_val) != DBUS_TYPE_BOOLEAN)
				return 0;

			dbus_message_iter_get_basic(&prop_val, &int_val);
			printf("PropertiesChanged->WirelessEnabled: boolean value: %d\n", int_val);

			ssize_t sent_bytes = 0;
			memset(buffer, '\0', sizeof buffer);

			if (int_val)
			{
				strncpy(buffer, "WirelessEnabled", strlen("WirelessEnabled"));
				/* send WirelessEnabled */
				/*Send uppercase message back to client, using serverStorage as the address*/
				sent_bytes = sendto(ecu_c_socket, buffer, strlen(buffer)+1, 0, (struct sockaddr *)&ecu_sim_bcast, sizeof ecu_sim_bcast);
			}
			else
			{
				strncpy(buffer, "WirelessDisabled", strlen("WirelessDisabled"));
				/* send WirelessDisabled */
				/*Send uppercase message back to client, using serverStorage as the address*/
				sent_bytes = sendto(ecu_c_socket, buffer, strlen(buffer)+1, 0, (struct sockaddr *)&ecu_sim_bcast, sizeof ecu_sim_bcast);
			}
			if (sent_bytes <= 0)
			{
				fprintf(stderr, "failed to broadcast (%s)\n", strerror(errno));
			}
			printf("sent %d bytes to broadcast\n", (int)sent_bytes);

		} while (dbus_message_iter_next(&dict));
		dbus_message_unref(dmsg);
	}
	return 0;
}

void ecu_nam_interconnect_c_main(void)
{
	int bcast_enabled = 1;
	int ret = 0;

	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
	ecu_c_socket = socket(PF_INET, SOCK_DGRAM, 0);

	/*---- Configure settings of the server address struct ----*/
	/* Address family = Internet */
	serverAddr.sin_family = AF_INET;
	/* Set port number, using htons function to use proper byte order */
	serverAddr.sin_port = htons(NAMD_IP_PORT);
	/* Set IP address to localhost */
	serverAddr.sin_addr.s_addr = inet_addr(NAMD_IP_ADDR);
	/* Set all bits of the padding field to 0 */
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

	/*---- Bind the address struct to the socket ----*/
	bind(ecu_c_socket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	addr_size = sizeof serverStorage;

	memset(&ecu_sim_bcast, 0, sizeof ecu_sim_bcast);
	ecu_sim_bcast.sin_family = PF_INET;
	ecu_sim_bcast.sin_addr.s_addr = inet_addr(ECU_NW_BCAST_IP_ADDR);
	ecu_sim_bcast.sin_port = htons(ECU_SIM_SERVER_PORT);

	ret = setsockopt(ecu_c_socket, SOL_SOCKET, SO_BROADCAST, &bcast_enabled, sizeof bcast_enabled);
	if (ret)
	{
		fprintf(stderr, "failed to set broadcast permissions (%s) \n", strerror(errno));
	}
	wifi_monitor_init();

	while(1) {
		if (!wifi_message_rcvd())
		{
			continue;
		}
	}

	return;
}

extern void iptc_helper_add_rule(unsigned char *ipa, unsigned char *proto, unsigned char *action);

void ecu_nam_interconnect_s_main(void)
{
	int ret = 0;
	struct sockaddr_in	*tmp;
	unsigned char 		*ipa;

	printf("entered %s\n", __func__);
#if 0
	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
	ecu_s_socket = socket(PF_INET, SOCK_DGRAM, 0);

	/*---- Configure settings of the server address struct ----*/
	/* Address family = Internet */
	serverAddr.sin_family = PF_INET;
	/* Set port number, using htons function to use proper byte order */
	serverAddr.sin_port = htons(NAMD_IP_PORT);
	/* Set IP address to localhost */
	serverAddr.sin_addr.s_addr = inet_addr(NAMD_IP_ADDR);
	/* Set all bits of the padding field to 0 */
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

	/*---- Bind the address struct to the socket ----*/
	bind(ecu_s_socket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
	addr_size = sizeof serverStorage;
#endif
	while(1) {
		printf("%s:waiting for data from ECUs\n", __func__);
		ret = recvfrom(ecu_c_socket, buffer, 1024, 0, (struct sockaddr*)&serverStorage, &addr_size);
		printf("received %s:%d bytes on the %s\n", buffer, ret, __func__);

		tmp = (struct sockaddr_in*)&serverStorage;
		ipa = (unsigned char*)&(tmp->sin_addr.s_addr);
	
		if (!strncmp(buffer, "BlockPing", strlen("BlockPing")))
		{
			printf("iptc function to block ping from %d.%d.%d.%d\n", ipa[0], ipa[1], ipa[2], ipa[3]);
			iptc_helper_add_rule(ipa, "icmp", "block");
		}
		else if (!strncmp(buffer, "UnBlockPing", strlen("UnBlockPing")))
		{
			printf("iptc function to unblock ping from %d.%d.%d.%d\n", ipa[0], ipa[1], ipa[2], ipa[3]);
			iptc_helper_add_rule(ipa, "icmp", "unblock");
		}
		memset(buffer, '\0', sizeof buffer);
	}

	return;
}
