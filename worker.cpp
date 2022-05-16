#include <iostream>
#include <regex>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define MAXBUFF 8192
#define PERMS 0666

#define MAXURLS 250

#define OUTPUTPATH "/tmp/"

using namespace std;

int url_extracter(char *,string *);

int main(){
    string totalUrls[MAXURLS];
    int totalUrlsCounter = 0;
    int totalUrlsNumber[MAXURLS]; //Parallel array with totalUrls, how many times this url is
    int w;
    for(w=0;w<MAXURLS;w++){totalUrlsNumber[w] = 1;} //Initializing with 1 (every url exists at least 1 time)
    bool existingUrl = false;

    int readfd;
    char buffer[MAXBUFF];
    ssize_t nread;

    string filenameOUT;
    char *filenameOUTchar;

    int filedes,fd;
    int i,j,z,strLength,url_counter,bytes;

    string urls[MAXBUFF/7 + 1]; //http:// has 7 letters so you cant have more than MAXBUFF/7 urls in a MAXBUFF buffer
    char arr[MAXBUFF]; //Highest url, like buffer


    string pathFifo = OUTPUTPATH;
    pathFifo.append(to_string(getpid()));
    char *pathFifoChar = &pathFifo[0];

    if ((readfd = open(pathFifoChar, O_RDONLY)) < 0){
        perror("Worker: can't open read fifo \n");
        exit(1);
    }

    int rsize;
    char readbuffer[MAXBUFF];
    char *filenameCreate;
    regex regexSlash(".+/(.+)$"); // Takes everything that is after the last / (in other words ignore the path)
    smatch m;
    string regexbuf;
    string filenamestr;
    while(true){
        rsize = read(readfd,readbuffer,MAXBUFF);
        regexbuf = readbuffer;

        regex_search(regexbuf,m,regexSlash);  
        for (auto x : m){
            filenamestr = x;
        }
        if(filenamestr.empty()){
            filenameCreate = readbuffer;
        }else{
            filenameCreate = &filenamestr[0]; // We get the file without the path           
        }

        if ((filedes = open(readbuffer, O_RDONLY))== -1){ // Opens the file with the absolute path 
            perror(" error in opening anotherfile \n");
            exit(1);
        }

        // The file that the urls will be .out
        filenameOUT = OUTPUTPATH; // Reinitialization
        filenameOUT.append(filenameCreate); // Now we create the .out without the path (for the case of absolute path)
        filenameOUT.append(".out");
        filenameOUTchar = &filenameOUT[0]; // We make string into char * in order to put it in open()       
        
        if ((fd = open(filenameOUTchar, O_CREAT|O_TRUNC|O_WRONLY|O_APPEND,0777))== -1){
                perror(" error in creating\n");
                exit(1);
        }

        while((nread = read(filedes,buffer,MAXBUFF)) > 0){
            url_counter = url_extracter(buffer,urls);
            for(i=0;i<url_counter;i++){
                urls[i] = urls[i].substr(7,urls[i].length()); // Removing http:// 
                if(urls[i].length()>3){ // Has at least 4 characters
                    if(urls[i][0] == 'w' && urls[i][1] == 'w' && urls[i][2] == 'w' && urls[i][3] == '.'){ // Has a www. infront
                        urls[i] = urls[i].substr(4,urls[i].length()); // Removing "www."
                    }
                }
                strLength = urls[i].length();
                for(j=0;j<strLength;j++){
                    if(urls[i][j] == '/'){ // We find if and where is a '/' in the url
                        urls[i] = urls[i].substr(0,j); // We keep the string from the start until the j letter meaning where 'j' is
                        break;
                    }
                }

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
            }
            memset(buffer,0,strlen(buffer)); // Flushing the buffer
        }
        for(w=0;w<totalUrlsCounter;w++){
            totalUrls[w].append(" "); totalUrls[w].append(to_string(totalUrlsNumber[w])); totalUrls[w].append("\n");
            strcpy(arr, totalUrls[w].c_str());
            bytes = write(fd,arr,strlen(arr));
        }
        cout << "Successful write of " << filenameOUTchar << endl; 

        // Reinitializing the counter so no need to reinitializing totalUrls 
        totalUrlsCounter = 0; // (we leave garbage data as we are protected for accessing only data that this counter tells us)  
        for(w=0;w<MAXURLS;w++){totalUrlsNumber[w] = 1;} // Reinitializing totalUrlsNumber

        raise(SIGSTOP);
    }

    exit(0);
}

int url_extracter(char *buffer,string *url_string){
    int url_counter=0;
    string buffer_str = buffer; //To work with regex_search

    regex regexHttp("http://\\S*"); //Matches http:// and all the non-white space characters after the match until it finds a white space
    smatch m;
    
    while (regex_search(buffer_str, m, regexHttp)) {
        url_string[url_counter] = m[0]; // We get the match
        buffer_str = m.suffix(); // We search after the match
        url_counter++;
    }

    return url_counter; //Return how many urls

}