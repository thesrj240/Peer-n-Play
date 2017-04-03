#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>
#include <wait.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <dirent.h>
#include <pthread.h>
#include "stream.h"


int getServingPeerListFromServer(endPoint *servingPeerEP,char fileName[]){
	// endPoint myEndPoint;
	// strcpy(myEndPoint.addr,myIP);
	// myEndPoint.port = myPort;
	//puts(fileName);
	struct sockaddr_in serverAddr; 
    int senderSock;
	if ((senderSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("socket() failed");
        return -1;
    }	
	memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;                     
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);   
    serverAddr.sin_port        = htons(SERVER_PORT);
    if(connect(senderSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0){
        perror("connect() failed");
        return -1;
        //exit(1);
    }
    if (send(senderSock,fileName,sizeof(char)*MAXFILENAME,0) < 0){
    	perror("Send");
    }
    int isPresent=0;

    socketReceive(senderSock, &isPresent, sizeof(isPresent));

    if(isPresent==1){
    	socketReceive(senderSock, servingPeerEP, sizeof(endPoint));    	
    	return 0;
    }

    return -1;
}

int getStreamFromPeer(char fileName[], endPoint ep){
	
	struct sockaddr_in peerAddr; 
    int peerSocket;
    if ((peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("socket() failed");
        return -1;
    }
    
    memset(&peerAddr, 0, sizeof(peerAddr));
    peerAddr.sin_family      = AF_INET;                     
    peerAddr.sin_addr.s_addr = inet_addr(ep.addr);   
    peerAddr.sin_port        = htons(ep.port);
    if(connect(peerSocket, (struct sockaddr *) &peerAddr, sizeof(peerAddr)) < 0){
        perror("connect() failed");
        return -1;
        //exit(1);
    }
    char buffer[BUFFER_SIZE];
	int bytesRcvd, totalBytesRcvd;
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
    	int long total_received = 0;
    	// int abnormal_close = 0;
    	int sent;
    	close(p[0]);
    	//streamRequest request;
    	streamRequest request;
    	request.offset = 0;
    	//request.offset = 1000000/4;
    	
    	strcpy(request.fileName,fileName);
    	printf("\nSENDING\n");
    	sent = send(peerSocket,&request,sizeof(request),0);
    	if(sent < 0 ){
    		perror("Request");
    	}
    	printf("\nSENT:%d\n",sent);
    	while(1){
	    	bytesRcvd = recv(peerSocket,buffer,BUFFER_SIZE-1,0);

	    	// printf("read: %d\n",bytesRcvd);
      //       fflush(stdout);
	    	if(bytesRcvd<0){
	    		perror("Recv");
	    		//close(p[1]);
	    		break;
	    	}
	    	else if(bytesRcvd==0){
	    		printf("Connection Closed\n");
	    		//abnormal_close = 1;
	    		break;
	    	}
	    	else{
	    		if(write(p[1],buffer,bytesRcvd) <0){
					perror("Write");
					break;
				}
				total_received+=bytesRcvd;
	    	}
	    }
    	close(p[1]);
	    waitpid(child_pid,NULL,0);
    }

	return 0;
}

int clientProcess(void){
	char fileName[MAXFILENAME];
	printf("Enter file name\n");
	fgets(fileName,MAXFILENAME,stdin);
	int i;
	for(i=0;i<MAXFILENAME;i++){
		if(fileName[i]=='\n'){
			fileName[i] = '\0';
			break;			
		}
	}
	
	//gets(fileName);
	endPoint servingPeerEP;
	int isPresent = getServingPeerListFromServer(&servingPeerEP,fileName);
	if(isPresent==-1){
		printf("Not Present\n");
	}
	else{
		printf("%s\t%hu\n",servingPeerEP.addr, servingPeerEP.port);
	}
	getStreamFromPeer(fileName,servingPeerEP);

	return 0;
}


int main(int argc,char **argv){
	clientProcess();
	return 0;
}