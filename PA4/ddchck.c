#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

int parent [10] = {0,};
int threadList [10];

int 
main ()
{
	int fd = open("channel", O_RDONLY | O_SYNC) ;

	while (1) {
		unsigned long int tid;
		char lid[50];
		char buf[256];
		int len =0;
		if ((len = read(fd, buf, 128)) == -1)
			break ;
		if (len > 0) 
			printf("%s\n", buf) ;
	}
	close(fd) ;
	return 0 ;
}

