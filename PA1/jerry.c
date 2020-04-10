#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main (int argc, char *argv[] ){
    char uname[256];
    char fname[256];
		

    if (argc == 3){
        strcpy(uname, argv[1]);
        strcpy(fname, argv[2]);
	
	char uid[256];
	FILE *pf;
	char command[256] = "id -u ";
	strcat(command, uname);
	pf = popen(command, "r");
	fgets(uid, 256, pf);
	uid[strlen(uid)-1] = '\0';
	printf("uid: %s\n", uid);

	printf("uname: %s\n", uname);
	printf("fname: %s\n", fname);

	strcat(fname, " ");
	strcat(fname, uid);

        int fd = open("/proc/openhook", O_RDWR);
        write(fd, fname, strlen(fname)+1);
    }

}
