#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/************exec函数族************/
/**
 * int execve(const char *filename, char **const argv, char **const envp);
 * 功能：向子进程空间加载新程序代码
 * 参数：filename--新程序路径（可执行文件），可以是任何编译型语言所写的程序；
 *      argv--传递给新程序main函数的参数
 *      envp--传递给新程序的环境变量表
 * 返回值：成功不返回，失败返回-1，errno被设置
 * 
*/

int main(int argc, char *argv[])
{
    pid_t pid = fork();
    
    if (pid > 0)
    {
        //返回值大于0，为父进程调用的fork
        printf("father process, pid (%d), return pid (%d)\n", getpid(), pid);
        sleep(3);
    }
    else if (pid == 0)
    {
        extern char **environ;
        //返回值等于0，为子进程调用的fork
        printf("son process, pid (%d), return pid (%d)\n", getpid(), pid);
        //子进程中加载新程序
        execve("./hello", argv, environ);
    }

    return 0;
}