#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX 50
/***********信号量************/
/*信号量与互斥锁配合使用*/
/*
 * 操作同一个共享资源(例如链表)
 * 5个生产者线程(对链表元素进行添加，头插)
 * 5个消费者线程(对链表元素进行删除，头删)
 * 
*/

//生产者信号量
sem_t semProduce;
//消费者信号量
sem_t semConsume;
//创建一把互斥锁
static pthread_mutex_t mutex;

struct Node
{
    int num;
    struct Node *next;
};
//创建头结点
struct Node *phead = NULL;

void* threadProduce(void *arg)
{
    while (1)
    {
        //生产者资源不为0时开始生产，为0时阻塞
        sem_wait(&semProduce);
        pthread_mutex_lock(&mutex);
        //创建新结点
        struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
        newNode->num = rand() % 1000;
        //新结点指向旧节点
        newNode->next = phead;
        //头结点指向新结点
        phead = newNode;
        printf("生产者：%ld, 生产值：%d\n", pthread_self(), newNode->num);
        pthread_mutex_unlock(&mutex);
        sem_post(&semConsume);//通知消费者，消费者的资源加1
        sleep(rand()%3);
    }
    return NULL;
}

void* threadConsume(void *arg)
{
    while (1)
    {
        //消费者资源不为0时消费，为0时阻塞
        sem_wait(&semConsume);
        pthread_mutex_lock(&mutex);
        printf("消费者：%ld, 消费值：%d\n", pthread_self(), phead->num);
        //删除头结点
        struct Node *pnext = phead->next;
        free(phead);
        phead = pnext;
        pthread_mutex_unlock(&mutex);
        sem_post(&semProduce);//通知生产者，生产者资源加1
        sleep(rand()%3);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    //初始化互斥锁
    pthread_mutex_init(&mutex, NULL);
    //初始化信号量，允许同时工作的线程为资源总和
    sem_init(&semProduce, 0, 5);
    sem_init(&semConsume, 0, 0);//消费者资源初始化为0
    pthread_t pidProduce[5];//生产者id
    pthread_t pidConsume[5];//消费者id
       
    //读线程
    for (int i = 0; i < 5; i++)
    {
        pthread_create(&pidProduce[i], NULL, threadProduce, NULL);
        pthread_create(&pidConsume[i], NULL, threadConsume, NULL);
    }
    
    printf("主线程id:%ld\n", pthread_self());


    //线程阻塞，资源回收
    for (int i = 0; i < 5; i++)
    {
        pthread_join(pidProduce[i], NULL);
        pthread_join(pidConsume[i], NULL);
    }

    //销毁锁
    pthread_mutex_destroy(&mutex);
    //销毁信号量
    sem_destroy(&semProduce);
    sem_destroy(&semConsume);
    return 0;
}