#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

/************wait函数************/
/**
 * pid_t wait(int *wstatus);
 * 功能：阻塞等待子进程结束
 * 参数：wstatus--子进程结束的返回状态
 * 
*/

int main(int argc, char *argv[])
{
    pid_t pid = fork();
    if (pid > 0)
    {
        //父进程
        int status = 0;
        //阻塞等待子进程结束返回状态
        wait(&status);
        printf("status: %d\n", status);
        if (WIFEXITED(status))
        {
            //子进程正常终止
            //打印正常终止返回值
            printf("exited:%d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            //异常终止
            //打印异常终止信号编号
            printf("%d\n", WTERMSIG(status));
        }
    }
    else if (pid == 0)
    {
        //子进程
        execve("./hello", argv, NULL);
    }

    return 0;
}