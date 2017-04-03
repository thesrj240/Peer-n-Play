#define SERVER_PORT 5551
#define SERVER_NEW_CONNECT_PORT 6661
#define SERVER_IP "172.17.15.41"
#define MAXPEER 100
#define BUFFER_SIZE 1024
#define MAXPENDING 10
#define MAXFILENAME 40
/* TO-DO:
1) Code Segregation
2) data type of port
*/
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