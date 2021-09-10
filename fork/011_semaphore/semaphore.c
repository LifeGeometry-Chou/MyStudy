#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define FILENAME "./semfile"

/**********信号量***********/
/**
 * int semget(key_t key, int nsems, int semflg);
 * 功能：根据key值创建或获取信号量集合，并返回其标识符。实现互斥时集合中只需要一个信号量，实现同步时需要多个信号量
 * 参数：key--创建或获取信号量集合的标识符；nsems--集合中信号量个数；semflg--原始权限(0664 | IPC_CREAT)
 * 返回值：-1--失败；其他--信号量标识符
 * 
 * int semctl(int semid, int semnum, int cmd, ...);
 * 功能：根据cmd对集合中的信号量进行控制
 * 参数：semid--信号量标识符；semnum--集合中信号量的编号(>=0)；cmd--指令；...--可选参数，信号量初始值
 * 返回值：-1--失败；其他--成功
 * 
 * int semop(int semid, struct sembuf *sops, size_t nsops);
 * 功能：对指定的信号量进行p/v操作，即加锁/解锁。p--信号量值-1，信号量值为0时阻塞；v--信号量值+1，不会阻塞
 * 参数：semid--信号量标识符；sops--结构体数组；nsops--指定数组元素个数
 * 返回值：
*/

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int fd = -1;
int semid = -1;

//nsems--信号量数量
int createSem(const char *fileName, const char ch, int nsems)
{
    int fileFd = open(fileName, O_CREAT | O_RDWR, 0664);
    if (fileFd == -1)
    {
        perror("open");
        return -1;
    }

    key_t key = ftok(fileName, ch);
    if (key == -1)
    {
        perror("ftok");
        return -1;
    }

    int semid = semget(key, nsems, 0664 | IPC_CREAT);
    if (semid == -1)
    {
        perror("semget");
        return -1;
    }

    return semid;
}

//semnum--信号量编号；val--初始化值
int initSem(int semid, int semnum, int val)
{
    union semun semUn;
    semUn.val = val;

    int ret = semctl(semid, semnum, SETVAL, semUn);
    if (ret == -1)
    {
        perror("semctl");
        return -1;
    }

    return ret;
}

//nsems信号量数量
void delSem(int semid, int nsems)
{
    int i;
    for (i = 0; i < nsems; i++)
    {
        if (semctl(semid, i, IPC_RMID) == -1)
            perror("semctl");
    }
}

//semnum[]--信号量编号数组；nsems--信号量数量
void pSem(int semid, int semnum[], int nsems)
{
    int i;
    struct sembuf ops[nsems];
    for (i = 0; i < nsems; i++)
    {
        ops[i].sem_num = semnum[i];
        ops[i].sem_op = -1;
        ops[i].sem_flg = SEM_UNDO;
    }

    if (semop(semid, ops, nsems) == -1)
        perror("semop");
}

//semnum[]--信号量编号数组；nsems--信号量数量
void vSem(int semid, int semnum[], int nsems)
{
    int i;
    struct sembuf ops[nsems];
    for (i = 0; i < nsems; i++)
    {
        ops[i].sem_num = semnum[i];
        ops[i].sem_op = 1;
        ops[i].sem_flg = SEM_UNDO;
    }

    if (semop(semid, ops, nsems) == -1)
        perror("semop");
}

void exitMain(int sig)
{
    close(fd);
    delSem(semid, 1);
    remove(FILENAME);
    putchar('\n');
    exit(0);
}

int main(int argc, char *argv[])
{
    fd = open("test.txt", O_CREAT | O_RDWR | O_TRUNC, 0664);
    if (fd == -1)
    {
        perror("open");
        return -1;
    }

    //创建或获取信号量
    semid = createSem(FILENAME, 'a', 1);
    //初始化信号量
    if (initSem(semid, 0, 1) == -1)
        return -1;
        
    int semnum[1] = {0};

    pid_t pid = fork();
    if (pid > 0)
    {
        //父进程
        signal(SIGINT, exitMain);
        while (1)
        {
            //操作的信号量编号
            semnum[0] = 0;
            //加锁
            pSem(semid, semnum, 1);
            write(fd, "hello", 5);
            write(fd, " world\n", 7);
            //解锁
            vSem(semid, semnum, 1);
        }
    }
    else if (pid == 0)
    {
        //子进程
        while (1)
        {
            //操作的信号量编号
            semnum[0] = 0;
            //加锁
            pSem(semid, semnum, 1);
            write(fd, "okoko", 5);
            write(fd, " byeby\n", 7);
            //解锁
            vSem(semid, semnum, 1);
        }
    }

    return 0;
}