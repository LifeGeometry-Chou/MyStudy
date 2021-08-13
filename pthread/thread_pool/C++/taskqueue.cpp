#include "taskqueue.h"

//任务类构造函数
template <class T>
Task<T>::Task()
{
    this->function = nullptr;
    this->arg = nullptr;
}

template <class T>
Task<T>::Task(callback func, void *arg)
{
    this->function = func;
    this->arg = static_cast<T*>(arg);
}

//任务队列类构造函数
template <class T>
TaskQueue<T>::TaskQueue()
{
    pthread_mutex_init(&taskMutex, NULL);
}

//任务队列类析构函数
template <class T>
TaskQueue<T>::~TaskQueue()
{
    pthread_mutex_destroy(&taskMutex);
}

//入队一个任务
template <class T>
void TaskQueue<T>::pushTask(Task<T> &task)
{
    //多线程访问共享队列资源，需要加锁
    pthread_mutex_lock(&taskMutex);
    taskQueue.push(task);
    //解锁
    pthread_mutex_unlock(&taskMutex);
}

//入队一个任务
template <class T>
void TaskQueue<T>::pushTask(callback func, void *arg)
{
    //多线程访问共享队列资源，需要加锁
    pthread_mutex_lock(&taskMutex);
    taskQueue.push(Task<T>(func, arg));
    //解锁
    pthread_mutex_unlock(&taskMutex);
}

//出队一个任务对象
template <class T>
Task<T> TaskQueue<T>::popTask()
{
    Task<T> t;
    if (!taskQueue.empty())
    {
        //多线程访问共享队列资源，需要加锁
        pthread_mutex_lock(&taskMutex);
        t = taskQueue.front();
        taskQueue.pop();
        //解锁
        pthread_mutex_unlock(&taskMutex);
        return t;
    }
    return Task<T>();
}

//获取任务队列大小
template <class T>
size_t TaskQueue<T>::getTaskQueueSize()
{
    //多线程访问共享队列资源，需要加锁
    pthread_mutex_lock(&taskMutex);
    size_t size = taskQueue.size();
    //解锁
    pthread_mutex_unlock(&taskMutex);
    return size;
}