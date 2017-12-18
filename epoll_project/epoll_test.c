#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define MAX_EVENT_NUMBER 1504
#define BUFFER_SIZE 100
#define FD_NUM_MAX 1500

int setnonblocking(int fd) {
        int old_option = fcntl(fd, F_GETFL);
        int new_option = old_option | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_option);
        return old_option;
}

void addfd(int epollfd, int fd, int enable_et) {
        epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN;
        if (enable_et) {
                event.events |= EPOLLET;
        }
        epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
        setnonblocking(fd);
}

void lt(epoll_event *events, int number, int epollfd, int listenfd) {
        char buf[BUFFER_SIZE];
        for (int i=0; i<number; i++) {
                int sockfd = events[i].data.fd;
                if (sockfd == listenfd) {
                        sockaddr_in client_address;
                        socklen_t client_addrlen = sizeof(client_address);
                        int connfd = accept(listenfd, (sockaddr*)&client_address,
                                                &client_addrlen);
                        addfd(epollfd, connfd, 0);
                }
                else if (events[i].events & EPOLLIN) {
                        printf("lt event trigger once\n");
                        memset(buf, '\0', BUFFER_SIZE);
                        int ret = recv(sockfd, buf, BUFFER_SIZE-1, 0);
                        if (ret <= 0) {
                                close(sockfd);
                                continue;
                        }
                        printf("get %d bytes of content: %s\n", ret, buf);
                }
                else {
                        printf("something else happened\n");
                }
        }
}

void et(epoll_event *events, int number, int epollfd, int listenfd) {
        char buf[BUFFER_SIZE];
        for (int i=0; i<number; i++) {
                int sockfd = events[i].data.fd;
                if (sockfd == listenfd) {
                        sockaddr_in client_address;
                        socklen_t client_addrlen = sizeof(client_address);
                        int connfd = accept(listenfd, (sockaddr*)&client_address,
                                                &client_addrlen);
                        addfd(epollfd, connfd, 1);
                }
                else if(events[i].events & EPOLLIN) {
                        // Need to read complete
                        printf("et event trigger once\n");
                        while (true) {
                                memset(buf, '\0', BUFFER_SIZE);
                                int ret = recv(sockfd, buf, BUFFER_SIZE-1, 0);
                                if (ret < 0) {
                                        // Below shows complete
                                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                                                printf("read later\n");
                                                break;
                                        }
                                        printf("some error happens\n");
                                        close(sockfd);
                                        break;
                                }
                                else if (ret == 0) {
                                        close(sockfd);
                                        break;
                                }
                                else {
                                        printf("get %d bytes of content: %s\n", ret, buf);
                                }
                        }
                }
                else {
                        printf("something else happened\n");
                }
        }
}

int main(int argc, char *argv[]) {
        if (argc <= 1) {
                printf("usage: %s port_number\n", basename(argv[0]));
                return 1;
        }

        int port = atoi(argv[1]);
        int ret = 0;
        sockaddr_in address;
        bzero(&address, sizeof(address));
        address.sin_family = AF_INET;
//        if (argc >= 3) {
//                const char *ip =argv[2];
//                inet_pton(AF_INET, ip, &address.sin_addr);
//        }
//        else {
                address.sin_addr.s_addr = INADDR_ANY;
//        }
        address.sin_port = htons(port);

        int listenfd = socket(PF_INET, SOCK_STREAM, 0);
        assert(listenfd >= 0);

        ret = bind(listenfd, (sockaddr*)&address, sizeof(address));
        assert(ret != - 1);

        ret = listen(listenfd, FD_NUM_MAX);
        assert(ret != -1);

        epoll_event events[MAX_EVENT_NUMBER];
        int epollfd = epoll_create(FD_NUM_MAX + 1);    //参数为监听的数目大小
        assert(epollfd != -1);
        addfd(epollfd, listenfd, true);

        while(true) {
/* 等待事件的产生，类似于select()调用。参数events用来从内核得到事件的集合，maxevents告之内核这个events有多大(数组成员的个数)，这个maxevents的值不能大于创建epoll_create()时的size，参数timeout是超时时间（毫秒，0会立即返回，-1将不确定，也有说法说是永久阻塞）。
    该函数返回需要处理的事件数目，如返回0表示已超时。
    返回的事件集合在events数组中，数组中实际存放的成员个数是函数的返回值。返回0表示已经超时。
*/
                ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
                if (ret < 0) {
                        printf("epoll failure\n");
                        break;
                }
                lt(events, ret, epollfd, listenfd); // lt
                //et(events, ret, epollfd, listenfd); // et
        }

        close(listenfd);
        return 0;

}