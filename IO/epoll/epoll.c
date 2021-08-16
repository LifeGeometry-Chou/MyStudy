#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h>

/*********epoll函数************/
/*
 * int epoll_create(int size);//创建epoll实例，用红黑树管理待检测集合
 * size：内核2.6.8以后忽略，但必须指定一个>0的数
 * int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);//管理epoll上的fd，增删改
 * int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);//检测epoll中是否有就绪的fd
 * epoll使用的是回调机制，效率高，处理效率也不会随着检测集合的变大而下降
 * epoll中内核和用户区使用的是共享内存，省去了不必要的内存拷贝
 * epoll可以直接得到已就绪的文件描述符集合，无需再次检测
*/

//创建监听fd
int listenFd;

//退出函数
void exitMain(int sig)
{
    close(listenFd);
    printf("Exit\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        printf("Example: %s 5050\n", argv[0]);
        return -1;
    }

    //退出信号
    signal(SIGINT, exitMain);
    signal(SIGTERM, exitMain);

    int error;

    //初始化监听fd
    listenFd = socket(AF_INET, SOCK_STREAM, 0);

    //初始化服务端地址
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(atoi(argv[1]));
    //绑定服务端监听端口和地址
    error = bind(listenFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (error != 0)
    {
        perror("bind error");
        close(listenFd);
        return -1;
    }

    //开始监听
    error = listen(listenFd, 5);
    if (error != 0)
    {
        perror("listen error");
        return -1;
    }

    //创建epoll实例
    int epfd = epoll_create(1);
    if (epfd == -1)
    {
        perror("epoll_create");
        return -1;
    }

    //往epfd中添加事件
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listenFd;
    error = epoll_ctl(epfd, EPOLL_CTL_ADD, listenFd, &event);
    if (error == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    //创建事件集合
    struct epoll_event events[1024];
    int eventsSize = sizeof(events) / sizeof(events[0]);
    //开始循环检测
    while (1)
    {
        int readyNum = epoll_wait(epfd, events, eventsSize, -1);
        //printf("Ready Num:%d\n", readyNum);
        for (int i = 0; i < readyNum; i++)
        {
            int readyFd = events[i].data.fd;
            //有连接请求，监听fd就绪
            if (readyFd == listenFd)
            {
                //客户端地址信息
                struct sockaddr_in clientAddr;
                memset(&clientAddr, 0, sizeof(clientAddr));
                int clientLen = sizeof(clientAddr);
                //已有连接请求，不会阻塞
                int clientFd = accept(readyFd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientLen);
                if (clientLen == -1)
                {
                    perror("accept");
                    break;
                }
                printf("Client IP:(%s) PORT:(%d) connect\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                //将fd的属性设置为非阻塞
                /*
                int flag = fcntl(clientFd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(clientFd, F_SETFL, flag);
                */
                //将客户端fd添加到epoll
                event.events = EPOLLIN;
                event.data.fd = clientFd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &event);
            }
            else
            {
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                int recvLen = recv(readyFd, buffer, sizeof(buffer), 0);
                if (recvLen == -1)
                {
                    perror("recv");
                    break;
                }
                else if (recvLen == 0)
                {
                    printf("Disconnect\n");
                    //将fd从epoll中删除
                    epoll_ctl(epfd, EPOLL_CTL_DEL, readyFd, NULL);
                    close(readyFd);
                }
                printf("Receive:%s\n", buffer);

                //小写转大写
                for (int i = 0; i < recvLen; i++)
                    buffer[i] = toupper(buffer[i]);

                int sendLen = send(readyFd, buffer, strlen(buffer), 0);
                if (sendLen == -1)
                {
                    perror("send");
                    break;
                }
                printf("Send:%s\n", buffer);
            }
        }
    }

    close(listenFd);
    return 0;
}