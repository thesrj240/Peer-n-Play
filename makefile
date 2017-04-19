all:
	#gcc client.c -o client
	#gcc server.c -o server -pthread
	#gcc servingPeer.c -o servingPeer
	gcc clientFunctions.c serverFunctions.c servingPeerFunctions.c peerProcessMain.c -pthread -o peer
	gcc clientFunctions.c serverFunctions.c servingPeerFunctions.c serverProcessMain.c -pthread -o server 