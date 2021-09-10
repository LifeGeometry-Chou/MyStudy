#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/************system函数************/
/**
 * int system(const char *command);
 * 功能：创建子进程，并加载新程序到进程空间运行
 * 参数：command--新程序的路径名
 * 
*/

int main(int argc, char *argv[])
{
    system("ls");
    return 0;
}