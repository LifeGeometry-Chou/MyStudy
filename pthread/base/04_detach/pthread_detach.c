#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

/***********线程分离************/

void* threadmain(void *arg)
{
     for (int i = 0; i < 5; i++)
        printf("子线程id:%ld\n", pthread_self());
    
    pthread_exit(NULL);//子线程退出后由其他进程回收资源
}

int main(int argc, char *argv[])
{
    pthread_t pid;
    pthread_create(&pid, NULL, threadmain, NULL);//参数4为子线程函数的参数
    printf("主线程id:%ld\n", pthread_self());

    pthread_detach(pid);//线程分离

    pthread_exit(NULL);//主线程先退出，不影响子线程的运行

    return 0;
}