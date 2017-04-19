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


void child_handler(int signo){
    while(waitpid(-1,NULL,WNOHANG));
}

int getOwnIP(char ownIP[]){
	struct ifaddrs *addrs,*tmp;
	if(getifaddrs(&addrs) < 0){
		perror("getifaddrs");
		return -1;
	}
	tmp = addrs;
	while (tmp!=NULL) 
	{
	    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET && (tmp->ifa_flags & (IFF_RUNNING)) && !(tmp->ifa_flags & IFF_LOOPBACK))
	    {

	        struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
	        strcpy(ownIP,inet_ntoa(pAddr->sin_addr));
	        //printf("%s: %s\n", tmp->ifa_name, inet_ntoa(pAddr->sin_addr));
	        freeifaddrs(addrs);
	        return 0;
	    }

	    tmp = tmp->ifa_next;
	}

	freeifaddrs(addrs);
	return -1;
}

int getFileList(char ***fileList, int *numFiles){
	*numFiles = 0;
	DIR *d;
	struct dirent *dir;
	d = opendir("./audio");
	if (d){
		while ((dir = readdir(d)) != NULL){
			if(dir->d_name[0]!='.'){
				//printf("%s\n", dir->d_name);

				(*numFiles)++;
			}
		}	
		closedir(d);
	}
	*fileList = (char **)malloc((*numFiles)*sizeof(char*));
	d = opendir("./audio");
	int i=0;
	if (d){
		while ((dir = readdir(d)) != NULL){
			if(dir->d_name[0]!='.'){
				(*fileList)[i] = (char*)malloc(MAXFILENAME*sizeof(char));
				strcpy((*fileList)[i],dir->d_name);
				i++;
			}
		}	
		closedir(d);
	}
	return 0;
}

int sendToServer(char myIP[20],unsigned short myPort){
	endPoint myEndPoint;
	strcpy(myEndPoint.addr,myIP);
	myEndPoint.port = myPort;
	struct sockaddr_in serverAddr; 
    int senderSock;
    if ((senderSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("socket() failed");
        return -1;
    }
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;                     
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);   
    serverAddr.sin_port        = htons(SERVER_NEW_CONNECT_PORT);
    if(connect(senderSock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0){
        perror("connect() failed");
        return -1;
        //exit(1);
    }
    if (send(senderSock,&myEndPoint,sizeof(myEndPoint),0) < 0){
    	perror("Send");
    }
    char **fileList=NULL;
	int numFiles=0;
	getFileList(&fileList,&numFiles);
	int i;
	if (send(senderSock,&numFiles,sizeof(numFiles),0) < 0){
    	perror("Send");
    }
    for(i=0;i<numFiles;i++){
    	//puts(fileList[i]);
    	if (send(senderSock,fileList[i], MAXFILENAME*sizeof(char),0) < 0){
    		perror("Send");
    	}
    }

	return 0;
}

int servingPeerServingClientRequest(int clntSock){
	char buffer[BUFFER_SIZE+1];
	FILE *fd;
    streamRequest request;
    //printf("\nReady to READ\n");
    int recv_count;
    recv_count = recv(clntSock,&request,sizeof(request),0);
    if( recv_count < 0){
        perror("Request");
        exit(1);
    }
    if(recv_count == 0){
        printf("Connection Closed\n");
        exit(1);
    }
    // printf("HERE1\n");
    // strcpy(request.fileName,"thesong.mp3");
    // printf("HERE");
    // request.offset = 0;
    //printf("Request: %s %ld\n",request.fileName,request.offset);
    fflush(stdout);
    char path_to_file[MAXFILENAME + 100];
    strcpy(path_to_file,"audio/");
    strcat(path_to_file,request.fileName);

    fd = fopen(path_to_file,"rb");

    if(fd==NULL){
        perror("File Open");
        exit(1);
    }
    if (fseek(fd,request.offset,SEEK_SET) == -1){
        perror("Seek");
    }
    int read_count,send_count;
    //int count_1 = 500;
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
            //sleep(1);
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
    return 0;
}

int servingPeerProcess(void){
    signal(SIGCHLD,child_handler);
    int listenSocket;                    
    int clientSocket;                    
    struct sockaddr_in listenAddr; 
    struct sockaddr_in clientAddr;
    struct sockaddr_in myAddr;
    socklen_t lenMyAddr = sizeof(myAddr);
    char ownIP[20];
    //unsigned short echoServPort;     
    unsigned int clientAddrLength;  
    clientAddrLength = sizeof(clientAddr);
    if(getOwnIP(ownIP) < 0){
    	printf("getOwnIP Error\n");
    	return -1;
    }

    memset(&listenAddr, 0, sizeof(listenAddr));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = inet_addr(ownIP);	 
    listenAddr.sin_port = htons(0);    				//kernel gives unique port itself

    if ((listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("Socket");
        exit(1);
    }	//socket on which this serving peer would serve the client
    if (bind(listenSocket, (struct sockaddr *) &listenAddr, sizeof(listenAddr)) < 0){
        perror("bind() failed");
    }
    
    if (listen(listenSocket, MAXPENDING) < 0){
        perror("listen() failed");
    }

    if(getsockname(listenSocket,(struct sockaddr *)&myAddr,&lenMyAddr) < 0){
    	perror("getsockname");
    }
    sendToServer(inet_ntoa(myAddr.sin_addr),ntohs(myAddr.sin_port));
    //above function is telling the server on which port it will serve the client
    //printf("Serving Peer:%s %d\n",inet_ntoa(myAddr.sin_addr),ntohs(myAddr.sin_port));
    // sleep(100);
    while(1){
        if ((clientSocket = accept(listenSocket, (struct sockaddr *) &clientAddr, &clientAddrLength)) < 0){
            perror("accept() failed");
        }
        
        if(fork()==0){
        	servingPeerServingClientRequest(clientSocket);
        	exit(0);
        }
        else{
        	close(clientSocket);
        }
	}

    return 0;
}

// int main(int argc,char **argv){
// 	servingPeerProcess();
// 	return 0;
// }