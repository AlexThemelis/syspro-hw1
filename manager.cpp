#include <iostream>
#include <regex>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define READ 0
#define WRITE 1

#define MAXBUFF 64
#define FIFO1 "/tmp/fifo.1" // We will do get_pid() as a name , the name of the worker
#define FIFO2 "/tmp/fifo.2"
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
        buffer_str = m.suffix();
        url_counter++;
    }

    return url_counter; //Return how many urls

}

int main()
{
    int p[2];
    int readfd,writefd;
    pid_t pid;

    regex regexInotifyCreate("CREATE (.+)"); //Explain
    regex regexInotifyMove_to("MOVED_TO (.+)"); //Explain
    smatch m;

    string filenametest;
    string inbuffer;

    if (pipe(p) == -1)
    {
        perror("pipe call");
        exit(-1);
    }


    if ( (mkfifo(FIFO1, PERMS) < 0) && (errno != EEXIST) ) {
        perror("can't create fifo"); 
    } 
    if ((mkfifo(FIFO2, PERMS) < 0) && (errno != EEXIST)) {
        unlink(FIFO1);
        perror("can't create fifo");
    }

    // if ( (readfd = open(FIFO1, O_RDONLY)) < 0) {
    //     perror("server: can't open read fifo");
    // }

    // if ( (writefd = open(FIFO2, O_WRONLY)) < 0) {
    //     perror("server: can't open write fifo");
    // }

        // The file that the urls will be
        int fd;
        if ((fd = open("/tmp/urls.out", O_CREAT|O_TRUNC|O_WRONLY|O_APPEND))== -1){ //Make it delete existing file, look diale3eis for that
            perror(" error in creating\n");
            exit(1);
        }


    if ((pid = fork()) < 0)
    {
        perror("Failed to create proccess");
        exit(-4);
    }

    if (pid == 0){ // Listener //
        close(p[READ]);
        dup2(p[WRITE], 1);
        int retval = 0;
        //Explain in readme what are the parameters doing, why we did only create and moved_to
        //From the man page, only these 2 events (create and moved_to) is what we care at our exercise
        retval = execl("/usr/bin/X11/inotifywait", "inotifywait", ".", "-m", "-e", "create", "-e", "moved_to", NULL);
    }
    else{ // Manager //
        close(p[WRITE]);
        int rsize;
        char inbuf[MAXBUFF];
        char *filename;
        string urls[9]; //You cant have more than 9 urls in a 64 buffer
        while(true){
            rsize = read(p[READ], inbuf, MAXBUFF);
            inbuffer = inbuf;
            regex_search(inbuffer,m,regexInotifyCreate); //If inotifywait gave CREATE ...
            for (auto x : m)
                filenametest = x;
            if(filenametest.empty()){ // If inotifywait gave MOVED_TO and therefore filenametest is empty
                regex_search(inbuffer,m,regexInotifyMove_to); 
                for (auto x : m)
                    filenametest = x;                
            } 

            filename = &filenametest[0]; //We make string into char * in order to be able to easier handle it

            int filedes;
            if ((filedes = open(filename, O_RDONLY))== -1){
                perror(" error in opening anotherfile \n");
                exit(1);
            }


            char buffer[64];
            ssize_t nread;
            int url_counter;
            int i;

            int testbytes;
            string tests;
            char arr[64]; //Highest url, like buffer

            while((nread = read(filedes,buffer,MAXBUFF)) > 0){
                url_counter = url_extracter(buffer,urls);
                //write here in the file the urls
                for(i=0;i<url_counter;i++){
                    urls[i] = urls[i].substr(7,urls[i].length()); // Removing http://
                    urls[i].append("\n"); // Each url is on a new line
                    strcpy(arr, urls[i].c_str()); // Make the string -> char in order to work with write()
                    testbytes = write(fd,arr,strlen(arr)); // the length part
                }
            }
                        

        }
    }
}