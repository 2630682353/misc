#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

int32 handle_test(const int16 cmd, void *ibuf, int32 ilen, void *obuf, int32 *olen)
{
	printf("handle_test123\n");
	//memset((char*)ibuf+ilen, 0, 1);
	printf("handle_test %d : %s\n", ilen, (char*)ibuf);
	strcpy(obuf, "testcmd back");
	*olen = strlen("testcmd back");
	return 0;
}

int32 handle_test_netlink(const int16 cmd, void *ibuf, int32 ilen, void *obuf, int32 *olen)
{
	
	//memset((char*)ibuf+ilen, 0, 1);
	printf("handle_test_netlink %d : %s\n", ilen, (char*)ibuf);
	strcpy(obuf, "testcmd netlink back");
	*olen = strlen("testcmd netlink back");
	return 0;
}

void sig_hander( int a )  
{  
	msg_final(); 
	exit(0);
} 

int main()
{
	msg_init(GATEWAY_MID, 3, "server1_rcv", "server1_snd",111, 222);
	signal(SIGINT, sig_hander);
	if (msg_cmd_register(GATEWAY_UNIX_TEST1, handle_test))
		printf("register error\n");
	if (msg_cmd_register(GATEWAY_NETLINK_TEST1, handle_test_netlink))
		printf("register error\n");

	msg_dst_module_register_unix(DPI_MID, "client1_rcv");
	msg_dst_module_register_netlink(GATEWAY_KERNEL);
	msg_dst_module_cmd_add(GATEWAY_KERNEL, GATEWAY_KERNEL_NETLINK_TEST1);

	

	char test[] = "testcmd go";
	char *get = NULL;
	int rlen = 0;
	char temp[100] = {0};
	while (1) {
		sleep(5);
		/*
		if (msg_send_syn(GATEWAY_KERNEL, GATEWAY_KERNEL_NETLINK_TEST1, test, strlen(test), (void *)&get, &rlen) !=0 )
			continue;
		memset(temp, 0, 100);
		memcpy(temp, get, rlen);
		free(get);
		printf("size:%d, %s\n",rlen, temp);
		*/
		
	}
	
}