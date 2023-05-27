#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include<string.h>//c字符串
#include<sys/mman.h>//内存映射
#include<sys/stat.h>//文件状态信息
#include<sys/wait.h>//进程等等
#include<dirent.h>//目录操作
#include<signal.h>//信号
#include<sys/time.h>//时间，计时器
#include<sys/ipc.h>//ipc进程间通信
#include<sys/shm.h>//共享内存段
#include<sys/msg.h>//消息队列


int main(){
    
    pid_t pid;
    // OPEN FILES
    int fd;
    fd = open("test.txt" , O_RDWR | O_CREAT | O_TRUNC);
    if (fd == -1)
    {
        /* code */
    }
    //write 'hello fcntl!' to file
    char *init = "hello fcntl!";
    write(fd, init, 12);

    // DUPLICATE FD
    int copy_fd = fcntl(fd, F_DUPFD);

    pid = fork();

    if(pid < 0){
        // FAILS
        printf("error in fork");
        return 1;
    }
    
    struct flock fl;

    if(pid > 0){
        // PARENT PROCESS
        //set the lock
        int res = fcntl(copy_fd, F_SETLK, &fl);
        //append 'b'
        write(copy_fd, "b", 1);
        //unlock
        res = fcntl(copy_fd, F_SETLK, &fl);
        sleep(3);

        // printf("%s", str); the feedback should be 'hello fcntl!ba'
        
        exit(0);

    } else {
        // CHILD PROCESS
        sleep(2);
        //get the lock
        
        //append 'a'
        write(copy_fd, "a", 1);
        exit(0);
    }
    close(fd);
    return 0;
}