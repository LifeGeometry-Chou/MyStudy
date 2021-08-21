#include "tcpsocket.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        printf("Example: %s 127.0.0.1 5050\n", argv[0]);
        return -1;
    }

    //创建fd
    int fd = createSocket();
    if (fd == -1)
        return -1;

    //连接服务端
    int ret = connectToHost(fd, argv[1], atoi(argv[2]));
    if (ret == -1)
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
        sendMessage(fd, fileMessage, length);
        //清空字符串发送新内容
        memset(fileMessage, 0, sizeof(fileMessage));
    }

    sleep(5);

    //结束通信
    closeSocket(fd);

    return 0;
}