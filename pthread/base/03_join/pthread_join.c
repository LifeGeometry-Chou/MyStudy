#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

/***********线程阻塞************/

struct Person
{
    char name[20];
    int age;
};

struct Person person = {.name = "Jack", .age = 20,};

void* threadmain(void *arg)
{
    //子线程处理主线程的栈数据
    struct Person *p = (struct Person *)arg;
    strcpy(p->name, "Lily");
    p->age = 24;

    for (int i = 0; i < 5; i++)
        printf("子线程id:%ld\n", pthread_self());
    
    //struct Person person = {.name = "Jack", .age = 20,};//子线程退出后栈释放，局部变量内存也释放
    pthread_exit(&person);
}

int main(int argc, char *argv[])
{
    pthread_t pid;
    struct Person p1;//使用主线程的栈空间
    pthread_create(&pid, NULL, threadmain, &p1);//参数4为子线程函数的参数
    printf("主线程id:%ld\n", pthread_self());

    void *ptr;
    pthread_join(pid, &ptr);//阻塞等待子线程退出，参数2接收子线程退出时的返回值
    struct Person *p2 = (struct Person *)ptr;
    printf("name:%s, age:%d\n", p2->name, p2->age);

    printf("name:%s, age:%d\n", p1.name, p1.age);//
    return 0;
}