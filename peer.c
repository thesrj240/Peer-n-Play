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
#include "stream.h"
#define BUFFER_SIZE 1024
#define MAXPENDING 10
#define MAXFILENAME 40



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
int serverProcessNewPeer(void){
	int newPeerSocket;                    
    int clientSocket;                    
    struct sockaddr_in newPeerSockAddr; 
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLength = sizeof(clientAddr);
    memset(&newPeerSockAddr, 0, sizeof(newPeerSockAddr));
    newPeerSockAddr.sin_family = AF_INET;
    newPeerSockAddr.sin_addr.s_addr = inet_addr(SERVER_IP);	//chenge this later 
    newPeerSockAddr.sin_port = htons(SERVER_NEW_CONNECT_PORT);    				//kernel gives unique port itself
    endPoint ep;
    if ((newPeerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("Socket");
        exit(1);
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
	    // size_t totalReceived = 0;
	    // int abhiReceived;
	    
	    // while(totalReceived < sizeof(ep)){
	    // 	abhiReceived = recv(clientSocket,(&ep)+(totalReceived),sizeof(ep)-totalReceived,0);
	    // 	if(abhiReceived<0){
	    // 		perror("Recv");
	    // 		break;
	    // 		//return -1;
	    // 	}
	    // 	else{
	    // 		totalReceived = (size_t)abhiReceived + totalReceived;
	    // 	}
	    // }
	    printf("Server2:%s %hu\n",ep.addr,ep.port);
	
	//char **fileList=NULL;
		char fileName[MAXFILENAME];
		int numFiles=0,i;
		socketReceive(clientSocket,&numFiles,sizeof(numFiles));
		printf("Num:Files:%d\n",numFiles);
		for(i=0;i<numFiles;i++){
			socketReceive(clientSocket,fileName, sizeof(fileName));
			puts(fileName);
		}
	}
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
    //unsigned int clientAddrLength;  
    //clntLen = sizeof(echoClntAddr);
    if(getOwnIP(ownIP) < 0){
    	printf("getOwnIP Error\n");
    	return -1;
    }

    memset(&listenAddr, 0, sizeof(listenAddr));
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_addr.s_addr = inet_addr(ownIP);	//chenge this later 
    listenAddr.sin_port = htons(0);    				//kernel gives unique port itself

    if ((listenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("Socket");
        exit(1);
    }
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
    printf("Serving Peer:%s %d\n",inet_ntoa(myAddr.sin_addr),ntohs(myAddr.sin_port));
    // sleep(100);
    return 0;
}



int main(int argc,char **argv){
	if(argc!=2){
		printf("Enter the correct option\n");
		exit(1);
	}
	int choice = atoi(argv[1]);
	if(choice == 0){
		serverProcessNewPeer();
	}
	else if(choice == 1){
		servingPeerProcess();
	}
	else if(choice ==2){
		char **fl=NULL;
		int num=0;
		getFileList(&fl,&num);
		int i;
		for(i=0;i<num;i++){
			puts(fl[i]);
		}
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