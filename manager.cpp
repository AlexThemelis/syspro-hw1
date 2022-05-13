#include <iostream>
#include <regex>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define READ 0
#define WRITE 1

#define MAXBUFF 4096
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
        //cout << "Caught a " << url_string[url_counter] << endl;
        buffer_str = m.suffix();
        url_counter++;
    }

    return url_counter; //Return how many urls

}

// void url_counter(char *fileNameOUT){
//     int fd;
//     if ((fd = open(fileNameOUT, O_RDONLY))== -1){
//         perror(" error in opening \n");
//         exit(1);
//      }
// }

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


    // if ( (mkfifo(FIFO1, PERMS) < 0) && (errno != EEXIST) ) {
    //     perror("can't create fifo"); 
    // } 
    // if ((mkfifo(FIFO2, PERMS) < 0) && (errno != EEXIST)) {
    //     unlink(FIFO1);
    //     perror("can't create fifo");
    // }

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
            filename = &filenametest[0]; //We make string into char * in order to put it in open()

//////////////////////////////////////Worker Territory///////////////////////////////////////////////////////

            //Making the output as <filename>.out and put it in the /tmp directory
            filenameOUT.append(filename); 
            filenameOUT.append(".out");

            filenameOUTchar = &filenameOUT[0]; //We make string into char * in order to put it in open()

            
            if ((filedes = open(filename, O_RDONLY))== -1){
                perror(" error in opening anotherfile \n");
                exit(1);
            }

            // The file that the urls will be
            if ((fd = open(filenameOUTchar, O_CREAT|O_TRUNC|O_WRONLY|O_APPEND))== -1){ //If present problem with creation, check putting (...|O_APPEND,0777) 
                perror(" error in creating\n");
                exit(1);
            }
            while((nread = read(filedes,buffer,MAXBUFF)) > 0){ //TODO , FIX HOW TO FLUSH BUFFER ETC
                url_counter = url_extracter(buffer,urls);
                //write here in the file the urls
                for(i=0;i<url_counter;i++){
                    //cout << "Before removing http:// " << urls[i] << endl;
                    urls[i] = urls[i].substr(7,urls[i].length()); // Removing http:// 
                    //cout << "after removing http:// " << urls[i] << endl;
                    //cout << "Before removing www. " << urls[i] << endl;
                    if(urls[i].length()>3){ //Has at least 4 characters
                        if(urls[i][0] == 'w' && urls[i][1] == 'w' && urls[i][2] == 'w' && urls[i][3] == '.'){ //Has a www. infront
                            urls[i] = urls[i].substr(4,urls[i].length()); //Removing "www."
                        }
                    }
                    //cout << "After removing http:// " << urls[i] << endl;
                    //cout << "Before removing garbish after / " << urls[i] << endl;
                    strLength = urls[i].length();
                    for(j=0;i<strLength;j++){
                        if(urls[i][j] == '/'){ //We find if and where is a '/' in the url
                            urls[i] = urls[i].substr(0,j); //We keep the string from the start until the j letter meaning where 'j' is
                            break;
                        }
                    }
                    //cout << "After removing garbish after / " << urls[i] << endl;
                    urls[i].append("\n"); // Each url is on a new line
                    strcpy(arr, urls[i].c_str()); // Make the string -> char in order to work with write()
                    testbytes = write(fd,arr,strlen(arr)); // the length part
                }
            }
        }
    }
}