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

int main()
{
    int p[2];
    int readfd,writefd;
    pid_t pid;
    char *filename;

    string mystr = "./ CREATE CriptoMasterProb4-2021-2022.pdf"; //https://stackoverflow.com/questions/54351071/match-the-nth-word-in-a-line
    regex regexInotify("^(?:\\w+ ){2}\\K \\w +"); //Fix this to a good regex
    smatch m;
    regex_search(mystr,m,regexInotify);

    for (auto x : m) 
        cout << x << " "; 
    return 0; 


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
        while(true){
            rsize = read(p[READ], inbuf, MAXBUFF);
            //filename = 
            printf("%.*s\n", rsize, inbuf);
        }
    }
}