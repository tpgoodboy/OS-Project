#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#define BUF_SIZE 100

int main(){
	char msg[BUF_SIZE+1];
	int fd = open("/dev/mypipe_in", O_RDWR, S_IRUSR | S_IWUSR);
	if(fd == -1)
		printf("input device open failed!\n");
	else{
		int flag;
		while(1){
			printf("send message: ");
			gets(msg);
			flag = write(fd, msg, strlen(msg));
			while(flag == 0)
				flag = write(fd, msg, strlen(msg));
			if(strcmp(msg, "quit") == 0){
				close(fd);
				break;
			}
		}
	}
	return 0;
}

