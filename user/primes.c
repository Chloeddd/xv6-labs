#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_NUM 35
#define MIN_NUM 2

void primes(int p[]){
    close(p[1]);
    
    //读取第一个数据打印
    int prime = 0; 
    read(p[0], &prime, 1);
    printf("prime %d\n", prime);
    
    int next = 0;
    if (read(p[0], &next, 1) == 0){ //如果通道已空，则退出
        return;
    }    
    
    //建一个新的通道
    int p_new[2];
    pipe(p_new);

    int pid = fork();
    if (pid == 0){
        close(p[0]);
        primes(p_new);
    } else{
        close(p_new[0]);

        //对旧通道中每一个数据进行该过程，判断是否写入新的通道
        do
        {
            if (next % prime){ //不整除则写入新的通道
                write(p_new[1], &next, 1);
            }
            
        } while (read(p[0], &next, 1));

        close(p[0]);
        close(p_new[1]);

        wait(0);
    }   
}


int main()
{
    int p[2];
    pipe(p);

    int pid = fork();
    if (pid == 0){
        primes(p);
    }
    else{ 
        close(p[0]);
        for (int i = MIN_NUM; i <= MAX_NUM; i++){
            write(p[1], &i, 1);
        }
        close(p[1]);
        wait(0);
    }

    exit(0);
}