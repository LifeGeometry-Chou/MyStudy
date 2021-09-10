#include <stdio.h>

int main(int argc, char *argv[], char **environ)
{
    int i;
    printf("new hello world\n");
    
    //打印参数
    for (i = 0; i < argc; i++)
        printf("%s ", argv[i]);

    putchar('\n');

    //打印环境变量表
    for (i = 0; environ[i] != NULL; i++)
        printf("%s ", environ[i]);

    putchar('\n');
    
    return 0;
}