#include "tcpsocket.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>

typedef struct SocketInfos
{
    int fd;
    struct sockaddr_in addr;
}SocketInfos;

SocketInfos info[128];

int listenFd;

void exitMain(int sig)
{
    closeSocket(listenFd);
    putchar('\n');
    exit(0);
}

void* threadMain(void *arg)
{
    SocketInfos *pInfo = (SocketInfos *)arg;
    //打印客户端信息
    char ip[32];
    printf("IP:%s, PORT:%d\n", inet_ntop(AF_INET, &pInfo->addr.sin_addr.s_addr, ip, sizeof(ip)), ntohs(pInfo->addr.sin_port));
    //打开或创建新文件
    int fileFd = open("read.txt", O_CREAT | O_WRONLY, S_IRWXU);
    //通信
    while (1)
    {
        char *buffer;
        int recvLen = recvMessage(pInfo->fd, &buffer);
        if (recvLen <= 0)
            break;
        
        write(fileFd, buffer, recvLen);
        free(buffer);
        buffer = NULL;
    }
    printf("读取完成\n");
    close(fileFd);
    pInfo->fd = -1;
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

    //创建监听fd
    listenFd = createSocket();
    if (listenFd == -1)
        return -1;

    //绑定fd和ip和端口，开始监听
    int ret = setListen(listenFd, atoi(argv[1]));
    if (ret == -1)
        return -1;
    
    int max = sizeof(info) / sizeof(info[0]);
    for (int i = 0; i < max; i++)
    {
        bzero(&info[i], sizeof(info[0]));
        info[i].fd = -1;
    }

    //阻塞等待客户端连接
    while (1)
    {
        SocketInfos *pInfo;
        for (int i = 0; i < max; i++)
        {
            if (info[i].fd == -1)
            {
                pInfo = &info[i];
                break;
            }
        }
        pInfo->fd = acceptConnect(listenFd, &pInfo->addr);
        //创建子线程与客户端通信
        pthread_t pid;
        pthread_create(&pid, NULL, threadMain, pInfo);
        pthread_detach(pid);
    }

    return 0;
}