#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define STR_MAX 256

void printHelp()
{
	printf("usage: jerry < -BlockOpen username filename | -BlockKill username | -ReleaseAll >\n");
	printf("\t BlockOpen: Block user with given user name from opening file has given filename as substring\n");
	printf("\t BlockKill: Make processes made by given user name never be killed by other process\n");
	printf("\t ReleaseAll: Release the protection provided by Mousehole.\n");
}

/* convert username to uid */
int getUid (char * uname, char * uid){
	char command[STR_MAX] = "id -u ";

	/*run the linux command and bring the result */
	strcat(command, uname);
	strcat(command, " 2>&1");
	FILE * pf = NULL;
	pf = popen(command, "r");

	/* check if user put invalid username */
	fgets(uid, STR_MAX, pf);	
	for (int i = 0 ; i < strlen(uid)-1 ; i++){
		if (!isdigit(uid[i])) return -1;
	}	
	
	pclose(pf);	
	return 0;
}

/* Read proc file system to get status of mousehole */
void readProc (){
	char buf[STR_MAX];
	int proc = open("/proc/mousehole", O_RDWR);
	read(proc, buf, STR_MAX);
	puts(buf);
	close(proc);

}

int main(int argc, char *argv[])
{
	char uid[STR_MAX];
	/* Exception handler for invalid command */
	if (argc == 1 || (strcmp(argv[1], "-BlockKill") && strcmp(argv[1], "-BlockOpen") && strcmp(argv[1], "-ReleaseAll"))){
		printf("invalid option\n");
		printHelp();
		
		return 0;
	}
	
	/* Open mousehole proc file system */
	int fd = open("/proc/mousehole", O_RDWR);

	/* Command #1: Release current protection. */
	if (!strcmp(argv[1], "-ReleaseAll") && argc == 2){
		char buf[STR_MAX];
		write(fd, "1", 2);
		readProc();
		return 0;
	}
	
	/* Command #2: Block opening file with specific substring from certain user */
	else if (!strcmp(argv[1], "-BlockOpen") && argc == 4){
		char fname[STR_MAX];
		char msg[STR_MAX] = "2 ";	// command number to pass LKM to identify which option is selected
		strcpy(fname, argv[3]);	// put given string to variable 

		/* Exception handler for invalid username */
		if (getUid(argv[2], uid)){
			printf("No such username in our system.\n");
			return 0;
		}

		/*concat[command username filename] */
		strcat(uid, " ");
		strcat(uid, fname);
		strcat(msg, uid);

		/*write the string into /proc filesystem */
		write(fd, msg, strlen(msg) + 1);
		readProc();
		return 0;	
	}

	/* Command #3: Block killing process that created by given user */
	else if (!strcmp(argv[1], "-BlockKill") && argc == 3){

		/* Exception handler for invalid username */
		if (getUid(argv[2], uid)){
			printf("No such username in our system.\n");
			return 0;
		}

		/*concat[command username] */
		char msg[STR_MAX] = "3 ";
		strcat(msg, uid);

		/*write the string into /proc filesystem */
		write(fd, msg, strlen(msg) + 1);
		readProc();
		return 0;
	}
	else{
		printHelp();
		return 0;
	}

	return 0;	
}
