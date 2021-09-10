#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    pid_t pid = fork();
    
    if (pid > 0)
    {
        //父进程
        //父进程死循环
        while (1) {}
    }
    else if (pid == 0)
    {
        //子进程
        //子进程什么都没做就结束，成为僵尸进程
    }
    

    /*
    if (pid > 0)
    {
        //父进程
        //父进程什么都不做，直接结束
    }
    else if (pid == 0)
    {
        //子进程
        //子进程死循环，成为孤儿进程
        while (1) {}
    }
    */

    return 0;
}