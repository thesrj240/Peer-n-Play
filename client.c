#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>
#include <wait.h>
#include "stream.h"
#define BUFFER_SIZE 1024

int main(int argc,char** argv){
	if(argc!=3){
		printf("Wrong no. of arguments\n");
		exit(1);
	}
	int sock;                        
    struct sockaddr_in echoServAddr; 
    unsigned short echoServPort;     
    char *servIP;                    
    char *echoString;                
    char buffer[BUFFER_SIZE];     
    //unsigned int echoStringLen;      
    int bytesRcvd, totalBytesRcvd;
    servIP = argv[1]; 
    echoServPort = atoi(argv[2]);
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("socket() failed");
        exit(1);
    }

    
    memset(&echoServAddr, 0, sizeof(echoServAddr));     

    echoServAddr.sin_family      = AF_INET;                     
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   
    echoServAddr.sin_port        = htons(echoServPort); 

    printf("IP:%s\n",servIP);
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0){
        perror("connect() failed");
        exit(1);
    }
    int p[2];
    pipe(p);
    pid_t child_pid;
    child_pid = fork();
    if(child_pid==0){
    	close(p[1]);
    	close(0);
    	dup(p[0]);
    	execlp("mpg123","mpg123","-",NULL);
    	perror("execp");
    	exit(1);
    }
    else{
    	close(p[0]);
    	//streamRequest request;
    	streamRequest request;
    	request.offset = 1000000/4;
    	int sent;
    	strcpy(request.fileName,"thesong.mp3");
    	printf("\nSENDING\n");
    	sent = send(sock,&request,sizeof(request),0);
    	if(sent < 0 ){
    		perror("Request");
    	}
    	printf("\nSENT:%d\n",sent);
    	while(1){
	    	bytesRcvd = recv(sock,buffer,BUFFER_SIZE-1,0);
	    	printf("read: %d\n",bytesRcvd);
            fflush(stdout);
	    	if(bytesRcvd<0){
	    		perror("Recv");
	    		//close(p[1]);
	    		break;
	    	}
	    	else if(bytesRcvd==0){
	    		printf("Connection Closed\n");
	    		break;
	    	}
	    	else{
	    		if(write(p[1],buffer,bytesRcvd) <0){
					perror("Write");
					break;
				}
	    	}
	    }
	    close(p[1]);
	    waitpid(child_pid,NULL,0);
    }


    return 1;
}