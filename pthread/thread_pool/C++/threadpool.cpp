#include "threadpool.h"
#include "taskqueue.cpp"
#include <cstring>
#include <iostream>
#include <string>
using namespace std;

//线程池构造
template <class T>
ThreadPool<T>::ThreadPool(int threadMin, int threadMax)
{
    do {
        /*线程相关*/
        //工作线程分配内存
        this->workerThreadID = new pthread_t[threadMax];
        if (nullptr == this->workerThreadID)
        {
            cout << "new workerThreadID failed." << endl;
            break;
        }
        //初始化工作线程内存
        memset(this->workerThreadID, 0, sizeof(this->workerThreadID)*threadMax);//若workerThreadID存储了ulong，则线程在使用，为0则线程未使用

        //线程参数初始化
        this->threadMinNum = threadMin;
        this->threadMaxNum = threadMax;
        this->threadAliveNum = threadMin;//初始和最小线程数相同
        this->threadBusyNum = 0;
        this->threadExitNum = 0;

        /*锁相关*/
        //互斥锁及条件变量初始化
        if (pthread_mutex_init(&this->mutexPool, nullptr) != 0
        || pthread_cond_init(&this->consumerNotEmpty, nullptr) != 0)
        {
            cout << "init mutex or cond failed." << endl;
            break;
        }

        /*任务队列相关*/
        //任务队列初始化
        this->taskQueue = new TaskQueue<T>;
        if (nullptr == this->taskQueue)
        {
            cout << "new tackqueue failed." << endl;
            break;
        }

        //创建管理线程
        pthread_create(&this->managerThreadID, nullptr, managerMain, this);
        //创建工作线程
        for (int i = 0; i < threadMin; i++)
            pthread_create(&this->workerThreadID[i], nullptr, workerMain, this);

        this->shutdown = false;
        
        return;
    } while (0);

    //任意项内存分配失败，资源释放
    if (this->taskQueue)
        delete this->taskQueue;
    if (this->workerThreadID)
        delete[] this->workerThreadID;
}

//线程池析构
template <class T>
ThreadPool<T>::~ThreadPool()
{
    //关闭线程池
    this->shutdown = true;
    //阻塞回收管理线程
    pthread_join(this->managerThreadID, NULL);
    //唤醒阻塞的消费者线程
    for (int i = 0; i < this->threadAliveNum; i++)
        pthread_cond_signal(&this->consumerNotEmpty);

    //释放堆内存
    if (this->workerThreadID)
        delete[] this->workerThreadID;
    if (this->taskQueue)
        delete this->taskQueue;

    //销毁锁资源
    pthread_mutex_destroy(&this->mutexPool);
    pthread_cond_destroy(&this->consumerNotEmpty);
}

//添加任务
template <class T>
void ThreadPool<T>::addTask(Task<T> task)
{
    //线程唤醒后线程池有可能被关闭，需要判断
    if (this->shutdown)
    {
        pthread_mutex_unlock(&this->mutexPool);
        return;
    }

    //添加任务
    this->taskQueue->pushTask(task);
    
    //唤醒消费线程
    pthread_cond_signal(&this->consumerNotEmpty);
}

//获取正在工作线程数
template <class T>
int ThreadPool<T>::getWorkingThreadNum()
{
    //加锁
    pthread_mutex_lock(&this->mutexPool);
    int busyNum = this->threadBusyNum;
    //解锁
    pthread_mutex_unlock(&this->mutexPool);
    return busyNum;
}

//获取存活线程数
template <class T>
int ThreadPool<T>::getAliveThreadNum()
{
    //加锁
    pthread_mutex_lock(&this->mutexPool);
    int aliveNum = this->threadAliveNum;
    //解锁
    pthread_mutex_unlock(&this->mutexPool);
    return aliveNum;
}

//管理线程函数
template <class T>
void* ThreadPool<T>::managerMain(void *arg)
{
    /*管理线程要做的是不停检测任务数以及存活线程数，根据一定规则来创建/销毁线程(主要工作是创建/销毁线程)*/
    ThreadPool *pool = static_cast<ThreadPool*>(arg);
    while (!pool->shutdown)
    {
        //每隔3秒检测一次
        sleep(3);
        //操作的线程池属于共享资源，需要加锁
        pthread_mutex_lock(&pool->mutexPool);
        //取出线程池中任务数和存活线程数以及正在工作的线程数
        size_t taskQueueSize = pool->taskQueue->getTaskQueueSize();
        int threadAliveNum = pool->threadAliveNum;
        int threadBusyNum = pool->threadBusyNum;
        //解锁
        pthread_mutex_unlock(&pool->mutexPool);


        //根据一定的规则添加或销毁线程，可修改
        //任务数 > 存活线程数 && 存活线程数 < 最大线程数时，添加线程
        if (taskQueueSize > threadAliveNum && threadAliveNum < pool->threadMaxNum)
        {
            //已添加线程数
            int counter = 0;
            //从0开始到threadMaxNum遍历，查找哪一个线程ID空闲可用
            //操作了线程共享资源，需要加锁
            pthread_mutex_lock(&pool->mutexPool);
            for (int i = 0; i < pool->threadMaxNum && pool->threadAliveNum < pool->threadMaxNum && counter < NUMBER; i++)
            {
                if (pool->workerThreadID[i] == 0)
                {
                    pthread_create(&pool->workerThreadID[i], NULL, workerMain, pool);
                    counter++;
                    pool->threadAliveNum++;
                }
            }
            //解锁
            pthread_mutex_unlock(&pool->mutexPool);
        }
        //正在工作的线程数*2 < 存活线程数 && 存活线程数 > 最小线程数，销毁线程
        if (threadBusyNum*2 < threadAliveNum && threadAliveNum > pool->threadMinNum)
        {
            //加锁
            pthread_mutex_lock(&pool->mutexPool);
            pool->threadExitNum = NUMBER;
            //解锁
            pthread_mutex_unlock(&pool->mutexPool);
            //唤醒线程，让工作线程执行线程退出函数
            for (int i = 0; i < NUMBER; i++)
                pthread_cond_signal(&pool->consumerNotEmpty);
        }
    }
}

//工作线程函数
template <class T>
void* ThreadPool<T>::workerMain(void *arg)
{
    /*工作线程要做的是从线程池的任务队列中取出一个任务，在本线程中执行*/
    ThreadPool *pool = static_cast<ThreadPool *>(arg);
    while (true)
    {
        //线程池属于共享资源，需要加锁
        pthread_mutex_lock(&pool->mutexPool);
        //判断任务队列是否为空，为空阻塞工作线程
        while (0 == pool->taskQueue->getTaskQueueSize() && !pool->shutdown) 
        {
            //任务队列不为空时唤醒
            pthread_cond_wait(&pool->consumerNotEmpty, &pool->mutexPool);
            //唤醒后若要退出的线程数>0，则退出线程
            if (pool->threadExitNum > 0)
            {
                pool->threadExitNum--;
                if (pool->threadAliveNum > pool->threadMinNum)
                {
                    pool->threadAliveNum--;
                    //printf("唤醒后被管理线程退出的线程\n");
                    pthread_mutex_unlock(&pool->mutexPool);
                    pool->threadExit();
                }
            }
        }

        //线程唤醒后线程池有可能被关闭，需要判断
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            //printf("唤醒后线程池关闭的线程\n");
            pool->threadExit();
        }

        //从任务队列中取出任务
        Task<T> task = pool->taskQueue->popTask();
        //正在工作的线程数加1
        pool->threadBusyNum++;
        //解锁，让其他线程可用获取任务
        pthread_mutex_unlock(&pool->mutexPool);
        
        cout << "Thread (" << to_string(pthread_self()) << ") start working..." << endl;
        //在此线程执行任务函数
        task.function(task.arg);
        //任务参数可能在堆区分配内存
        delete task.arg;
        task.arg = nullptr;
        cout << "Thread (" << to_string(pthread_self()) << ") end working..." << endl;

        //任务结束，正在工作的线程数减1
        pthread_mutex_lock(&pool->mutexPool);
        pool->threadBusyNum--;
        pthread_mutex_unlock(&pool->mutexPool);

    }
}

template <class T>
void ThreadPool<T>::threadExit()
{
    pthread_t pid = pthread_self();
    for (int i = 0; i < this->threadMaxNum; i++)
    {
        if (this->workerThreadID[i] == pid)
        {
            this->workerThreadID[i] = 0;
            cout << "threadExit() called, (" << to_string(pid) << ") exiting..." << endl;
            break;
        }
    }
    pthread_exit(NULL);
}