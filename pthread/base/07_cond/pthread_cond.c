#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define MAX 50
/***********条件变量************/
/*条件变量与互斥锁配合使用*/
/*
 * 操作同一个共享资源(例如链表)
 * 5个生产者线程(对链表元素进行添加，头插)
 * 5个消费者线程(对链表元素进行删除，头删)
 * 
*/

struct Node
{
    int num;
    struct Node *next;
};
//创建头结点
struct Node *phead = NULL;
//创建一把互斥锁
static pthread_mutex_t mutex;
//创建一个条件变量
static pthread_cond_t cond;

void* threadProduce(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        /*
        while (仓库已满)
            pthread_cond_wait(&cond, &mutex);
        */
        //创建新结点
        struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
        newNode->num = rand() % 1000;
        //新结点指向旧节点
        newNode->next = phead;
        //头结点指向新结点
        phead = newNode;
        printf("生产者：%ld, 生产值：%d\n", pthread_self(), newNode->num);
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&cond);//唤醒消费者线程
        sleep(rand()%3);
    }
    return NULL;
}

void* threadConsume(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mutex);
        //链表为空时阻塞消费值线程
        while (phead == NULL)//条件变量
            pthread_cond_wait(&cond, &mutex);//函数内部在条件变量满足时进行解锁，待解除时会抢锁，未抢到锁的线程继续阻塞在这里
        printf("消费者：%ld, 消费值：%d\n", pthread_self(), phead->num);
        //删除头结点
        struct Node *pnext = phead->next;
        free(phead);
        phead = pnext;
        pthread_mutex_unlock(&mutex);
        /*
        pthread_cond_broadcast(&cond);//唤醒生产者线程
        */
        sleep(rand()%3);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    //初始化互斥锁
    pthread_mutex_init(&mutex, NULL);
    //初始化条件变量
    pthread_cond_init(&cond, NULL);
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
    //销毁条件变量
    pthread_cond_destroy(&cond);
    return 0;
}