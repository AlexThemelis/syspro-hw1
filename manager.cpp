#include <iostream>
#include <regex>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include <queue>

#define READ 0
#define WRITE 1

#define MAXBUFF 4096
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


int main(int argc, char *argv[])
{
    char *InotifyPath = (char *) "."; // Casting (char *) is because of compiler warning  
    switch(argc){
        case 1:
            break;
        case 3:
            if(argv[1] != "-p"){
                cout << "Wrong Input!" << endl;
                return -1;
            }
            InotifyPath = argv[1];
            break;
        default:
            cout << "Wrong Input!" << endl;
            return -1;
    }

    int p[2];
    int readfd,writefd;
    pid_t pid,pid2;

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
        retval = execl("/usr/bin/X11/inotifywait", "inotifywait", InotifyPath, "-m", "-e", "create", "-e", "moved_to", NULL);
    }
    else{ // Manager //
        close(p[WRITE]);
        int rsize;
        char inbuf[MAXBUFF];
        char *filename;
        int filedes;
        int fd;
        string filenameOUT = "/tmp/";
        char *filenameOUTchar;
        string urls[MAXBUFF/7 + 1]; //http:// has 7 letters so you cant have more than MAXBUFF/7 urls in a MAXBUFF buffer

        char buffer[MAXBUFF];
        ssize_t nread;
        int url_counter;
        int i;
        int j;

        int testbytes;
        string tests;
        char arr[MAXBUFF]; //Highest url, like buffer
        int strLength;

        queue<pid_t> workers_available; //PID of the worker that is available
        pid_t curr_Worker;
        string pathFifo;
        char *pathFifoChar;

        int status; 
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


            //From c_str(), documentation suggests To get a pointer that allows modifying the contents use @c &str[0] instead, put it in README
            filename = &filenametest[0]; //We make string into char * in order the worker to put it in open()

            if(workers_available.empty()){

                if ((pid2 = fork()) < 0)
                {
                    perror("Failed to create proccess");
                    exit(-4);
                }

                if (pid2 == 0){ // Worker //
                    pathFifo = "/tmp/"; //Reinitializing 
                    pathFifo.append(to_string(getpid())); //pid of the worker (execl keeps pid the same)
                    pathFifoChar = &pathFifo[0];
                    if ((mkfifo(pathFifoChar, PERMS) < 0) && (errno != EEXIST)){ 
                        perror("can't create fifo"); 
                    }
                    execl("worker","worker",NULL);
                }else{
                    pathFifo = "/tmp/"; //Reinitializing 
                    pathFifo.append(to_string(pid2)); //pid2 has the pid of the child
                    pathFifoChar = &pathFifo[0];
                    if ((writefd = open(pathFifoChar, O_WRONLY)) < 0){
                        perror("Manager: can't open write fifo \n");
                    }
                    if(write(writefd,filename,strlen(filename)) != strlen(filename)){
                        perror("Filename not passed correctly from manager to worker\n");
                    }

                    while((waitpid(pid2,&status,WNOHANG|WUNTRACED)) == 0); // If not working check instead of -1, do pid2
                    cout << "Manager exited from waitpid and I'm pushing worker as available" << endl;
                    workers_available.push(pid2); //pid2 is childs pid -> workers pid (stays the same with execvp)
                }
            }else{
                cout << "Workers available were NOT empty and so i got into the else" << endl;
                curr_Worker =  workers_available.front();
                workers_available.pop();
                kill(curr_Worker,SIGCONT); //Telling the worker to continue
                pathFifo = "/tmp/"; //Reinitializing 
                pathFifo.append(to_string(curr_Worker));
                pathFifoChar = &pathFifo[0];
                if ((writefd = open(pathFifoChar, O_WRONLY)) < 0){
                    perror("Manager: can't open write fifo \n");
                }
                //Kanoyme write sto WRITE END tou named pipe me onoma curr_Worker (to PID tou dia8esimou worker) to filename
                if(write(writefd,filename,strlen(filename)) != strlen(filename)){
                    perror("Filename not passed correctly from manager to worker\n");
                }
            }
        }
    }
}