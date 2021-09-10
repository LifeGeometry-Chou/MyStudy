#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define FILENAME "./fifo"

/*********mkfifo有名管道***********/
/**
 * int mkfifo(const char *pathname, mode_t mode);
 * 功能：创建一个有名管道用于进程间通信
 * 参数：pathname--管道文件路径；mode--管道不存在时
 * 返回值：0--成功；-1--失败
*/

int fd;

void exitMain(int sig)
{
    close(fd);
    remove(FILENAME);
    exit(0);
}

int createFifo(const char *fileName, int openMode)
{
    //创建有名管道，管道文件存在时忽略错误
    int ret = mkfifo(fileName, 0664);
    if (ret == -1 && errno != EEXIST)
    {
        perror("mkfifo");
        return -1;
    }

    //打开管道文件
    int fd = open(fileName, openMode);
    if (fd == -1)
    {
        perror("open");
        return -1;
    }
    return fd;
}

int main(int argc, char *argv[])
{
    //设置退出信号
    signal(SIGINT, exitMain);
    signal(SIGTERM, exitMain);

    //获得管道文件fd
    fd = createFifo(FILENAME, O_WRONLY);
    if (fd == -1)
        return -1;

    char buffer[20];

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        scanf("%s", buffer);
        write(fd, buffer, sizeof(buffer));
    }

    return 0;
}