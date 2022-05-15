#include <iostream>
#include <regex>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define MAXBUFF 4096
#define PERMS 0666

//For know as a temporary solution, else try to do url_counter function
#define MAXURLS 250

using namespace std;

int url_extracter(char *buffer,string *url_string){
    int url_counter=0;
    string buffer_str = buffer; //To work with regex_search

    regex regexHttp("http://\\S*"); //Explain
    smatch m;
    
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

int main(){
    // temporary solution
    string totalUrls[MAXURLS]; //Double array
    int totalUrlsCounter = 0;
    int totalUrlsNumber[MAXURLS]; //Parallel array with totalUrls, how many times this url is
    for(int w=0;w<MAXURLS;w++){totalUrlsNumber[w] = 1;} //Initializing with 1 (every url exists at least 1 time)
    bool existingUrl = false;
    ///////

    int readfd,writefd;
    char buffer[MAXBUFF];
    ssize_t nread;

    string filenameOUT;
    char *filenameOUTchar;

    int filedes,fd;
    int i,j,z,strLength,url_counter,testbytes;

    string urls[MAXBUFF/7 + 1]; //http:// has 7 letters so you cant have more than MAXBUFF/7 urls in a MAXBUFF buffer
    char arr[MAXBUFF]; //Highest url, like buffer


    string pathFifo = "/tmp/";
    pathFifo.append(to_string(getpid()));
    char *pathFifoChar = &pathFifo[0];

    cout << "The path Fifo Char that worker is trying to read is " << pathFifoChar << endl;
    if ((readfd = open(pathFifoChar, O_RDONLY)) < 0){
        perror("Worker: can't open read fifo \n");
    }

    int rsize;
    char readbuffer[MAXBUFF];
    while(true){
        rsize = read(readfd,readbuffer,MAXBUFF);
        if ((filedes = open(readbuffer, O_RDONLY))== -1){
            perror(" error in opening anotherfile \n");
            exit(1);
        }

        // The file that the urls will be .out
        filenameOUT = "/tmp/"; //reinitialization
        filenameOUT.append(readbuffer); 
        filenameOUT.append(".out");
        filenameOUTchar = &filenameOUT[0]; //We make string into char * in order to put it in open()       
        
        cout << "the filenameout char is " << filenameOUTchar << endl;
        if ((fd = open(filenameOUTchar, O_CREAT|O_TRUNC|O_WRONLY|O_APPEND,0777))== -1){ //If present problem with creation, check putting (...|O_APPEND,0777) 
                perror(" error in creating\n");
                exit(1);
        }

        while((nread = read(filedes,buffer,MAXBUFF)) > 0){
            url_counter = url_extracter(buffer,urls);
            for(i=0;i<url_counter;i++){
                urls[i] = urls[i].substr(7,urls[i].length()); // Removing http:// 
                if(urls[i].length()>3){ //Has at least 4 characters
                    if(urls[i][0] == 'w' && urls[i][1] == 'w' && urls[i][2] == 'w' && urls[i][3] == '.'){ //Has a www. infront
                        urls[i] = urls[i].substr(4,urls[i].length()); //Removing "www."
                    }
                }
                strLength = urls[i].length();
                for(j=0;j<strLength;j++){
                    if(urls[i][j] == '/'){ //We find if and where is a '/' in the url
                        urls[i] = urls[i].substr(0,j); //We keep the string from the start until the j letter meaning where 'j' is
                        break;
                    }
                }
                /*Here for OLD METHOD
                //urls[i].append("\n"); // Each url is on a new line
                //strcpy(arr, urls[i].c_str()); // Make the string -> char in order to work with write()
                */

                //Testing
                existingUrl = false;
                for(z=0;z<totalUrlsCounter;z++){
                    if(totalUrls[z] == urls[i]){
                        totalUrlsNumber[z]++;
                        existingUrl = true;
                    }
                }
                if(!existingUrl){
                    totalUrls[totalUrlsCounter] = urls[i];
                    totalUrlsCounter++;
                }
                //testbytes = write(fd,arr,strlen(arr)); // the length part
            }
            memset(buffer,0,strlen(buffer)); //Flushing the buffer
        }
        for(int w=0;w<totalUrlsCounter;w++){
            totalUrls[w].append(" "); totalUrls[w].append(to_string(totalUrlsNumber[w])); totalUrls[w].append("\n");
            strcpy(arr, totalUrls[w].c_str());
            testbytes = write(fd,arr,strlen(arr));
        }

        //Reinitializing the counter so no need to reinitializing totalUrls 
        totalUrlsCounter = 0; //(we leave garbage data as we are protected for accessing only data that this counter tells us)  
        for(int w=0;w<MAXURLS;w++){totalUrlsNumber[w] = 1;} //Reinitializing totalUrlsNumber

        cout << "Worker will now raise(SIGSTOP)" << endl;
        raise(SIGSTOP);
        cout << "Worker just resumed" << endl;
    }

    exit(0);
}