#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main (int argc, char *argv[] ){
   
    /* if user did not put any arguments, print help message */
    if (argc == 1){
        printf("usage: <[blockopen username filename] | [blockkill username]>\n");
        printf("\t blockopen: Block user with given user name from opening file has given filename as substring\n");
        printf("\t blockkill: Make processes made by given user name never be killed by other process\n");
	return 0;
    }
   char uname[256];
   char uid[256] ;
   char buf[2560];
   FILE *pf = NULL;
         /* write the string into /proc filesystem */
        int fd = open("/proc/mousehole", O_RDWR);
   
   /* make linux command with given username */
   char command[256] = "id -u ";
   strcpy(uname, argv[2]);
   strcat(command, uname);

   /* run the linux command and bring the result */
   pf = popen(command, "r");
   fgets(uid, 256, pf);
   uid[strlen(uid)-1] = '\0';
 

    /* if suer select blockkill option */
    if (!strcmp(argv[1], "blockopen")){
	if (argc != 4) return 0;
        char fname[256];
	char str[256] = "2 "; // command number to pass LKM to identify which option is selected
        strcpy(fname, argv[3]); // put given string to variable 

        /* concat [command username filename] */
	strcat(uid, " ");
        strcat(uid, fname);
	strcat(str, uid);

        write(fd, str, strlen(str)+1);
   }


    /* if user select blockopen option */
    if (!strcmp(argv[1], "blockkill")){
	if (argc != 3) return 0;
        /* concat [command username] */
       	char str[256] = "3 " ;
	strcat(str, uid);
	printf("%s\n", str);
        /* write the string into /proc filesystem */
        write(fd, str, strlen(str)+1);
    }
	fgets(buf, 256, fd);	
printf("%s", buf);

}
