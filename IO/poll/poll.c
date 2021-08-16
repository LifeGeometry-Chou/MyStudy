#include <sys/select.h>
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
#include <poll.h>

/*********poll函数************/
/*
 * int poll(struct pollfd *fds, nfds_t nfds, int timeout);
 * fds：委托内核检测的文件描述符数组或单个元素的地址
 * nfds：文件描述符数量，数组最大下标+1
 * timeout：超时时长，单位ms
 * 返回值：>0--成功；-1--失败；0--超时
 * 底层线性表，效率与select相同，但是不夸平台，只能在linux使用，所以使用最少
*/

//fd信息结构体
typedef struct Fdinfo
{
    struct pollfd *fds;
    int fd;
    int *maxFd;
}Fdinfo;

//创建互斥锁
pthread_mutex_t mutex;
//创建监听fd
int listenFd;

//退出函数
void exitMain(int sig)
{
    pthread_mutex_destroy(&mutex);
    printf("Exit\n");
    exit(0);
}

//建立连接线程函数
void* connectMain(void *arg)
{
    Fdinfo *info = (Fdinfo *)arg;
    //客户端地址信息
    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    int clientLen = sizeof(clientAddr);
    //已有连接请求，不会阻塞
    int clientFd = accept(info->fds[0].fd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientLen);
    if (clientLen == -1)
    {
        perror("accept error");
        pthread_exit(NULL);
    }
    printf("Client IP:(%s) PORT:(%d) connect\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    //在fds中寻找空闲的fd存入
    int i;
    for (i = 0; i < 1024; i++)
    {
        if (info->fds[i].fd == -1)
        {
            //加锁
            pthread_mutex_lock(&mutex);
            info->fds[i].fd = clientFd;
            *info->maxFd = i > *info->maxFd ? i : *info->maxFd;
            //解锁
            pthread_mutex_unlock(&mutex);
            break;
        }
    }

    free(info);
    pthread_exit(NULL);
}

//通信线程函数
void* communicateMain(void *arg)
{
    Fdinfo *info = (Fdinfo *)arg;
    char buffer[10];
    int recvLen = recv(info->fd, buffer, sizeof(buffer), 0);
    if (recvLen == -1)
    {
        printf("Receive error\n");
        free(info);
        pthread_exit(NULL);
    }
    else if (recvLen == 0)
    {
        printf("Disconnect\n");
        close(info->fd);
        for (int i = 0; i < *info->maxFd; i++)
        {
            if (info->fd == info->fds[i].fd)
            {
                //加锁
                pthread_mutex_lock(&mutex);
                info->fds[i].fd = -1;
                //解锁
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        free(info);
        pthread_exit(NULL);
    }
    printf("Receive:%s\n", buffer);

    //小写转大写
    for (int i = 0; i < recvLen; i++)
        buffer[i] = toupper(buffer[i]);

    int sendLen = send(info->fd, buffer, strlen(buffer), 0);
    if (sendLen == -1)
        perror("send error");
        
    printf("Send:%s\n", buffer);
    free(info);
    pthread_exit(NULL);
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

    //初始化互斥锁
    pthread_mutex_init(&mutex, NULL);

    int error;

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

    //创建fd集合
    struct pollfd fds[1024];
    for (int i = 0; i < 1024; i++)
    {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }
    //监听fd放到第一个元素
    fds[0].fd = listenFd;

    int maxFd = 0;

    // Fdinfo info;
    // info.fds = fds;
    // info.maxFd = &maxFd;

    while (1)
    {
        //委托内核检测
        error = poll(fds, maxFd + 1, 5);
        if (error == -1)
        {
            perror("poll error");
            break;
        }

        //检测到读缓冲区有变化
        //有新连接
        if (fds[0].revents & POLLIN)
        {
            Fdinfo *info = (Fdinfo *)malloc(sizeof(Fdinfo));
            info->fds = fds;
            info->maxFd = &maxFd;
            //创建子线程处理连接请求
            pthread_t pid;
            pthread_create(&pid, NULL, connectMain, info);
            //线程分离
            pthread_detach(pid);
        }

        //判断哪一个通信fd有数据
        for (int i = 1; i <= maxFd; i++)
        {
            //有数据
            if (fds[i].revents & POLLIN)
            {
                Fdinfo *info = (Fdinfo *)malloc(sizeof(Fdinfo));
                info->fd = fds[i].fd;
                info->fds = fds;
                info->maxFd = &maxFd;
                //创建子线程处理通信数据
                pthread_t pid;
                pthread_create(&pid, NULL, communicateMain, info);
                //线程分离
                pthread_detach(pid);
            }
        }
    }

    return 0;
}