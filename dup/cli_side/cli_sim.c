/****************** CLIENT CODE ****************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define NAMD_IP_ADDR	"172.22.69.156"
#define NAMD_IP_PORT	7891
#define ECU_SIM_SERVER_PORT	7892
#define ECU_SIM_IP_ADDR	"172.22.64.243"

int main(){
	int ecu_sim_sock, namd_sock, nBytes;
	unsigned char buffer[1024], *ipa;
	struct sockaddr_in serverAddr, namdAddr, *tAddr;
	struct sockaddr_storage namdStorage;
	socklen_t addr_size;

	/*---- Create the socket. The three arguments are: ----*/
	/* 1) Internet domain 2) Stream socket 3) Default protocol (TCP in this case) */
	ecu_sim_sock = socket(PF_INET, SOCK_DGRAM, 0);

        /*---- Configure settings of the server address struct ----*/
        /* Address family = Internet */
        serverAddr.sin_family = PF_INET;
        /* Set port number, using htons function to use proper byte order */
        serverAddr.sin_port = htons(ECU_SIM_SERVER_PORT);
        /* Set IP address to localhost */
        //serverAddr.sin_addr.s_addr = inet_addr(ECU_SIM_IP_ADDR);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        /* Set all bits of the padding field to 0 */
        memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

        /*---- Bind the address struct to the socket ----*/
        bind(ecu_sim_sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr));


	namd_sock = socket(PF_INET, SOCK_DGRAM, 0);
	/*---- Configure settings of the server address struct ----*/
	/* Address family = Internet */
	namdAddr.sin_family = PF_INET;
	/* Set port number, using htons function to use proper byte order */
	namdAddr.sin_port = htons(NAMD_IP_PORT);
	/* Set IP address to localhost */
	namdAddr.sin_addr.s_addr = inet_addr(NAMD_IP_ADDR);
	/* Set all bits of the padding field to 0 */
	memset(namdAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

	/*---- Connect the socket to the server using the address struct ----*/
	addr_size = sizeof serverAddr;

	while (1)
	{
		printf("waiting for data from namd\n");
		/*Receive message from server*/
		nBytes = recvfrom(ecu_sim_sock, buffer, 1024, 0, (struct
					sockaddr*)&namdStorage, &addr_size);
		tAddr = (struct sockaddr_in*)&namdStorage;
		ipa = (unsigned char*)&tAddr->sin_addr.s_addr;
		printf("Received data: '%s' from IP %d.%d.%d.%d\n", buffer,
				ipa[0], ipa[1], ipa[2], ipa[3]);

		if (strncmp(buffer, "WirelessDisabled",
			strlen("WirelessDisabled")))
		{
			memset(buffer, '\0', sizeof buffer);
			printf("let's block ping\n");
			strncpy(buffer, "BlockPing", strlen("BlockPing"));
			nBytes = sendto(namd_sock, buffer, strlen(buffer)+1, 0,
					(struct sockaddr*)&namdAddr, 
					sizeof namdAddr);
		}
		else
		{
			memset(buffer, '\0', sizeof buffer);
			printf("let's unblock ping\n");
			strncpy(buffer, "UnBlockPing", strlen("UnBlockPing"));
			nBytes = sendto(namd_sock, buffer, strlen(buffer)+1, 0,
					(struct sockaddr*)&namdAddr,
					sizeof namdAddr);
		}
	}

	return 0;
}
