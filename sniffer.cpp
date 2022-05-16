#include "signal_handlers.h"

#include <iostream>
#include <regex>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include <queue>

#define READ 0
#define WRITE 1

#define MAXBUFF 8192
#define PERMS 0666

#define OUTPUTPATH "/tmp/"

using namespace std;

//Globals
queue<pid_t *> workers_available; // PID of the worker that is available
pid_t listener_pid;
char *InotifyPath = (char *) ".";
//


int main(int argc, char *argv[])
{
    char *InotifyPath = (char *) ".";
    switch(argc){
        case 1:
            break;
        case 3:
            if(strcmp(argv[1],"-p")){
                cout << "Wrong Input!" << endl;
                return -1;
            }
            InotifyPath = argv[2];
            break;
        default:
            cout << "Wrong Input!" << endl;
            return -1;
    }

    signal(SIGCHLD,sigchld_signal_handler); // Instead of ignoring SIGCHLD, handling it with signal_handler
    signal(SIGINT,sigint_signal_handler); // Proper SIGINT for ctrl-c, killing listeners and workers

    int p[2];
    int readfd,writefd;
    pid_t pid,pid2;

    regex regexInotifyCreate("CREATE (.+)"); // Match all the characters after CREATE and a space
    regex regexInotifyMove_to("MOVED_TO (.+)"); // Match all the characters after MOVED_TO and a space
    smatch m;

    string filenameregex;
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
        dup2(p[WRITE], 1); // Redirecting standard output to the write end of the PIPE
        int retval = 0;
        //From the man page, only these 2 events (create and moved_to) is what we care at our exercise
        retval = execl("/usr/bin/X11/inotifywait", "inotifywait", InotifyPath, "-m", "-e", "create", "-e", "moved_to", NULL);
    }
    else{ // Manager //
        listener_pid = pid;

        close(p[WRITE]);
        int rsize;
        char inbuf[MAXBUFF];
        char *filename;

        pid_t *curr_Worker;
        string pathFifo;
        char *pathFifoChar;

        int status; 
        while(true){
            rsize = read(p[READ], inbuf, MAXBUFF);
            inbuffer = inbuf;
            regex_search(inbuffer,m,regexInotifyCreate); //If inotifywait gave CREATE ...
            for (auto x : m)
                filenameregex = x;
            if(filenameregex.empty()){ // If inotifywait gave MOVED_TO and therefore filenametest is empty
                regex_search(inbuffer,m,regexInotifyMove_to); 
                for (auto x : m)
                    filenameregex = x;             
            } 


            //From c_str(), documentation suggests: To get a pointer that allows modifying the contents use @c &str[0] instead
            filename = &filenameregex[0]; //We make string into char * in order the worker to put it in open()

            if(workers_available.empty()){

                if ((pid2 = fork()) < 0)
                {
                    perror("Failed to create proccess");
                    exit(-4);
                }

                if (pid2 == 0){ // Worker //
                    pathFifo = OUTPUTPATH; //Reinitializing 
                    pathFifo.append(to_string(getpid())); //pid of the worker (execl keeps pid the same)
                    pathFifoChar = &pathFifo[0];
                    if ((mkfifo(pathFifoChar, PERMS) < 0) && (errno != EEXIST)){ 
                        perror("can't create fifo"); 
                    }
                    execl("worker","worker",NULL);
                }else{
                    pathFifo = OUTPUTPATH; //Reinitializing 
                    pathFifo.append(to_string(pid2)); //pid2 has the pid of the child
                    pathFifoChar = &pathFifo[0];
                    sleep(1); //Inorder to have time to create the pipe, could use a semaphore but sleep(1) does the job
                    if ((writefd = open(pathFifoChar, O_WRONLY)) < 0){
                        perror("Manager: can't open write fifo \n");
                    }
                    if(strcmp(InotifyPath,".")){ // If InotifyPath is not "." but we have path 
                        char *tempPath = InotifyPath;
                        strcat(tempPath,filename);
                        filename = tempPath;
                    }
                    if(write(writefd,filename,strlen(filename)) != strlen(filename)){
                        perror("Filename not passed correctly from manager to worker\n");
                    }
                }
            }else{
                curr_Worker =  workers_available.front();
                workers_available.pop();
                signal(SIGCHLD,null_handler); // Making SIGCHLD be ignored and thus kill(curr_Worker,SIGCONT) won't trigger it
                kill(*curr_Worker,SIGCONT); // Telling the worker to continue
                signal(SIGCHLD,sigchld_signal_handler); // Making SIGCHLD again to be handled
                pathFifo = OUTPUTPATH; // Reinitializing 
                pathFifo.append(to_string(*curr_Worker)); // Getting the path of available worker's fifo
                free(curr_Worker); // Freeing curr_Worker that we had malloced
                pathFifoChar = &pathFifo[0];
                if ((writefd = open(pathFifoChar, O_WRONLY)) < 0){
                    perror("Manager: can't open write fifo \n");
                }
                // Write in the named pipe with the name curr_worker (PID of available worker the filename)
                if(write(writefd,filename,strlen(filename)) != strlen(filename)){
                    perror("Filename not passed correctly from manager to worker\n");
                }
                close(writefd);
            }
        }
    }
}