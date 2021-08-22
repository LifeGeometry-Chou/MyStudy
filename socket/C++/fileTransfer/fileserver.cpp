#include "tcpsocket.h"
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>
#include <pthread.h>

using namespace std;

//文件信息类
class FileInfo
{
public:
    FileInfo()
    {
        memset(fileName, 0, sizeof(fileName));
        fileSize = 0;
    }
    ~FileInfo() {}

    char fileName[20];
    long fileSize;
};

//退出函数
void exitMain(int sig)
{
    putchar('\n');
    exit(0);
}

//线程函数
void* threadMain(void *arg)
{
    TcpSocket *sock = static_cast<TcpSocket *>(arg);

    //接收文件信息
    FileInfo info;
    sock->readn((char *)&info, sizeof(info));
    cout << "FileName:" << info.fileName << " FileSize:" << info.fileSize << endl;

    //重命名文件
    char newFile[30];
    sprintf(newFile, "server_%s", info.fileName);
    long newFileSize = 0;

    //打开文件
    int fileFd = open(newFile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    //接收数据
    while (true)
    {
        char *buffer;
        int recvLen = sock->recvMessage(&buffer);
        if (recvLen <= 0)
            break;

        write(fileFd, buffer, recvLen);
        newFileSize += recvLen;
        delete buffer;
        buffer = nullptr;
    }

    //判断文件是否完整
    if (newFileSize == info.fileSize)
        cout << "接收完成" << endl;
    else
        cout << "文件接收不完整" << endl;

    //释放资源及关闭文件
    delete sock;
    close(fileFd);
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

    //设置退出信号
    signal(SIGINT, exitMain);
    signal(SIGTERM, exitMain);
    
    //创建服务端监听对象
    TcpServer server;
    //开始监听
    bool ret = server.setListen(atoi(argv[1]));
    if (!ret)
    {
        cout << "监听错误" << endl;
        return -1;
    }

    //等待连接
    while (true)
    {
        //有新客户端连接
        int client = server.acceptConnect();
        if (client == -1)
        {
            cout << "客户端连接失败" << endl;
            continue;
        }

        //创建通信对象
        TcpSocket *newClient = new TcpSocket(client);
        //创建子线程与客户端通信
        pthread_t pid;
        pthread_create(&pid, NULL, threadMain, newClient);
        //线程分离
        pthread_detach(pid);
    }
    
    return 0;
}