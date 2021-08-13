#include "thread_pool.h"

#define NUMBER 2

ThreadPool* threadPoolCreate(int threadMin, int threadMax, int taskQueueCapacity)
{
    //线程池分配内存
    ThreadPool *pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    do {
        if (NULL == pool)
        {
            printf("malloc threadpool failed.\n");
            break;
        }

        /*线程相关*/
        //工作线程分配内存
        pool->workerThreadID = (pthread_t *)malloc(sizeof(pthread_t)*threadMax);
        if (NULL == pool->workerThreadID)
        {
            printf("malloc workerThreadID failed.\n");
            break;
        }
        //初始化工作线程内存
        memset(pool->workerThreadID, 0, sizeof(pool->workerThreadID)*threadMax);//若workerThreadID存储了ulong，则线程在使用，为0则线程未使用

        //线程参数初始化
        pool->threadMinNum = threadMin;
        pool->threadMaxNum = threadMax;
        pool->threadAliveNum = threadMin;//初始和最小线程数相同
        pool->threadBusyNum = 0;
        pool->threadExitNum = 0;

        /*锁相关*/
        //互斥锁及条件变量初始化
        if (pthread_mutex_init(&pool->mutexPool, NULL) != 0
        || pthread_mutex_init(&pool->mutexBusy, NULL) != 0
        || pthread_cond_init(&pool->producerNotFull, NULL) != 0
        || pthread_cond_init(&pool->consumerNotEmpty, NULL) != 0)
        {
            printf("init mutex or cond failed.\n");
            break;
        }

        /*任务队列相关*/
        //任务队列初始化
        pool->taskQueue = (Task *)malloc(sizeof(Task)*taskQueueCapacity);
        if (NULL == pool->taskQueue)
        {
            printf("malloc tackqueue failed.\n");
            break;
        }
        pool->taskQueueCapacity = taskQueueCapacity;
        pool->taskQueueSize = 0;
        pool->taskQueueFront = 0;
        pool->taskQueueRear = 0;

        //创建管理线程
        pthread_create(&pool->managerThreadID, NULL, managerMain, pool);
        //创建工作线程
        for (int i = 0; i < threadMin; i++)
            pthread_create(&pool->workerThreadID[i], NULL, workerMain, pool);

        pool->shutdown = 0;
        return pool;
    } while (0);

    //任意项内存分配失败，资源释放
    if (pool && pool->workerThreadID)
        free(pool->workerThreadID);
    if (pool && pool->taskQueue)
        free(pool->taskQueue);
    if (pool)
        free(pool);

    return NULL;
}

void* managerMain(void *arg)
{
    /*管理线程要做的是不停检测任务数以及存活线程数，根据一定规则来创建/销毁线程(主要工作是创建/销毁线程)*/
    ThreadPool *pool = (ThreadPool *)arg;
    while (!pool->shutdown)
    {
        //每隔3秒检测一次
        sleep(3);
        //操作的线程池属于共享资源，需要加锁
        pthread_mutex_lock(&pool->mutexPool);
        //取出线程池中任务数和存活线程数
        int taskQueueSize = pool->taskQueueSize;
        int threadAliveNum = pool->threadAliveNum;
        //解锁
        pthread_mutex_unlock(&pool->mutexPool);

        //取出正在工作的线程数
        pthread_mutex_lock(&pool->mutexBusy);
        int threadBusyNum = pool->threadBusyNum;
        pthread_mutex_unlock(&pool->mutexBusy);

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
            pthread_mutex_lock(&pool->mutexPool);
            pool->threadExitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            //唤醒线程，让工作线程执行线程退出函数
            for (int i = 0; i < NUMBER; i++)
                pthread_cond_signal(&pool->consumerNotEmpty);
        }
    }
}

void* workerMain(void *arg)
{
    /*工作线程要做的是从线程池的任务队列中取出一个任务，在本线程中执行*/
    ThreadPool *pool = (ThreadPool *)arg;
    while (1)
    {
        //线程池属于共享资源，需要加锁
        pthread_mutex_lock(&pool->mutexPool);
        //判断任务队列是否为空，为空阻塞工作线程
        while (0 == pool->taskQueueSize && !pool->shutdown) 
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
                    threadExit(pool);
                }
            }
        }

        //线程唤醒后线程池有可能被关闭，需要判断
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            //printf("唤醒后线程池关闭的线程\n");
            threadExit(pool);
        }

        //从任务队列中取出任务
        Task task;
        task.function = pool->taskQueue[pool->taskQueueFront].function;
        task.arg = pool->taskQueue[pool->taskQueueFront].arg;
        //出队一个任务
        pool->taskQueueFront = (pool->taskQueueFront + 1) % pool->taskQueueCapacity;
        //更新任务队列大小
        pool->taskQueueSize--;
        //唤醒生产者
        pthread_cond_signal(&pool->producerNotFull);
        //解锁，让其他线程可用获取任务
        pthread_mutex_unlock(&pool->mutexPool);
        
        //正在工作的线程数加1
        pthread_mutex_lock(&pool->mutexBusy);
        pool->threadBusyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        //在此线程执行任务函数
        printf("Thread (%ld) start working...\n", pthread_self());
        task.function(task.arg);
        //任务参数可能在堆区分配内存
        free(task.arg);
        task.arg = NULL;
        printf("Thread (%ld) end working...\n", pthread_self());
        //正在工作的线程数减1
        pthread_mutex_lock(&pool->mutexBusy);
        pool->threadBusyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);

    }
}

void threadExit(ThreadPool *pool)
{
    pthread_t pid = pthread_self();
    for (int i = 0; i < pool->threadMaxNum; i++)
    {
        if (pool->workerThreadID[i] == pid)
        {
            pool->workerThreadID[i] = 0;
            printf("threadExit() called, (%ld) exiting...\n", pthread_self());
            break;
        }
    }
    pthread_exit(NULL);
}

void threadAddTask(ThreadPool *pool, void (*taskFunction)(void *arg), void *arg)
{
    //线程池为共享资源，需要加锁
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->taskQueueSize == pool->taskQueueCapacity && !pool->shutdown)
        pthread_cond_wait(&pool->producerNotFull, &pool->mutexPool); //阻塞生产者线程

    //线程唤醒后线程池有可能被关闭，需要判断
    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }

    //添加任务
    pool->taskQueue[pool->taskQueueRear].function = taskFunction;
    pool->taskQueue[pool->taskQueueRear].arg = arg;
    //入队一个任务
    pool->taskQueueRear = (pool->taskQueueRear + 1) % pool->taskQueueCapacity;
    //更新任务队列大小
    pool->taskQueueSize++;
    //唤醒阻塞的工作线程
    pthread_cond_signal(&pool->consumerNotEmpty);
    //解锁，让其他线程可用获取任务
    pthread_mutex_unlock(&pool->mutexPool);
}

int getWorkingThreadNum(ThreadPool *pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int threadBusyNum = pool->threadBusyNum;
    pthread_mutex_unlock(&pool->mutexBusy);
    return threadBusyNum;
}

int getAliveThreadNum(ThreadPool *pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int threadAliveNum = pool->threadAliveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return threadAliveNum;
}

void threadPoolDestroy(ThreadPool *pool)
{
    if (NULL == pool)
        return;

    //关闭线程池
    pool->shutdown = 1;
    //阻塞回收管理线程
    pthread_join(pool->managerThreadID, NULL);
    //唤醒阻塞的消费者线程
    for (int i = 0; i < pool->threadAliveNum; i++)
        pthread_cond_signal(&pool->consumerNotEmpty);

    //释放堆内存
    if (pool && pool->workerThreadID)
        free(pool->workerThreadID);
    if (pool && pool->taskQueue)
        free(pool->taskQueue);

    //销毁锁资源
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->consumerNotEmpty);
    pthread_cond_destroy(&pool->producerNotFull);

    //销毁线程池
    if (pool)
        free(pool);

    pool = NULL;
}