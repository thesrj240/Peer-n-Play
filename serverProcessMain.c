#include <stdio.h>
#include <unistd.h>
#include "stream.h"

int clientProcess(char fileName[]);
int serverProcess();
int servingPeerProcess();

int main(){
	if(fork()==0){
		serverProcess();
	}
	else{
		sleep(2);
		servingPeerProcess();
	}
}