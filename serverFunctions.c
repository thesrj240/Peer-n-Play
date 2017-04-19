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
#include <time.h>
int socketReceive(int clientSocket, void* buffer, size_t receiveSize){ //place at right places
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
void timestamp(FILE* fp){
    time_t ltime; 
    ltime=time(NULL);
    fprintf(fp,"\n%s",asctime( localtime(&ltime) ) );
}

FILE *requestLog, *peerLog;
pthread_mutex_t mutexPeerInfoList;
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

int removePeer(int peerID){	//like yield, no guarantees here, untested ok please
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
	    //printf("Server:%s %hu\n",ep.addr,ep.port);
	
	//char **fileList=NULL;
		char fileName[MAXFILENAME];
		int numFiles=0,i;
		socketReceive(clientSocket,&numFiles,sizeof(numFiles));
		//printf("Num:Files:%d\n",numFiles);
		char **fileList;
		fileList = (char**)malloc(sizeof(char*)*numFiles);
		for(i=0;i<numFiles;i++){
			fileList[i] = (char*)malloc(sizeof(char)*MAXFILENAME);
			socketReceive(clientSocket,fileName, sizeof(fileName));
			strcpy(fileList[i],fileName);
			//puts(fileName);
		}
		peerLog = fopen(PEER_LOG_FILENAME,"a");
		timestamp(peerLog);
		fprintf(peerLog,"%s:%hu File List: ",ep.addr,ep.port);
		for(i=0;i<numFiles;i++){
			fprintf(peerLog,"%s ",fileList[i]);
		}
		fprintf(peerLog,"\n");
		fclose(peerLog);
		if(addPeer(ep.addr,ep.port,fileList,numFiles) < 0){
			printf("Can't Add More Peers\n");
			for(i=0;i<numFiles;i++){
				free(fileList[i]);
			}
			free(fileList);
		}
		fileList = NULL;
		close(clientSocket);
		//printPeers();
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
        //printf("Requested %s\n",fileName);
        requestLog = fopen(REQUEST_LOG_FILENAME,"a");
        timestamp(requestLog);
        fprintf(requestLog,"%s:%hu requested %s\n",inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port),fileName);
        fclose(requestLog);
        isPresent = searchForFile(fileName,&selectedServingPeer);
        //inet_ntoa(myAddr.sin_addr),ntohs(myAddr.sin_port)
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
void serverProcess(){
	pthread_mutex_init(&mutexPeerInfoList, NULL);
	pthread_t ptid1,ptid2;
	pthread_create(&ptid1,NULL,serverProcessNewPeer,NULL);
	pthread_create(&ptid2,NULL,serverProcessClientRequest,NULL);
	pthread_join(ptid1,NULL);
	pthread_mutex_destroy(&mutexPeerInfoList);
}

// int main(int argc,char **argv){	
// 	return 0;
// }
