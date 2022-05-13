#include <iostream>
#include <regex>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define MAXBUFF 4096
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

int url_extracter(char *buffer,string *url_string){
    int url_counter=0;
    string buffer_str = buffer; //To work with regex_search

    regex regexHttp("http://\\S*"); //Explain
    smatch m;

    // regex_search(buffer_str,m,regexHttp);
    // for (auto x : m){
    //     printf("test1\n");
    //     cout << x << endl;
    //     //url_string[0] = x;
    //     url_counter++;
    // }

    //This works, if you have time try like the above, else understand it with the suffix and explain it
    
    while (regex_search(buffer_str, m, regexHttp)) {
        url_string[url_counter] = m[0]; //check about strcpy etc
        //cout << "Caught a " << url_string[url_counter] << endl;
        buffer_str = m.suffix();
        url_counter++;
    }

    return url_counter; //Return how many urls

}

int main(){
    int readfd,writefd;
    char buffer[MAXBUFF];
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