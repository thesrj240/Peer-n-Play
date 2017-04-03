all:
	gcc client.c -o client 
	gcc server.c -o server -pthread
	gcc servingPeer.c -o servingPeer
	