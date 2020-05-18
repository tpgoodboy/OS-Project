#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#define BUF_SIZE 100

int main(){
	char msg[BUF_SIZE+1];
	int fd = open("/dev/mypipe_out", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd == -1)
		printf("output device open failed, %d!\n", fd);
	else{
		int flag;
		while(1){
			memset(msg, '\0', BUF_SIZE+1);
			flag = read(fd, msg, BUF_SIZE+1);
			if(flag)
				printf("receive message: %s\n", msg);
			if(strcmp(msg, "quit") == 0){
				close(fd);
				break;
			}
		}
	}
	return 0;
}

