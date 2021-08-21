#include "tcpsocket.h"

int createSocket()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }
    return fd;
}

int setListen(int listenFd, unsigned short listenPort)
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
        return -1;
    }

    ret = listen(listenFd, 128);
    if (ret == -1)
    {
        perror("listen");
        return -1;
    }

    return 0;
}

int acceptConnect(int listenFd, struct sockaddr_in *addr)
{
    //客户端fd
    int clientFd = -1;
    if (addr == NULL)
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

int connectToHost(int fd, const char *ip, unsigned short port)
{
    //服务端地址
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);

    int ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("connect");
        return -1;
    }

    return 0;
}

int closeSocket(int fd)
{
    if (fd >= 0)
    {
        int ret = close(fd);
        if (ret == -1)
        {
            perror("close");
            return -1;
        }
    }

    return 0;
}

//用于读取指定长度的数据
int readn(int fd, char *message, int size)
{
    //起始地址
    char *buffer = message;
    //剩余字节数
    int remain = size;
    while (remain > 0)
    {
        int len = recv(fd, buffer, remain, 0);
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

int recvMessage(int fd, char **message)
{
    if (fd < 0)
        return -1;

    /*对数据进行解包*/

    //先读取4字节的包头，获取数据长度
    int len = 0;
    int ret = readn(fd, (char *)&len, 4);
    if (ret == -1)
    {
        printf("包头获取失败\n");
        return -1;
    }
    //将包头信息转换为主机字节序
    len = ntohl(len);
    //根据数据长度动态申请内存
    char *data = (char *)malloc(len + 1);//加1为了加入'\0'
    //读取数据
    int length = readn(fd, data, len);
    if (length != len)
    {
        //通信异常
        printf("接收数据失败\n");
        close(fd);
        //释放内存
        free(data);
        return -1;
    }
    //data加入'\0'
    strcat(data, "\0");
    //data[len] = '\0';
    *message = data;

    return length;
}

//用于发送指定长度的数据
int writen(int fd, const char *message, int size)
{
    //起始地址
    const char *buffer = message;
    //剩余字节数
    int remain = size;
    while (remain > 0)
    {
        int len = send(fd, buffer, remain, 0);
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

int sendMessage(int fd, char *message, int len)
{
    if (fd < 0 || message == NULL || len <= 0)
        return -1;
    
    /*对数据进行打包*/
    
    //数据发送长度=数据长度+4字节
    char *data = (char *)malloc(len + 4);
    //需将数据长度转换为网络字节序放入数据包中
    int bigLen = htonl(len);
    //数据包=包头(4字节)+数据
    memcpy(data, &bigLen, 4);
    memcpy(data + 4, message, len);
    //调用writen发送固定大小数据包
    int ret = writen(fd, data, len + 4);
    if (ret == -1)
        close(fd);//发送失败
    
    //释放内存
    free(data);
    data = NULL;
    return ret;
}