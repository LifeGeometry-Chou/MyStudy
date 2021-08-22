#include "tcpsocket.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
using namespace std;

/* *****************************************************************************
 *                    socket通信类
 * ****************************************************************************/
TcpSocket::TcpSocket() {}

TcpSocket::TcpSocket(int &fd)
{
    communicateFd = fd;
}

TcpSocket::~TcpSocket()
{
    closeSocket();
}

void TcpSocket::setCommunication(int &fd)
{
    communicateFd = fd;
}

int TcpSocket::readn(char *message, int size)
{
    //起始地址
    char *buffer = message;
    //剩余字节数
    int remain = size;
    while (remain > 0)
    {
        int len = recv(communicateFd, buffer, remain, 0);
        if (len == -1) {
            perror("recv");
            return -1;//异常
        }
        else if (len == 0)
            return (size - remain);//断开连接
        
        //接收成功
        //调整剩余字节数
        remain -= len;
        //调整起始地址
        buffer += len;
    }
    
    return size;
}

int TcpSocket::recvMessage(char **message)
{
    if (communicateFd < 0)
        return -1;

    /*对数据进行解包*/

    //先读取4字节的包头，获取数据长度
    int len = 0;
    int ret = readn((char *)&len, 4);
    if (ret == -1)
    {
        cout << "包头获取失败" << endl;
        return -1;
    }
    //将包头信息转换为主机字节序
    len = ntohl(len);
    //根据数据长度动态申请内存
    char *data = new char[len + 1];//加1为了加入'\0'
    //读取数据
    int length = readn(data, len);
    if (length != len)
    {
        //通信异常
        cout << "接收数据失败" << endl;
        close(communicateFd);
        //释放内存
        delete data;
        return -1;
    }
    //data加入'\0'
    strcat(data, "\0");
    *message = data;

    return length;
}

int TcpSocket::writen(const char *message, int size)
{
    //起始地址
    const char *buffer = message;
    //剩余字节数
    int remain = size;
    while (remain > 0)
    {
        int len = send(communicateFd, buffer, remain, 0);
        if (len == -1)
            return -1;//异常
        else if (len == 0)
            continue;//未发送出去
        
        //发送成功
        //调整剩余字节数
        remain -= len;
        //调整起始地址
        buffer += len;
    }

    return size;
}

int TcpSocket::sendMessage(char *message, int len)
{
    if (communicateFd < 0 || message == nullptr || len <= 0)
        return -1;
    
    /*对数据进行打包*/
    
    //数据发送长度=数据长度+4字节
    char *data = new char[len + 4];
    //需将数据长度转换为网络字节序放入数据包中
    int bigLen = htonl(len);
    //数据包=包头(4字节)+数据
    memcpy(data, &bigLen, 4);
    memcpy(data + 4, message, len);
    //调用writen发送固定大小数据包
    int ret = writen(data, len + 4);
    if (ret == -1)
        close(communicateFd);//发送失败
    
    //释放内存
    delete data;
    data = nullptr;
    return ret;
}

bool TcpSocket::closeSocket()
{
    if (communicateFd >= 0)
    {
        int ret;
        if ((ret = close(communicateFd)) == -1)
        {
            perror("close");
            return false;
        }
        communicateFd = -1;
    }

    return true;
}


/* *****************************************************************************
 *                    socket服务端类
 * ****************************************************************************/
TcpServer::TcpServer()
{
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd == -1)
        perror("socket");
}

TcpServer::~TcpServer()
{
    closeSocket();
}

bool TcpServer::setListen(unsigned short listenPort, int listenMax)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(listenPort);

    int ret = bind(listenFd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        return false;
    }

    ret = listen(listenFd, listenMax);
    if (ret == -1)
    {
        perror("listen");
        return false;
    }

    return true;
}

int TcpServer::acceptConnect(struct sockaddr_in *addr)
{
    //客户端fd
    int clientFd = -1;
    if (addr == nullptr)
    {
        //不需要返回地址
        clientFd = accept(listenFd, NULL, NULL);
    }
    else
    {
        //需要返回地址
        int addrLen = sizeof(struct sockaddr_in);
        clientFd = accept(listenFd, (struct sockaddr *)addr, (socklen_t *)&addrLen);
    }

    if (clientFd == -1)
    {
        perror("accept");
        return -1;
    }

    return clientFd;
}

bool TcpServer::closeSocket()
{
    if (listenFd >= 0)
    {
        int ret;
        if ((ret = close(listenFd)) == -1)
        {
            perror("close");
            return false;
        }
        listenFd = -1;
    }

    return true;
}

/* *****************************************************************************
 *                    socket客户端类
 * ****************************************************************************/
TcpClient::TcpClient()
{
    communicateFd = socket(AF_INET, SOCK_STREAM, 0);
    if (communicateFd == -1)
        perror("socket");
}

TcpClient::~TcpClient() {}

bool TcpClient::connectToHost(const char *ip, unsigned short port)
{
    if (communicateFd < 0)
        return false;

    //服务端地址
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);

    int ret = connect(communicateFd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("connect");
        return false;
    }

    return true;
}