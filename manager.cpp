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
#define FIFO1 "/tmp/fifo.1"
#define FIFO2 "/tmp/fifo.2"
#define PERMS 0666

using namespace std;

string * url_extracter(char *buffer){
    //char ** urls;
    int url_counter=0;
    string *url_string = (string *)malloc(10 *sizeof(string));
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
        url_string[url_counter] = m[0];
        buffer_str = m.suffix();
        url_counter++;
    }

    // for(int i=0;i<MAXBUFF;i++){
    //     if(buffer[i] == EOF)
    //         return urls;
        

    // }

    return url_string;
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
        //TODO check if /usr/bin/inotifywait does the job (tell in the readme that we can find the path with "which inotifywait")
    }
    else{ // Manager //
        close(p[WRITE]);
        int rsize;
        char inbuf[MAXBUFF];
        char *filename;
        //char **urls;
        string *urls;
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
                exit (1) ;
            }

            char buffer[64];
            ssize_t nread;
            long total = 0;
            while((nread = read(filedes,buffer,MAXBUFF)) > 0){
                total+=nread;
                printf("total char %ld \n",total);
            }
                        

            urls = url_extracter(buffer);
            cout << urls[0] << endl;
            cout << urls[1] << "edw" << endl;

            free(urls);
        }
    }
}