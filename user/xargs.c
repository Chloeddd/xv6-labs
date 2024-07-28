#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
    char ch;
    char *new_argv[MAXARG];
    char buf[512];
    int n = 0;
    int new_argc = 0;

    //获取实际命令参数（去除‘xargs’）
    for (int i = 1; i < argc; i++) {
        new_argv[i - 1] = argv[i];
    }
    new_argc = argc - 1;

    //按字符读取标准化输入
    while (read(0, &ch, 1) > 0) {
        if (ch == ' ' || ch == '\n') {
            if (n > 0) {
                buf[n] = '\0';
                new_argv[new_argc] = buf;
                new_argc++;
                n = 0;//重置缓冲区索引
            }

            if (ch == '\n') {
                new_argv[new_argc] = 0;

                if (fork() == 0) {
                    exec(new_argv[0], new_argv);
                    exit(1);
                } else {
                    wait(0);
                }

                //重置索引
                new_argc = argc - 1;
            }
        } else {
            buf[n++] = ch;
        }
    }

    //最后一行可能没有换行符,单独讨论
    if (n > 0) {
        buf[n] = '\0';
        new_argv[new_argc] = buf;
        new_argc++;
        new_argv[new_argc] = 0;

        if (fork() == 0) {
            exec(new_argv[0], new_argv);
            exit(1);
        } else {
            wait(0);
        }
    }

    exit(0);
}
