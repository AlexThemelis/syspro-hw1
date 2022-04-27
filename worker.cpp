#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define MAXBUFF 4096
#define FIFO1 "/tmp/fifo.1"
#define FIFO2 "/tmp/fifo.2"
#define PERMS 0666

int main(){
    int readfd,writefd;

    if ( (writefd = open(FIFO1, O_WRONLY)) < 0){
        perror("client: can't open write fifo \n");
    }

    if ( (readfd = open(FIFO2, O_RDONLY)) < 0){
        perror("client: can't open read fifo \n");
    }


    if ( unlink(FIFO1) < 0){
        perror("client: can't unlink \n");
    }

    if ( unlink(FIFO2) < 0){
        perror("client: can't unlink \n");
    }
    
exit(0);

}