#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>

#define FILENAME "./shmfile"

/**********共享内存***********/
/**
 * int shmget(key_t key, size_t size, int shmflg);
 * 功能：创建或使用一块新的共享内存
 * 参数：key--生成共享内存的标识符；size--指定共享内存的大小；shmflg--原始权限(0664 | IPC_CREAT)
 * 返回值：-1--失败；其他--共享内存标识符
 * 
 * void *shmat(int shmid, const void *shmaddr, int shmflg);
 * 功能：将shmid所指向的共享内存空间映射到进程空间(虚拟内存空间)，并返回映射后的起始地址
 * 参数：shmid--共享内存标识符；shmaddr--指定映射起始地址(一般为NULL)；shmflg--指定映射条件(0--可读可写；SHM_RDONLY--只读)
 * 返回值：(void *)-1--失败；其他映射地址--成功
 * 
 * int shmdt(const void *shmaddr);
 * 功能：取消建立的映射
 * 参数：shmaddr--映射的起始地址
 * 返回值：-1--失败；0--成功
 * 
 * int shmctl(int shmid, int cmd, struct shmid_ds *buf);
 * 功能：根据cmd对shmid进行相应控制
 * 参数：shmid--共享内存标识符；cmd--指令；buf--获取或修改共享内存的信息
*/

int shmid = -1;

void *shmAddr = NULL;

void exitMain(int sig)
{
    putchar('\n');
    shmdt(shmAddr);
    shmctl(shmid, IPC_RMID, NULL);
    remove(FILENAME);
    remove("fifo");
    exit(0);
}

int createFifo(const char *fileName, int openMode)
{
    //创建有名管道，管道文件存在时忽略错误
    int ret = mkfifo(fileName, 0664);
    if (ret == -1 && errno != EEXIST)
    {
        perror("mkfifo");
        return -1;
    }

    //打开管道文件
    int fd = open(fileName, openMode);
    if (fd == -1)
    {
        perror("open");
        return -1;
    }
    return fd;
}

int createShareMemory(const char *fileName, const char ch, const size_t size)
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

    int shmid = shmget(key, size, 0664 | IPC_CREAT);
    if (shmid == -1)
    {
        perror("shmget");
        return -1;
    }

    return shmid;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, exitMain);
    //从有名管道获取进程id
    int fifo = createFifo("fifo", O_RDONLY);
    if (fifo == -1)
        return -1;
    pid_t pid;
    read(fifo, &pid, sizeof(pid));
    close(fifo);
    //创建共享内存
    shmid = createShareMemory(FILENAME, 'a', 4096);
    //映射到虚拟内存
    shmAddr = shmat(shmid, NULL, 0);
    if (shmAddr == (void *)-1)
        return -1;
    char buffer[30] = "hello world";
    //操作共享内存
    while (1)
    {
        // *((int*)shmAddr) = 100;
        memcpy(shmAddr, buffer, sizeof(buffer));
        kill(pid, SIGUSR1);
        sleep(1);
    }
    return 0;
}