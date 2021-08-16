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

/*********select函数************/
/*
 * int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);
 * nfds：委托内核检测的这三个集合中最大的文件描述符 + 1，windows下该参数无效，指定为-1即可
 * readfds：文件描述符的集合，内核只检测这个集合中文件描述符对应的读缓冲区
 * writefds：文件描述符的集合，内核只检测这个集合中文件描述符对应的写缓冲区
 * exceptfds：文件描述符的集合，内核检测集合中文件描述符是否有异常状态
 * timeout：超时时长，用来强制解除 select () 函数的阻塞的
 * 返回值：>0--成功；-1--失败；0--超时
 * 跨平台，支持windows,linux,mac
 * 委托内核帮助我们检测若干个文件描述符的状态，其实就是检测这些文件描述符对应的读写缓冲区的状态
 * 委托检测的文件描述符被遍历检测完毕之后，已就绪的这些满足条件的文件描述符会通过 select() 的参数分 3 个集合传出
*/

/*
 * void FD_CLR(int fd, fd_set *set);//将fd从set中删除 == fd在set中对应的标志位设置为0
 * int  FD_ISSET(int fd, fd_set *set);//判断fd是否在set中 == 检测fd在set中对应的标志位是0还是1
 * void FD_SET(int fd, fd_set *set);//将fd存入set == fd在set中对应的标志位设置为1
 * void FD_ZERO(fd_set *set);//将set中所有文件描述符对应的标志位设置为0
*/

//fd信息结构体
typedef struct Fdinfo
{
    int fd;
    int *maxFd;
    fd_set *readSet;
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
    int clientFd = accept(info->fd, (struct sockaddr *)&clientAddr, (socklen_t *)&clientLen);
    if (clientLen == -1)
    {
        perror("accept error");
        free(info);
        pthread_exit(NULL);
    }
    printf("Client IP:(%s) PORT:(%d) connect\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    //与客户端通信的fd放到读集合中，下一次调用select时通信才有效
    //加锁
    pthread_mutex_lock(&mutex);
    FD_SET(clientFd, info->readSet);
    *info->maxFd = clientFd > *info->maxFd ? clientFd : *info->maxFd;
    //解锁
    pthread_mutex_unlock(&mutex);
    
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
        //加锁
        pthread_mutex_lock(&mutex);
        FD_CLR(info->fd, info->readSet);
        //解锁
        pthread_mutex_unlock(&mutex);
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

    //创建读集合
    fd_set readSet;
    FD_ZERO(&readSet);
    //监听fd放到读集合中
    FD_SET(listenFd, &readSet);

    int maxFd = listenFd;

    while (1)
    {
        //加锁
        pthread_mutex_lock(&mutex);
        fd_set tempSet = readSet;
        //解锁
        pthread_mutex_unlock(&mutex);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        int ret = select(maxFd + 1, &tempSet, NULL, NULL, &timeout);
        //判断是否是监听fd
        if (FD_ISSET(listenFd, &tempSet))
        {
            //有客户端连接请求
            Fdinfo *info = (Fdinfo *)malloc(sizeof(Fdinfo));
            info->fd = listenFd;
            info->maxFd = &maxFd;
            info->readSet = &readSet;
            //创建子线程处理连接请求
            pthread_t pid;
            pthread_create(&pid, NULL, connectMain, info);
            //线程分离
            pthread_detach(pid);
        }

        //判断哪一个通信fd有数据
        for (int i = 0; i < maxFd + 1; i++)
        {
            if (i != listenFd && FD_ISSET(i, &tempSet))
            {
                //有客户端通信数据
                Fdinfo *info = (Fdinfo *)malloc(sizeof(Fdinfo));
                info->fd = i;
                info->maxFd = &maxFd;
                info->readSet = &readSet;
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