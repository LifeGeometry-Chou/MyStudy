#include "tcpsocket.h"
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

//文件信息类
class FileInfo
{
public:
    FileInfo()
    {
        memset(fileName, 0, sizeof(fileName));
        fileSize = 0;
    }
    ~FileInfo() {}

    char fileName[20];
    long fileSize;
};

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "Usage: " << argv[0] << " <ip> <port> <filename>" << endl;
        cout << "Example: " << argv[0] << " 127.0.0.1 5050 test.txt" << endl;
        return -1;
    }

    //创建客户端通信对象
    TcpClient client;
    //连接服务端
    bool ret = client.connectToHost(argv[1], atoi(argv[2]));
    if (!ret)
    {
        cout << "连接失败" << endl;
        return -1;
    }

    //只读打开文件
    int fileFd = open(argv[3], O_RDONLY);
    if (fileFd == -1)
    {
        perror("open");
        return -1;
    }

    //获取文件信息
    struct stat fileInfo;
    int fileRet = fstat(fileFd, &fileInfo);
    if (fileRet == -1)
    {
        perror("fstat");
        return -1;
    }
    FileInfo info;
    strcpy(info.fileName, argv[3]);
    info.fileSize = fileInfo.st_size;
    cout << "FileName:" << info.fileName << " FileSize:" << info.fileSize << endl;

    //发送文件信息
    client.writen((char *)&info, sizeof(info));
    
    //文件读取到内存
    char fileContent[4*1024];
    memset(fileContent, 0, sizeof(fileContent));
    int readSize = 0;
    while ((readSize = read(fileFd, fileContent, sizeof(fileContent))) > 0)
    {
        //发送数据
        client.sendMessage(fileContent, readSize);
        //清空变量
        memset(fileContent, 0, sizeof(fileContent));
    }
    cout << "发送完成" << endl;

    return 0;
}