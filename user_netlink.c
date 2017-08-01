#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define NETLINK_TEST 31

#define NLMSG_SETECHO 0x11
#define NLMSG_GETECHO 0x12

#define MAX_PAYLOAD 101

struct sockaddr_nl src_addr, dst_addr;

struct iovec iov;
int sockfd;
struct nlmsghdr *nlh = NULL;
struct msghdr msg;

int main(int argc, char **argv)
{
	if (argc != 2 ) {
		printf("2 para\n");
		exit(-1);
	}

	sockfd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_TEST);
	bzero(&src_addr, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	src_addr.nl_groups = 0;
	bind(sockfd, (struct sockaddr*)&src_addr, sizeof(src_addr));

	bzero(&dst_addr, sizeof(dst_addr));
	dst_addr.nl_family = AF_NETLINK;
	dst_addr.nl_pid = 0;
	dst_addr.nl_groups = 0;

	//构造消息
	nlh = malloc(NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD); //保证对齐
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_type = NLMSG_GETECHO;
	strcpy(NLMSG_DATA(nlh), argv[1]);
	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = (void *)&dst_addr;
	msg.msg_namelen = sizeof(dst_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(sockfd, &msg, 0); //发送

	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	int flags = fcntl(sockfd, F_GETFL, 0); 
	fcntl(sockfd, F_SETFL, flags | ~O_NONBLOCK);
	while (1) {
		ssize_t res = recvmsg(sockfd, &msg, 0);
		printf(" Received message payload: %s  %d\n", NLMSG_DATA(nlh), res);
	}
}