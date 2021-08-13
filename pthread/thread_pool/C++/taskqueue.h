#pragma once

#include <queue>
#include <pthread.h>

using callback = void (*)(void*);//C++11特性

//任务类
template <class T>
class Task
{
public:
    //任务类构造函数
    Task<T>();
    Task<T>(callback func, void *arg);

    callback function;
    T *arg;
};

//任务队列类
template <class T>
class TaskQueue
{
public:
    //任务队列类构造函数
    TaskQueue();
    //任务队列类析构函数
    ~TaskQueue();

    /*********************************
    函数：pushTask
    参数：task--任务对象
    功能：获取任务队列大小
    返回值：任务队列大小
    **********************************/
    void pushTask(Task<T> &task);
    /*********************************
    函数：pushTask
    参数：func--任务函数；arg--任务函数参数
    功能：获取任务队列大小
    返回值：任务队列大小
    **********************************/
    void pushTask(callback func, void *arg);
    /*********************************
    函数：popTask
    参数：无
    功能：出队一个任务，并返回该任务对象
    返回值：任务对象
    **********************************/
    Task<T> popTask();
    /*********************************
    函数：getTaskQueueSize
    参数：无
    功能：获取任务队列大小
    返回值：任务队列大小
    **********************************/
    size_t getTaskQueueSize();

private:
    //任务队列
    std::queue<Task<T>> taskQueue;
    //任务队列锁
    pthread_mutex_t taskMutex;
};