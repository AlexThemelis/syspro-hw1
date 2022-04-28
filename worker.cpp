#include <iostream>
#include <regex>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define MAXBUFF 64
#define FIFO1 "/tmp/fifo.1"
#define FIFO2 "/tmp/fifo.2" //Could go with this implementation
// vasika o manager
// dimiourgei ena fifo gia kaue worker
// kai san onoma tou fifo
// dinei to to pid tou worker
// opote an 8es na miliseis se ena worker xereis apeu8eis poio fifo exei me ton magaer
// manager
#define PERMS 0666

using namespace std;

int main(){
    int readfd,writefd;
    char buffer[64];
    ssize_t nread;

    // if ( (writefd = open(FIFO1, O_WRONLY)) < 0){
    //     perror("client: can't open write fifo \n");
    // }

    // if ( (readfd = open(FIFO2, O_RDONLY)) < 0){
    //     perror("client: can't open read fifo \n");
    // }


    // if ( unlink(FIFO1) < 0){
    //     perror("client: can't unlink \n");
    // }

    // if ( unlink(FIFO2) < 0){
    //     perror("client: can't unlink \n");
    // }

    int filedes;
    if ((filedes = open("url.txt", O_RDONLY))== -1){
        perror(" error in opening anotherfile \n");
        exit (1) ;
    }
    
    long total = 0;
    while((nread = read(filedes,buffer,MAXBUFF)) > 0){
        total+=nread;
        printf("total char %ld \n",total);
    }
    

    exit(0);
}

//Returns a array of url's with the correct cut of https on a given buffer
char ** url_extracter(char *buffer){
    char ** urls;
    int url_counter=0;
    string url_string[10]; //A 64 buffer cant have more than 10 url's (because http:// has 7 characters)  
    string buffer_str = buffer; //To work with regex_search

    regex regexHttp("http://([^\\s]+)"); //Explain
    smatch m;

    regex_search(buffer_str,m,regexHttp);
    for (auto x : m){
        url_string[url_counter] = x;
        url_counter++;
    }

    for(int i=0;i++;i<MAXBUFF){
        if(buffer[i] == EOF)
            return urls;
        

    }

    return urls;
}