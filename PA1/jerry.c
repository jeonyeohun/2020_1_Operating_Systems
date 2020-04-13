#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void help()
{
	printf("usage:<[blockopen username filename] |[blockkill username]>\n");
	printf("\t blockopen: Block user with given user name from opening file has given filename as substring\n");
	printf("\t blockkill: Make processes made by given user name never be killed by other process\n");
}

int main(int argc, char *argv[])
{
	char uname[256];
	char uid[256];
	FILE *pf = NULL;

	/*open mousehole module */
	int fd = open("/proc/mousehole", O_RDWR);

	/*if user did not put any arguments, print help message */
	if (argc == 1 )
	{
		help();
		return 0;
	}

	if (strcmp(argv[1], "blockkill") && strcmp(argv[1], "blockopen"))
	{	
		help();
		return 0;
	}

	/*make linux command with given username */
	char command[256] = "id -u ";
	strcpy(uname, argv[2]);
	strcat(command, uname);

	/*run the linux command and bring the result */
	pf = popen(command, "r");
	fgets(uid, 256, pf);
	uid[strlen(uid) - 1] = '\0';

	/*if suer select blockkill option */
	if (!strcmp(argv[1], "blockopen"))
	{
		if (argc != 4)
		{
			help();
			return 0;
		}
		char fname[256];
		char str[256] = "2 ";	// command number to pass LKM to identify which option is selected
		strcpy(fname, argv[3]);	// put given string to variable 

		/*concat[command username filename] */
		strcat(uid, " ");
		strcat(uid, fname);
		strcat(str, uid);

		/*write the string into /proc filesystem */
		write(fd, str, strlen(str) + 1);
	}

	/*if user select blockopen option */
	if (!strcmp(argv[1], "blockkill"))
	{
		if (argc != 3)
		{
			help();
			return 0;
		}
		/*concat[command username] */
		char str[256] = "3 ";
		strcat(str, uid);
		printf("%s\n", str);
		/*write the string into /proc filesystem */
		write(fd, str, strlen(str) + 1);
	}
	
	char buf[256];
	pf = popen("cat /proc/mousehole", "r");
	fgets(buf, 256, pf);
	buf[strlen(buf)-1] = '\0';
	printf("%s\n", buf);
}
