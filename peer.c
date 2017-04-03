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


pthread_mutex_t mutexPeerInfoList;

//**********test remove peer******************//
peerInfo peerInfoList[MAXPEER];

void printPeers(){
	int i,j;
	for(i=0;i<MAXPEER;i++){
		if(peerInfoList[i].peerID != -1){
			printf("ID:%d\tIP:%s\tPort:%hu\t \n",peerInfoList[i].peerID,peerInfoList[i].ep.addr,peerInfoList[i].ep.port);
			for(j=0;j<peerInfoList[i].numFiles;j++){
				puts(peerInfoList[i].fileList[j]);
			}
		}
	}
}
	
int initializePeerInfoList(){
	int i;
	pthread_mutex_lock (&mutexPeerInfoList);
	for(i=0;i<MAXPEER;i++){
		peerInfoList[i].peerID = -1;
		peerInfoList[i].fileList = NULL;
		peerInfoList[i].numFiles = -1;
	}
	pthread_mutex_unlock (&mutexPeerInfoList);
	return 0;
}
int addPeer(char ip[],unsigned short port, char **fileList,int numFiles){
	int i;
	pthread_mutex_lock (&mutexPeerInfoList);

	for(i=0;i<MAXPEER;i++){
		if(peerInfoList[i].peerID == -1){
			peerInfoList[i].peerID = i;
			peerInfoList[i].ep.port = port;
			strcpy(peerInfoList[i].ep.addr,ip);
			peerInfoList[i].numFiles = numFiles;
			peerInfoList[i].fileList = fileList;
			pthread_mutex_unlock (&mutexPeerInfoList);
			return i;
		}
	}
	pthread_mutex_unlock (&mutexPeerInfoList);
	return -1;
}

int removePeer(int peerID){
	int i;
	pthread_mutex_lock (&mutexPeerInfoList);
	if(peerInfoList[peerID].peerID==peerID){
		peerInfoList[peerID].peerID = -1;
		for(i=0;i<peerInfoList[peerID].numFiles;i++){
			free(peerInfoList[peerID].fileList[i]);
		}
		free(peerInfoList[peerID].fileList);
		peerInfoList[peerID].fileList = NULL;
		peerInfoList[peerID].numFiles = -1;
		pthread_mutex_unlock (&mutexPeerInfoList);
		return peerID;
	}
	else{
		pthread_mutex_unlock (&mutexPeerInfoList);
		return -1;
	}
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
    	puts(fileList[i]);
    	if (send(senderSock,fileList[i], MAXFILENAME*sizeof(char),0) < 0){
    		perror("Send");
    	}
    }

	return 0;
}

int socketReceive(int clientSocket, void* buffer, size_t receiveSize){
    size_t totalReceived = 0;
    int abhiReceived=0;
	while(totalReceived < receiveSize){
    	abhiReceived = recv(clientSocket,(buffer)+(totalReceived),receiveSize-totalReceived,0);
    	if(abhiReceived<0){
    		perror("Recv");
    		break;
    		//return -1;
    	}
    	else{
    		totalReceived = (size_t)abhiReceived + totalReceived;
    	}
    }
    return totalReceived;
}
void* serverProcessNewPeer(void* dummy){
	initializePeerInfoList();
	int newPeerSocket;                    
    int clientSocket;                    
    struct sockaddr_in newPeerSockAddr; 
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLength = sizeof(clientAddr);
    memset(&newPeerSockAddr, 0, sizeof(newPeerSockAddr));
    newPeerSockAddr.sin_family = AF_INET;
    newPeerSockAddr.sin_addr.s_addr = inet_addr(SERVER_IP);	 
    newPeerSockAddr.sin_port = htons(SERVER_NEW_CONNECT_PORT);    				
    endPoint ep;
    if ((newPeerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("Socket");
        //exit(1);
    }
    if (bind(newPeerSocket, (struct sockaddr *) &newPeerSockAddr, sizeof(newPeerSockAddr)) < 0){
        perror("bind() failed");
    }
    
    if (listen(newPeerSocket, MAXPENDING) < 0){
        perror("listen() failed");
    }
    while(1){
    	if ((clientSocket = accept(newPeerSocket, (struct sockaddr *) &clientAddr, &clientAddrLength)) < 0){
            perror("accept() failed");
        }
        socketReceive(clientSocket,&ep,sizeof(ep));
	    printf("Server:%s %hu\n",ep.addr,ep.port);
	
	//char **fileList=NULL;
		char fileName[MAXFILENAME];
		int numFiles=0,i;
		socketReceive(clientSocket,&numFiles,sizeof(numFiles));
		printf("Num:Files:%d\n",numFiles);
		char **fileList;
		fileList = (char**)malloc(sizeof(char*)*numFiles);
		for(i=0;i<numFiles;i++){
			fileList[i] = (char*)malloc(sizeof(char)*MAXFILENAME);
			socketReceive(clientSocket,fileName, sizeof(fileName));
			strcpy(fileList[i],fileName);
			//puts(fileName);
		}
		if(addPeer(ep.addr,ep.port,fileList,numFiles) < 0){
			printf("Can't Add More Peers\n");
			for(i=0;i<numFiles;i++){
				free(fileList[i]);
			}
			free(fileList);


		}
		fileList = NULL;
		close(clientSocket);
		printPeers();

	}
	return NULL;
}

int searchForFile(char fileName[], endPoint* ep){
	int i;
	pthread_mutex_lock (&mutexPeerInfoList);
	for(i=0;i<MAXPEER;i++){
		if(peerInfoList[i].peerID!=-1){
			int j;
			for(j=0;j<peerInfoList[i].numFiles;j++){
				if(strcmp(fileName,peerInfoList[i].fileList[j])==0){
					ep->port = peerInfoList[i].ep.port;
					strcpy(ep->addr,peerInfoList[i].ep.addr);
					pthread_mutex_unlock (&mutexPeerInfoList);
					return 1;
				}
			}
		}
	}
	pthread_mutex_unlock (&mutexPeerInfoList);
	return -1;
}

void* serverProcessClientRequest(void* dummy){
	int listenSocket;                    
    int clientSocket;                    
    struct sockaddr_in listenAddr; 
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLength = sizeof(clientAddr);
    memset(&listenAddr, 0, sizeof(listenAddr));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = inet_addr(SERVER_IP);	 
    listenAddr.sin_port = htons(SERVER_PORT);
	if ((listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("Socket");
        //exit(1);
    }
    if (bind(listenSocket, (struct sockaddr *) &listenAddr, sizeof(listenAddr)) < 0){
        perror("bind() failed");
    }
    
    if (listen(listenSocket, MAXPENDING) < 0){
        perror("listen() failed");
    }
    while(1){
    	if ((clientSocket = accept(listenSocket, (struct sockaddr *) &clientAddr, &clientAddrLength)) < 0){
            perror("accept() failed");
        }
        char fileName[MAXFILENAME];
        socketReceive(clientSocket,fileName,MAXFILENAME*sizeof(char));
        endPoint selectedServingPeer;
        int isPresent;
        printf("Requested %s\n",fileName);
        isPresent = searchForFile(fileName,&selectedServingPeer);
        
        if(send(clientSocket,&isPresent,sizeof(isPresent),0) < 0){
    		perror("Send");
    	}

    	if(isPresent==1){
    		if(send(clientSocket,&selectedServingPeer,sizeof(selectedServingPeer),0) < 0){
    			perror("Send");
    		}
    	}
    	close(clientSocket);
    }
    return NULL;
}
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

int servingPeerServingClientRequest(int clntSock){
	char buffer[BUFFER_SIZE+1];
	FILE *fd;
    streamRequest request;
    printf("\nReady to READ\n");
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
    printf("Request: %s %ld\n",request.fileName,request.offset);
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
    printf("Serving Peer:%s %d\n",inet_ntoa(myAddr.sin_addr),ntohs(myAddr.sin_port));
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



int main(int argc,char **argv){
	if(argc!=2){
		printf("Enter the correct option\n");
		exit(1);
	}
	int choice = atoi(argv[1]);
	if(choice == 0){
		pthread_mutex_init(&mutexPeerInfoList, NULL);
		pthread_t ptid1,ptid2;
		pthread_create(&ptid1,NULL,serverProcessNewPeer,NULL);
		pthread_create(&ptid2,NULL,serverProcessClientRequest,NULL);
		pthread_join(ptid1,NULL);
		pthread_mutex_destroy(&mutexPeerInfoList);
		//serverProcessNewPeer(NULL);
	}
	else if(choice == 1){
		servingPeerProcess();
	}
	// else if(choice ==2){
	// 	char **fl=NULL;
	// 	int num=0;
	// 	getFileList(&fl,&num);
	// 	int i;
	// 	for(i=0;i<num;i++){
	// 		puts(fl[i]);
	// 	}
	// }
	else if(choice==2){
		clientProcess();
	}
	else{
		printf("Bye\n");
	}
	// struct ifaddrs *addrs,*tmp;
	// getifaddrs(&addrs);
	// tmp = addrs;

	// while (tmp) 
	// {
	//     if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
	//     {
	//         struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
	//         printf("%s: %s\n", tmp->ifa_name, inet_ntoa(pAddr->sin_addr));
	//     }

	//     tmp = tmp->ifa_next;
	// }

	// freeifaddrs(addrs);
	// char ownIP[100];
	// getOwnIP(ownIP);
	// printf("%s\n",ownIP);
	//servingPeerProcess();
	return 0;
}