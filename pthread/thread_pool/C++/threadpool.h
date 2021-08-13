#pragma once

#include "taskqueue.h"
#include <unistd.h>

//线程池类
template <class T>
class ThreadPool
{
public:
    //线程池构造函数
    ThreadPool(int threadMin = 0, int threadMax = 5);
    //线程池析构函数
    ~ThreadPool();

    /*********************************
    函数：addTask
    参数：task--任务函数
    功能：添加任务
    返回值：无
    **********************************/
    void addTask(Task<T> task);
    /*********************************
    函数：getWorkingThreadNum
    参数：无
    功能：获取正在工作的线程数
    返回值：正在工作的线程数
    **********************************/
    int getWorkingThreadNum();
    /*********************************
    函数：getAliveThreadNum
    参数：无
    功能：获取存活的线程数
    返回值：存活的线程数
    **********************************/
    int getAliveThreadNum();

private:
    /*********************************
    函数：managerMain
    参数：arg--线程池
    功能：监测线程池的任务数以及存活线程数，按一定规则添加/销毁线程
    返回值：无
    **********************************/
    static void* managerMain(void *arg);

    /*********************************
    函数：workerMain
    参数：arg--线程池
    功能：从线程池的任务队列中获取一个任务，在本线程中执行
    返回值：无
    **********************************/
    static void* workerMain(void *arg);

    /*********************************
    函数：threadExit
    参数：pool--线程池
    功能：将工作线程ID置0，并退出线程
    返回值：无
    **********************************/
    void threadExit();

private:
    /*任务队列相关*/
    //任务队列
    TaskQueue<T> *taskQueue;

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
    //线程冗余/不足时，一次释放/添加的线程数
    static const int NUMBER = 2;

    /*锁相关*/
    //互斥锁，锁定整个线程池
    pthread_mutex_t mutexPool;
    //阻塞条件变量
    pthread_cond_t consumerNotEmpty;//消费者阻塞

    //是否销毁线程池，销毁为1，不销毁为0
    bool shutdown;
};