#include "signal_handlers.h"

#include <iostream>
#include <cstring>

#include <sys/types.h>
#include <sys/wait.h>


#include <queue>

#define OUTPUTPATH "/tmp/" //Could put this #define in signal_handlers.cpp to mantain it

extern std::queue<pid_t *> workers_available;
extern pid_t listener_pid;

void sigchld_signal_handler(int signum){ //Signal handler for SIGCHLD
    int status;
    pid_t* pid_ptr = (pid_t *) malloc(sizeof(pid_t));
    while(!(*pid_ptr = waitpid(-1,&status,WNOHANG|WUNTRACED)));
    workers_available.push(pid_ptr);
}

void sigint_signal_handler(int signum){
    pid_t *temp_pid;
    std::string pathFifo;
    char *pathFifoChar;
    while(!workers_available.empty()){
        temp_pid= workers_available.front();
        pathFifo = OUTPUTPATH; // Reinitializing 
        pathFifo.append(std::to_string(*temp_pid)); // Getting the path of worker's fifo
        pathFifoChar = &pathFifo[0];
        remove(pathFifoChar); // We delete the FIFO
        kill(*temp_pid,SIGKILL);
        workers_available.pop();
        free(temp_pid);
    }
    kill(listener_pid,SIGKILL);
    
    std::cout << std::endl;
    exit(0); //Finally exiting
}


void null_handler(int signum){ // Null handler, like ignore

}