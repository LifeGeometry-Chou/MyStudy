#include "thread_pool.h"

void task(void *arg)
{
    int num = *(int *)arg;
    printf("thread (%ld) is working, number:%d\n", pthread_self(), num);
    sleep(1);
}

int main(int argc, char *argv[])
{
    //创建线程池
    ThreadPool *pool = threadPoolCreate(3, 10, 100);
    //添加任务
    for (int i = 0; i < 100; i++)
    {
        int *num = (int *)malloc(sizeof(int));
        *num = i + 100;
        threadAddTask(pool, task, num);
    }
    sleep(25);
    //销毁线程池
    threadPoolDestroy(pool);
    return 0;
}