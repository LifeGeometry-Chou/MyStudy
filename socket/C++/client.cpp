#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

#include "tcpsocket.h"

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        printf("Example: %s 127.0.0.1 5050\n", argv[0]);
        return -1;
    }

    //创建客户端对象
    TcpClient sock;
    //连接服务器
    bool ret = sock.connectToHost(argv[1], atoi(argv[2]));
    if (!ret)
        return -1;
    
    //只读打开一个文件
    int fileFd = open("test.txt", O_RDONLY);
    int length = 0;
    char fileMessage[100];
    memset(fileMessage, 0, sizeof(fileMessage));
    //与服务端通信
    while ((length = read(fileFd, fileMessage, rand() % 100 + 1)) > 0)
    {
        //发送数据
        sock.sendMessage(fileMessage, length);
        //清空字符串发送新内容
        memset(fileMessage, 0, sizeof(fileMessage));
    }
    cout << "发送完成" << endl;
    return 0;
}