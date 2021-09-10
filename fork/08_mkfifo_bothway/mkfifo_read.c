#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define FILENAME1 "./fifo1"
#define FILENAME2 "./fifo2"

/*********mkfifo有名管道***********/
/**
 * int mkfifo(const char *pathname, mode_t mode);
 * 功能：创建一个有名管道用于进程间通信
 * 参数：pathname--管道文件路径；mode--管道不存在时
 * 返回值：0--成功；-1--失败
*/

int writeFd;
int readFd;

void exitMain(int sig)
{
    close(writeFd);
    close(readFd);
    remove(FILENAME1);
    remove(FILENAME2);
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
    //获得管道文件fd
    readFd = createFifo(FILENAME1, O_RDONLY);
    writeFd = createFifo(FILENAME2, O_WRONLY);
    if (writeFd == -1 || readFd == -1)
        return -1;

    char buffer[20];

    pid_t pid = fork();
    if (pid > 0)
    {
        //设置退出信号
        signal(SIGINT, exitMain);
        signal(SIGTERM, exitMain);
        //父进程写
        while (1)
        {
            memset(buffer, 0, sizeof(buffer));
            scanf("%s", buffer);
            write(writeFd, buffer, sizeof(buffer));
        }
    }
    else if (pid == 0)
    {
        while (1)
        {
            //子进程读
            memset(buffer, 0, sizeof(buffer));
            read(readFd, buffer, sizeof(buffer));
            printf("process 2 recv:%s\n", buffer);
        }
    }

    return 0;
}