#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#define FILENAME "./queue"
#define MSG_SIZE 1024

struct MsgBuf
{
    long msgType;//存放消息编号，必须大于0
    char msgText[MSG_SIZE];//消息正文
};

/***********消息队列*************/
/**
 * int msgget(key_t key, int msgflg);
 * 功能：创建新的消息队列、或获取已存在的消息队列，返回唯一的标识符
 * 参数：key--队列唯一对应；msgflag--创建队列时的权限
 * 返回值：-1--失败；其他--消息队列标识符
 * 
 * key_t ftok(const char *pathname, int proj_id);
 * 功能：通过指定文件的路径名和一个整型数(低8位有效)，可以计算并返回一个唯一对应的key值
 * 返回值：唯一固定的key值
 * 
 * int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
 * 功能：利用消息队列标识符发送某编号的消息
 * 参数：msqid--消息队列；msgp--存放消息的缓存地址(long+char[msgsz]的结构体)；msgsz--正文大小；msgflg--0阻塞，IPC_NOWAIT不阻塞
 * 返回值：-1--失败；0--成功
 * 
 * ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
 * 功能：利用消息队列标识符接收某编号的消息
 * 参数：msqid--消息队列；msgp--存放消息的缓存地址(long+char[msgsz]的结构体)；msgsz--正文大小；msgtyp--要接收消息的编号；msgflg--0阻塞，IPC_NOWAIT不阻塞
 * 返回值：-1--失败；其他--返回消息正文字节数
 * 
 * int msgctl(int msqid, int cmd, struct msqid_ds *buf);
 * 功能：利用消息队列标识符来获取队列属性/修改队列属性/删除队列等等
 * 参数：msqid--消息队列；cmd--操作选项；buf--获取或修改队列属性
 * 返回值：-1--失败；0--成功
 * 
 */

int msgid;

void exitMain(int sig)
{
    putchar('\n');
    msgctl(msgid, IPC_RMID, NULL);
    remove(FILENAME);
    exit(0);
}

int createMessageQueue(const char *fileName, const char ch)
{
    //创建消息队列专用文件
    int fileFd = open(fileName, O_CREAT | O_RDWR, 0664);
    if (fileFd == -1)
    {
        perror("open");
        return -1;
    }

    //根据fileName和8位整型数计算获得唯一key值
    key_t key = ftok(fileName, ch);
    //根据key值创建或获取消息队列
    int msgid = msgget(key, 0664 | IPC_CREAT);
    if (msgid == -1)
    {
        perror("msgget");
        return -1;
    }

    return msgid;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <recvnum>\n", argv[0]);
        printf("Example: %s 1\n", argv[0]);
        return -1;
    }

    //创建或获取消息队列
    msgid = createMessageQueue(FILENAME, 'a');
    if (msgid == -1)
        return -1;
    
    //创建消息地址
    struct MsgBuf buf;
    long recvType = atol(argv[1]);

    pid_t pid = fork();
    if (pid > 0)
    {
        signal(SIGINT, exitMain);
        signal(SIGTERM, exitMain);
        //父进程，发送消息
        while (1)
        {
            //封装消息包
            scanf("%s", buf.msgText);
            printf("input msgType:");
            scanf("%ld", &buf.msgType);
            //发送消息包
            msgsnd(msgid, &buf, MSG_SIZE, 0);
        }
    }
    else if (pid == 0)
    {
        int ret = 0;
        //子进程，接收消息
        while (1)
        {
            memset(&buf, 0, sizeof(buf));
            ret = msgrcv(msgid, &buf, MSG_SIZE, recvType, 0);
            if (ret > 0)
                printf("%s\n", buf.msgText);
        }
    }

    return 0;
}