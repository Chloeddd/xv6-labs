#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *
fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    //从路径字符串的末尾向前搜索，直到遇到 '/' 字符或者到达路径的开头
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++; //指向第一个非 '/' 字符的位置

    if (strlen(p) >= DIRSIZ) //p之后的长度（当前文件名或目录名长度）大于DIRSIZ
        return p;
    memmove(buf, p, strlen(p)); //将文件名或目录名拷贝到buf中
    memset(buf + strlen(p), 0, 1); //buf末尾补充空字符，确保字符串终止
    return buf; //返回规范化后的文件名或目录名
}

void find(char *path, char *file)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type){
    case T_FILE: //如果是文件，且文件名与待查找的文件名相同，则打印该文件的路径
        if (strcmp(fmtname(path), file)==0){
            printf("%s\n", path);
        }
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("find: path too long\n");
            break;
        }

        //在当前路径后加上/
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de)){
            //跳过当前目录、父目录、空目录项
            if (strcmp(de.name, ".")==0 || strcmp(de.name, "..")==0 || de.inum == 0)
                continue;
            
            memmove(p, de.name, DIRSIZ); //将目录项的名称拷贝到buf的末尾
            p[DIRSIZ] = 0; //确保字符串终止
            find(buf, file); //递归查询
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("find: invalid format\n");
        exit(1);
    }

    find(argv[1], argv[2]);

    exit(0);
}