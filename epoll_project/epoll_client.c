#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXLINE 4096
#define MAX_SOCK 300

int main(int argc, char** argv)
{
    int    sockfd, n, i = 0;
	int sock_array[MAX_SOCK];
//    char    recvline[4096], sendline[4096];
	char sendline[100] = {0};
    struct sockaddr_in    servaddr;

    if( argc != 2){
	    printf("usage: ./client <port>\n");
	    exit(0);
    }

    

	memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));
//	    if( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0){
//		    printf("inet_pton error for %s\n",argv[1]);
//		    exit(0);
//	    }
	servaddr.sin_addr.s_addr = INADDR_ANY;

	for (i = 0; i < MAX_SOCK; i++) {
		if((sock_array[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		    printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
		    exit(0);
    	}
	   
	    if(connect(sock_array[i], (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		    printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
		    exit(0);
	    }
	}

//    printf("send msg to server: \n");
//    fgets(sendline, 4096, stdin);
	int index = 0;
	while (1) {
		sprintf(sendline, "client message hahahaha%d", index);
	    if( send(sock_array[index], sendline, strlen(sendline), 0) < 0) {
		    printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
		    exit(0);
	    }
		index++;
		usleep(1000*200);
		if (index == MAX_SOCK - 1)
			index = 0;
	}
    close(sockfd);
    exit(0);
}