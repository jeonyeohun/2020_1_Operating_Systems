#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main (int argc, char *argv[] ){
   char uname[256];
   char uid[256] ;
   FILE *pf = NULL;
   
   /* make linux command with given username */
   char command[256] = "id -u ";
   strcpy(uname, argv[1]);
   strcat(command, uname);

   /* run the linux command and bring the result */
   pf = popen(command, "r");
   fgets(uid, 256, pf);
   uid[strlen(uid)-1] = '\0';
    
    /* if user did not put any arguments, print help message */
    if (argc == 1){
        printf("usage: ./jerry [--blockopen username filename] [--blockkill username]\n");
        printf("/t blockopen: Block user with given user name from opening file has given filename as substring\n");
        printf("/t blockkill: Make processes made by given user name never be killed by other process\n");
    }

    /* if suer select blockkill option */
    if (!strcmp(argv[2], "--blockopen")){
        char fname[256];
	char str[256] = "2 "; // command number to pass LKM to identify which option is selected
        strcpy(fname, argv[2]); // put given string to variable 

        /* concat [command username filename] */
	strcat(uid, " ");
        strcat(uid, fname);
	strcat(str, uid);

        /* write the string into /proc filesystem */
        int fd = open("/proc/openhook", O_RDWR);
        write(fd, str, strlen(str)+1);
    }

    /* if user select blockopen option */
    if (!strcmp(argv[2], "--blockkill")){
        /* concat [command username] */
       	char str[256] = "3 " ;
	strcat(str, uid);

        /* write the string into /proc filesystem */
        int fd = open("/proc/openhook", O_RDWR);
        write(fd, str, strlen(str)+1);
    }
}