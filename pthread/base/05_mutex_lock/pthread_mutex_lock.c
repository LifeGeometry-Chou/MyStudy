#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

/***********线程同步(加锁)************/

int num = 0;
//创建一把锁
static pthread_mutex_t mutex;

void* thread1(void *arg)
{
    for (int i = 0; i < 50; i++)
    {
        //加锁
        pthread_mutex_lock(&mutex);
        int temp = num;
        temp++;
        usleep(10);
        num = temp;
        printf("线程1：%d\n", num);
        //解锁
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void* thread2(void *arg)
{
    for (int i = 0; i < 50; i++)
    {
        //加锁
        pthread_mutex_lock(&mutex);
        int temp = num;
        temp++;
        num = temp;
        printf("线程2：%d\n", num);
        //解锁
        pthread_mutex_unlock(&mutex);
        usleep(10);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    //初始化锁
    pthread_mutex_init(&mutex, NULL);

    pthread_t pid1;
    pthread_t pid2;
    pthread_create(&pid1, NULL, thread1, NULL);//参数4为子线程函数的参数
    pthread_create(&pid2, NULL, thread2, NULL);
    printf("主线程id:%ld\n", pthread_self());


    //线程阻塞，资源回收
    pthread_join(pid1, NULL);
    pthread_join(pid2, NULL);

    //释放锁
    pthread_mutex_destroy(&mutex);
    return 0;
}