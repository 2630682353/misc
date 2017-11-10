#include "message.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>




void sig_hander( int a )  
{  
	msg_final(); 
	exit(0);
} 

int main()
{
	msg_init(DPI_MID, 3, "client1_rcv", "client1_snd", 101, 102);
	signal(SIGINT, sig_hander);
	msg_dst_module_register_unix(GATEWAY_MID, "server1_rcv");
	msg_dst_module_register_netlink(GATEWAY_MID);
	msg_dst_module_cmd_add(GATEWAY_MID, GATEWAY_UNIX_TEST1);
	sleep(2);
	char test[] = "testcmd go";
	char *get = NULL;
	int rlen = 0;
	char temp[100] = {0};
	
	
	while (1) {
		sleep(1);
		if (msg_send_syn(GATEWAY_MID, GATEWAY_UNIX_TEST1, test, strlen(test), (void *)&get, &rlen) !=0 )
			continue;
		memset(temp, 0, 100);
		memcpy(temp, get, rlen);
		free(get);
		printf("size:%d, %s\n",rlen, temp);

		if (msg_send_syn(GATEWAY_MID, GATEWAY_UNIX_TEST1, test, strlen(test), (void *)&get, &rlen) !=0 )
			continue;
		memset(temp, 0, 100);
		memcpy(temp, get, rlen);
		free(get);
		printf("size:%d, %s\n",rlen, temp);
		//sleep(2);
	}
	
}

