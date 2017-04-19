#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include "stream.h"

int clientProcess(char fileName[]);
int serverProcess();
int servingPeerProcess();
int main(){
	signal(SIGINT,SIG_IGN);
	if(fork()==0){
		servingPeerProcess();
	}
	else{

		char fileName[MAXFILENAME];
		
		pid_t child_pid;
		while(1){
			printf("Enter file name\n");
			fgets(fileName,MAXFILENAME,stdin);
			child_pid = fork();
			if(child_pid==0){
				signal(SIGINT,SIG_DFL);
				clientProcess(fileName);
			}
			else{
				waitpid(child_pid,NULL,0);
			}
		}
	}
}