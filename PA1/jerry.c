#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define STR_MAX 256

void printHelp()
{
	printf("usage:< -BlockOpen username filename | -BlockKill username | -ReleaseAll >\n");
	printf("\t BlockOpen: Block user with given user name from opening file has given filename as substring\n");
	printf("\t BlockKill: Make processes made by given user name never be killed by other process\n");
	printf("\t ReleaseAll: Release the protection provided by Mousehole.\n");
}

/* convert username to uid */
int getUid (char * uname, char * uid){
	FILE *pf = NULL;
	char command[STR_MAX] = "id -u ";
	char uid[STR_MAX];
	
	/*run the linux command and bring the result */
	pf = popen(command, "r");
	strcat(command, uname);
	fgets(uid, STR_MAX, pf);
	uid[strlen(uid) - 1] = '\0';

	/* check if user put invalid username  */
	for (int i = 0 ; i < stelen(uid) ; i++){
		if (!isdigit(uid[i])) return -1;
	}
	
	return 0;
}

int main(int argc, char *argv[])
{
	char uid[STR_MAX];
	/* Exception handler for invalid command */
	if (argc == 1 || (strcmp(argv[1], "-BlockKill") && strcmp(argv[1], "-BlockOpen") && strcmp(argv[1], "-ReleaseAll"))){
		printHelp();
		return 0;
	}
	
	/* Exception handler for invalid username */
	if (getUid(argv[2], uid)){
		printf("No such username in our system.\n");
		return 0;
	}

	/* Open mousehole proc file system */
	int fd = open("/proc/mousehole", O_RDWR);

	/* Command #1: Release current protection. */
	if (!strcmp(argv[1], "-ReleaseAll" && argc == 2)){
		write(fd, "1", 2);
	}

	/* Command #2: Block opening file with specific substring from certain user */
	else if (!strcmp(argv[1], "-BlockOpen") && argc == 4){
		char fname[STR_MAX];
		char msg[STR_MAX] = "2 ";	// command number to pass LKM to identify which option is selected
		strcpy(fname, argv[3]);	// put given string to variable 

		/*concat[command username filename] */
		strcat(uid, " ");
		strcat(uid, fname);
		strcat(msg, uid);

		/*write the string into /proc filesystem */
		write(fd, msg, strlen(msg) + 1);
	}

	/* Command #3: Block killing process that created by given user */
	else if (!strcmp(argv[1], "-BlockKill") && argc == 3){

		/*concat[command username] */
		char msg[STR_MAX] = "3 ";
		strcat(msg, uid);

		/*write the string into /proc filesystem */
		write(fd, msg, strlen(msg) + 1);
	}
	else{
		printf("Invalid command.\n");
		printHelp();
		return 0;
	}
	
	/* Read proc file system to get status of mousehole */
	char buf[STR_MAX];
	read(fd, buf, STR_MAX);
	puts(buf);
}
