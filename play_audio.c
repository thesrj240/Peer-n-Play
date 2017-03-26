#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#define BUFFER_SIZE 1024
int main(){
	int fd,p[2];
	char buffer[BUFFER_SIZE+1];

	pipe(p);
	pid_t child_pid;
	child_pid = fork();
	if(child_pid==0){
		close(p[1]);
		close(0);
		dup(p[0]);
		execlp("mpg123","mpg123","-",NULL);
		perror("execvp");
		exit(1);

	}
	else{
		int test=0;
		close(p[0]);
		fd = open("thesong.mp3",O_RDONLY);
		int read_count;
		
		while(test<250){
			read_count = read(fd,buffer,BUFFER_SIZE);
			if(read_count<0){
				perror("Read");
				break;
			}
			else if(read_count==0){
				
				break;

			}
			else{
				if(write(p[1],buffer,read_count) <0){
					perror("Write");
					break;
				}
			}

			//printf("Enter something");
			//sleep(1);
			printf("Test:%d\n",test++);
			//scanf("%d",&test);
		}
		close(p[1]);
		close(fd);
		waitpid(child_pid,NULL,0);

		return 0;

	}
	

}	
