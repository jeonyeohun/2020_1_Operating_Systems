#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main (int argc, char *argv[] ){
   char uname[256];
    
    if (argc == 1){
        printf("usage: ./jerry [--blockopen username filename] [--blockkill username]\n");
        printf("/t blockopen: Block user with given user name from opening file has given filename as substring\n");
        printf("/t blockkill: Make processes made by given user name never be killed by other process\n");
    }

    if (argc == 2){
        printf("todo: prevent kill");
        strcpy(uname, argv[1]);
        char uid[256] ;
       	char str[256] = "2 " ;
        FILE *pf;
        
        char command[256] = "id -u ";
        strcat(command, uname);
        pf = popen(command, "r");
        fgets(uid, 256, pf);
        uid[strlen(uid)-1] = '\0';
	strcat(str, uid);
        int fd = open("/proc/openhook", O_RDWR);
        write(fd, str, strlen(str)+1);
    }
	
    if (argc == 3){
        char fname[256];
        char uid[256];
	char str[256] = "3 ";
        strcpy(uname, argv[1]);
        strcpy(fname, argv[2]);
  
       
        FILE *pf;
        
        /*set linux command by concat username with getting uid command*/
        char command[256] = "id -u ";
        strcat(command, uname);

        /* get uid with username*/
        pf = popen(command, "r");
        fgets(uid, 256, pf);
        uid[strlen(uid)-1] = '\0';

        /*put two arguments and command type in one string*/
	strcat(fname, " ");
        strcat(fname, uid);
	strcat(str, fname);

        int fd = open("/proc/openhook", O_RDWR);
        write(fd, str, strlen(str)+1);
    }

}
