#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#define NAMD_THREAD_TIMEOUT	10

extern void ecu_nam_interconnect_s_main(void);
extern void ecu_nam_interconnect_c_main(void);

void* ecu_nam_interconnect_s(void *arg)
{
	/* sets up the udp server to receive messages from ECUs */
	ecu_nam_interconnect_s_main();
	return NULL;
}

void* ecu_nam_interconnect_c(void *arg)
{
	/* runs the wifi monitor and broadcasts state dynamically */
	ecu_nam_interconnect_c_main();
	return NULL;
}

void* telematics_nam_interconnect_s(void *arg)
{
	while (1)
	{
		printf("%s running\n", __func__);
		sleep(NAMD_THREAD_TIMEOUT);
	}
	return NULL;
}

void* telematics_nam_interconnect_c(void *arg)
{
	while (1)
	{
		printf("%s running\n", __func__);
		sleep(NAMD_THREAD_TIMEOUT);
	}
	return NULL;
}

enum
{
	ECU_NAM_INTERCONNECT_SERVER,
	ECU_NAM_INTERCONNECT_CLIENT,
	TELEMATICS_NAM_INTERCONNECT_SERVER,
	TELEMATICS_NAM_INTERCONNECT_CLIENT,
	MAX_NAM_THREADS,
};

typedef struct nam_threads_s
{
	pthread_t id;
} nam_thread_t;


nam_thread_t nam_threads[MAX_NAM_THREADS];

int main(void)
{
	int i = 0;
	int err;

	for (i=0; i<MAX_NAM_THREADS; i++)
	{
		switch (i)
		{
			case ECU_NAM_INTERCONNECT_SERVER:
				err = pthread_create(&(nam_threads[i].id), NULL, &ecu_nam_interconnect_s, NULL);
				if (err != 0)
					printf("can't create thread :[%s]", strerror(err));
				else
					printf("%d:Thread created successfully\n",
							ECU_NAM_INTERCONNECT_SERVER);
				break;
			case ECU_NAM_INTERCONNECT_CLIENT:
				err = pthread_create(&(nam_threads[i].id), NULL, &ecu_nam_interconnect_c, NULL);
				if (err != 0)
					printf("can't create thread :[%s]", strerror(err));
				else
					printf("%d:Thread created successfully\n",
							ECU_NAM_INTERCONNECT_CLIENT);
				break;
			case TELEMATICS_NAM_INTERCONNECT_SERVER:
				err = pthread_create(&(nam_threads[i].id), NULL, telematics_nam_interconnect_s, NULL);
				if (err != 0)
					printf("can't create thread :[%s]", strerror(err));
				else
					printf("%d:Thread created successfully\n",
							TELEMATICS_NAM_INTERCONNECT_SERVER);
				break;
			case TELEMATICS_NAM_INTERCONNECT_CLIENT:
				err = pthread_create(&(nam_threads[i].id), NULL, telematics_nam_interconnect_c, NULL);
				if (err != 0)
					printf("can't create thread :[%s]", strerror(err));
				else
					printf("%d:Thread created successfully\n",
							TELEMATICS_NAM_INTERCONNECT_CLIENT);
				break;
		}

	}
	while (1);
	return 0;
}
