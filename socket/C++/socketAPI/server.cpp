#include <iostream>
#include <csignal>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tcpsocket.h"

using namespace std;

void exitMain(int sig)
{
    putchar('\n');
    exit(0);
}

void* threadMain(void *arg)
{
    TcpSocket *sock = (TcpSocket *)arg;
    //打开或创建新文件
    int fileFd = open("read.txt", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    //开始通信
    while (1)
    {
        char *buffer;
        int recvLen = sock->recvMessage(&buffer);
        if (recvLen <= 0)
            break;
        
        write(fileFd, buffer, recvLen);
        delete buffer;
        buffer = nullptr;
    }
    //通信结束
    cout << "接收完成" << endl;
    close(fileFd);
    delete sock;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <port>" << endl;
        cout << "Example: " << argv[0] << " 5050" << endl;
        return -1;
    }

    //退出信号
    signal(SIGINT, exitMain);
    signal(SIGTERM, exitMain);

    //创建服务器对象
    TcpServer server;
    //开始监听
    bool ret = server.setListen(atoi(argv[1]));
    if (!ret)
        return -1;

    //阻塞等待客户端连接
    while (1)
    {
        int fd = server.acceptConnect();
        TcpSocket *sock = new TcpSocket(fd);
        //创建子线程与客户端通信
        pthread_t pid;
        pthread_create(&pid, NULL, threadMain, sock);
        pthread_detach(pid);
    }

    return 0;
}