#define SERVER_PORT 5251
#define SERVER_NEW_CONNECT_PORT 65101
#define SERVER_IP "172.17.15.41"
#define MAXPEER 100
#define BUFFER_SIZE 10240
#define MAXPENDING 10
#define MAXFILENAME 40
#define PEER_LOG_FILENAME "peerlog.txt"
#define REQUEST_LOG_FILENAME "requestlog.txt"

int socketReceive(int clientSocket, void* buffer, size_t receiveSize);

typedef struct{
	char fileName[MAXFILENAME];
	long offset;
    long no_bytes;
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

