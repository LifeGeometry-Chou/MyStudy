#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class TcpSocket
{
public:
    /******************************************************************************
     * 函数：TcpSocket
     * 参数：无
     * 功能：无
     * 返回值：无
     *****************************************************************************/
    inline TcpSocket();

    /******************************************************************************
     * 函数：TcpSocket
     * 参数：fd
     * 功能：无
     * 返回值：无
     *****************************************************************************/
    TcpSocket(int &fd);

    /******************************************************************************
     * 函数：~TcpSocket
     * 参数：无
     * 功能：无
     * 返回值：无
     *****************************************************************************/
    ~TcpSocket();

    /******************************************************************************
     * 函数：setCommunication
     * 参数：fd--对方fd
     * 功能：设置通信fd
     * 返回值：无
     *****************************************************************************/
    void setCommunication(int &fd);

    /******************************************************************************
     * 函数：readn
     * 参数：message--发送的信息；size--信息大小
     * 功能：接收指定长度的数据
     * 返回值：-1--失败；其他--已接收的字节数
     *****************************************************************************/
    int readn(char *message, int size);

    /******************************************************************************
     * 函数：recvMessage
     * 参数：message--接收的信息
     * 功能：接收来自fd的信息，并存放在message中
     * 返回值：-1--失败；0--断开连接；其他--接收的字节数
     *****************************************************************************/
    int recvMessage(char **message);

    /******************************************************************************
     * 函数：writen
     * 参数：message--发送的信息；size--信息大小
     * 功能：发送指定长度的数据
     * 返回值：-1--失败；其他--已发送的字节数
     *****************************************************************************/
    int writen(const char *message, int size);

    /******************************************************************************
     * 函数：sendMessage
     * 参数：message--发送的信息；size--信息大小
     * 功能：向fd发送信息
     * 返回值：-1--失败；0--断开连接；其他--发送的字节数
     *****************************************************************************/
    int sendMessage(char *message, int size);

    /******************************************************************************
     * 函数：closeSocket
     * 参数：无
     * 功能：关闭fd
     * 返回值：false--失败；true--成功
     *****************************************************************************/
    bool closeSocket();

protected:
    int communicateFd;
};

class TcpServer
{
public:
    /******************************************************************************
     * 函数：TcpServer
     * 参数：无
     * 功能：构造函数初始化监听fd
     * 返回值：无
     *****************************************************************************/
    TcpServer();
    
    ~TcpServer();

    /******************************************************************************
     * 函数：setListen
     * 参数：listenPort--监听端口；listenMax--最大监听队列
     * 功能：将listenFd和监听的ip，端口等进行绑定，并开启监听
     * 返回值：false--失败；true--成功
     *****************************************************************************/
    bool setListen(unsigned short listenPort, int listenMax = 5);

    /******************************************************************************
     * 函数：acceptConnect
     * 参数：addr--返回连接地址
     * 功能：接收连接请求，并返回连接地址
     * 返回值：-1--失败；其他--客户端fd
     *****************************************************************************/
    int acceptConnect(struct sockaddr_in *addr = nullptr);

    /******************************************************************************
     * 函数：closeSocket
     * 参数：无
     * 功能：关闭fd
     * 返回值：false--失败；true--成功
     *****************************************************************************/
    bool closeSocket();

private:
    int listenFd;
};

class TcpClient : public TcpSocket
{
public:
    TcpClient();
    ~TcpClient();

    /******************************************************************************
     * 函数：connectToHost
     * 参数：ip--服务端ip；port--服务端端口
     * 功能：向服务端发起连接请求
     * 返回值：false--失败；true--成功
     *****************************************************************************/
    bool connectToHost(const char *ip, unsigned short port);
};