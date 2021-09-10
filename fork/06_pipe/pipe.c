#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*********pipe无名管道***********/
/**
 * int pipe(int pipefd[2]);
 * 功能：创建一个无名管道用于进程间通信
 * 参数：pipefd[0]--读fd；pipefd[1]--写fd
 * 返回值：0--成功；-1--失败
*/

int main(int argc, char *argv[])
{
    //创建无名管道
    int pipefd[2];//主进程写--子进程读
    int pipefd2[2];//主进程读--子进程写
    int ret = pipe(pipefd);
    if (ret == -1)
    {
        perror("pipe");
        return -1;
    }
    ret = pipe(pipefd2);
    if (ret == -1)
    {
        perror("pipe");
        return -1;
    }

    char buffer[20];
    memset(buffer, 0, sizeof(buffer));
    //创建进程
    pid_t pid = fork();
    if (pid > 0)
    {
        //父进程，向管道写
        //管道1不需要读，关闭读fd
        close(pipefd[0]);
        //管道2不需要写，关闭写fd
        close(pipefd2[1]);
        while (1)
        {
            write(pipefd[1], "hello", 5);
            memset(buffer, 0, sizeof(buffer));
            read(pipefd2[0], buffer, sizeof(buffer));
            printf("father process recv:%s\n", buffer);
            sleep(1);
        }
    }
    else if (pid == 0)
    {
        //子进程，从管道读
        //管道不需要写，关闭写fd
        close(pipefd[1]);
        //管道2不需要读，关闭读fd
        close(pipefd2[0]);
        while (1)
        {
            write(pipefd2[1], "world", 5);
            memset(buffer, 0, sizeof(buffer));
            read(pipefd[0], buffer, sizeof(buffer));
            printf("son process recv:%s\n", buffer);
        }
    }

    return 0;
}