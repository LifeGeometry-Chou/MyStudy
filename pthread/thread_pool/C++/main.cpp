#include "threadpool.h"
#include "threadpool.cpp"
#include <iostream>

using namespace std;

void taskMain(void *arg)
{
    int num = *(int *)arg;
    cout << "thread (" << to_string(pthread_self()) << ") is working, number:" << num << endl;
    sleep(1);
}

int main(int argc, char *argv[])
{
    //创建线程池
    ThreadPool<int> pool(3, 10);
    //添加任务
    for (int i = 0; i < 100; i++)
    {
        int *num = new int(i+100);
        pool.addTask(Task<int>(taskMain, num));
    }
    sleep(20);
    return 0;
}