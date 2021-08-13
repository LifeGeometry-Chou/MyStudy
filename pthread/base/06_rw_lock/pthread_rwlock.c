#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define MAX 50
/***********读写锁************/
/*
 * 3个线程对共享资源进行写
 * 5个线程对共享资源进行读
 *
*/

int num = 0;
//创建一把读写锁
static pthread_rwlock_t rwlock;

void* threadWrite(void *arg)
{
    for (int i = 0; i < MAX; i++)
    {
        //加锁
        pthread_rwlock_wrlock(&rwlock);
        int temp = num;
        temp++;
        num = temp;
        printf("写线程：%d, id:%ld\n", num, pthread_self());
        //解锁
        pthread_rwlock_unlock(&rwlock);
        usleep(5);
    }
    return NULL;
}

void* threadRead(void *arg)
{
    for (int i = 0; i < MAX; i++)
    {
        //加锁
        pthread_rwlock_rdlock(&rwlock);
        printf("读线程：%d, id:%ld\n", num, pthread_self());
        //解锁
        pthread_rwlock_unlock(&rwlock);
        usleep(rand()%5);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    //初始化锁
    pthread_rwlock_init(&rwlock, NULL);

    pthread_t pidWrite[3];//写线程id
    pthread_t pidRead[5];//读线程id
    
    //写线程
    for (int i = 0; i < 3; i++)
        pthread_create(&pidWrite[i], NULL, threadWrite, NULL);
    
    //读线程
    for (int i = 0; i < 5; i++)
        pthread_create(&pidRead[i], NULL, threadRead, NULL);
    
    printf("主线程id:%ld\n", pthread_self());


    //线程阻塞，资源回收
    for (int i = 0; i < 3; i++)
        pthread_join(pidWrite[i], NULL);
    for (int i = 0; i < 5; i++)
        pthread_join(pidRead[i], NULL);

    //释放锁
    pthread_rwlock_destroy(&rwlock);
    return 0;
}