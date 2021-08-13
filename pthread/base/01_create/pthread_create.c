#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

/***********线程创建***********/

void* pthread_main(void *arg)
{
    printf("子线程id:%ld\n", pthread_self());
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t pid;
    pthread_create(&pid, NULL, pthread_main, NULL);

    printf("主线程id:%ld\n", pthread_self());

    pthread_join(pid, NULL);
    return 0;
}