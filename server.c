#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h> 
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include "stream.h"
#define MAXPENDING 10
#define BUFFER_SIZE 1024

void child_handler(int signo){
    while(waitpid(-1,NULL,WNOHANG));
}

int main(int argc, char *argv[])
{
    signal(SIGCHLD,child_handler);
    FILE *fd;
    char buffer[BUFFER_SIZE+1];
    int servSock;                    
    int clntSock;                    
    struct sockaddr_in echoServAddr; 
    struct sockaddr_in echoClntAddr; 
    unsigned short echoServPort;     
    unsigned int clntLen;  
    clntLen = sizeof(echoClntAddr);          

    if (argc != 2)     
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  

    
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("Socket");
        exit(1);
    }
      
    
    memset(&echoServAddr, 0, sizeof(echoServAddr));   

    echoServAddr.sin_family = AF_INET;                    

    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    echoServAddr.sin_port = htons(echoServPort);      

    
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0){
        perror("bind() failed");
    }

    
    if (listen(servSock, MAXPENDING) < 0){
        perror("listen() failed");
    }
    while(1){
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0){
            perror("accept() failed");
        }
        
        if(fork()==0){
            streamRequest request;
            printf("\nReady to READ\n");
            if(recv(clntSock,&request,sizeof(request),0) < 0){
                perror("Request");
                exit(1);
            }
            // printf("HERE1\n");
            // strcpy(request.fileName,"thesong.mp3");
            // printf("HERE");
            // request.offset = 0;
            printf("Request: %s %ld\n",request.fileName,request.offset);
            fflush(stdout);
            fd = fopen(request.fileName,"rb");

            if(fd==NULL){
                perror("File Open");
                exit(1);
            }
            if (fseek(fd,request.offset,SEEK_SET) == -1){
                perror("Seek");
            }
            int read_count,send_count;
        
            while(1){
                //sleep(1);
                read_count = fread(buffer,1,BUFFER_SIZE,fd);
               // printf("read: %d\n",read_count);
              //  fflush(stdout);
                if(read_count<0){
                    perror("Read");
                    break;
                }
                else if(read_count==0){
                    
                    break;

                }
                else{
                    send_count = send(clntSock,buffer,read_count,0);
                    //printf("sent: %d\n",send_count);
                    //fflush(stdout);
                    if(send_count<0){
                        perror("Send:");
                        break;
                    }
                    if(send_count!=read_count){
                        perror("Send not same as read");
                    }

                    
                }
            }
            fclose(fd);
            close(clntSock);
        }
        else{
            //close(clntSock);
        }



  
    }

}