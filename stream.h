#define SERVER_PORT 5555
#define SERVER_NEW_CONNECT_PORT 6665
#define SERVER_IP "172.17.15.41"
#define MAXPEER 100
#define BUFFER_SIZE 1024
#define MAXPENDING 10
#define MAXFILENAME 40


//**********test remove peer******************//
//**********Handle absence of file in client***********//
typedef struct{
	char fileName[MAXFILENAME];
	long offset;
} streamRequest;

typedef struct {
	unsigned short port;
	char addr[20];
} endPoint;

typedef struct {
	int peerID;
	endPoint ep;
	char **fileList;
	int numFiles;
} peerInfo;

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