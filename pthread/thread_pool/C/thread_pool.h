#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//任务结构体
typedef struct Task
{
    void (*function)(void *arg);
    void *arg;
}Task;

//线程池结构体
typedef struct ThreadPool
{
    /*任务队列相关*/
    //任务队列
    Task *taskQueue;
    //队列容量
    int taskQueueCapacity;
    //队列大小
    int taskQueueSize;
    //队头
    int taskQueueFront;
    //队尾
    int taskQueueRear;

    /*线程相关*/
    //管理线程ID
    pthread_t managerThreadID;
    //工作线程ID
    pthread_t *workerThreadID;
    //最小线程数
    int threadMinNum;
    //最大线程数
    int threadMaxNum;
    //正在工作的线程
    int threadBusyNum;
    //存活线程数
    int threadAliveNum;
    //要销毁的线程数
    int threadExitNum;

    /*锁相关*/
    //互斥锁，锁定整个线程池
    pthread_mutex_t mutexPool;
    //互斥锁，锁定变化操作最多的threadBusyNum
    pthread_mutex_t mutexBusy;
    //阻塞条件变量
    pthread_cond_t producerNotFull;//生产者阻塞
    pthread_cond_t consumerNotEmpty;//消费者阻塞

    //是否销毁线程池，销毁为1，不销毁为0
    int shutdown;
}ThreadPool;

/*********************************
函数：threadPoolCreate
参数：threadMin--最小线程数；threadMax--最大线程数；taskQueueCapacity--任务队列容量
功能：创建一个线程池
返回值：线程池地址
**********************************/
ThreadPool* threadPoolCreate(int threadMin, int threadMax, int taskQueueCapacity);

/*********************************
函数：threadPoolDestroy
参数：pool--线程池
功能：销毁线程池，释放资源
返回值：无
**********************************/
void threadPoolDestroy(ThreadPool *pool);

/*********************************
函数：threadAddTask
参数：pool--线程池；taskFunction--任务函数；arg--任务函数的参数
功能：将任务函数添加到线程池的工作队列中
返回值：无
**********************************/
void threadAddTask(ThreadPool *pool, void (*taskFunction)(void *arg), void *arg);

/*********************************
函数：getWorkingThreadNum
参数：pool--线程池
功能：返回正在工作的线程数
返回值：正在工作的线程数
**********************************/
int getWorkingThreadNum(ThreadPool *pool);

/*********************************
函数：getAliveThreadNum
参数：pool--线程池
功能：返回存活线程数
返回值：存活线程数
**********************************/
int getAliveThreadNum(ThreadPool *pool);

/*********************************
函数：managerMain
参数：arg--线程池
功能：监测线程池的任务数以及存活线程数，按一定规则添加/销毁线程
返回值：无
**********************************/
void* managerMain(void *arg);

/*********************************
函数：workerMain
参数：arg--线程池
功能：从线程池的任务队列中获取一个任务，在本线程中执行
返回值：无
**********************************/
void* workerMain(void *arg);

/*********************************
函数：threadExit
参数：pool--线程池
功能：将工作线程ID置0，并退出线程
返回值：无
**********************************/
void threadExit(ThreadPool *pool);

#endif