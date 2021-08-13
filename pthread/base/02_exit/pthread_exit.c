#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

/*********线程退出********/

void* threadmain(void *arg)
{
    for (int i = 0; i < 5; i++)
        printf("子线程id:%ld\n", pthread_self());
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t pid;
    pthread_create(&pid, NULL, threadmain, NULL);
    printf("主线程id:%ld\n", pthread_self());
    pthread_exit(NULL);//主线程先退出
    return 0;
}