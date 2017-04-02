#define SERVER_PORT 5550
#define SERVER_NEW_CONNECT_PORT 6660
#define SERVER_IP "172.17.15.41"
#define MAXPEER 100
/* TO-DO:
1) Code Segregation
2) data type of port
*/
typedef struct{
	char fileName[64];
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