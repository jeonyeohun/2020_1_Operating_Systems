#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main (int argc, char *argv[] ){
    char uname[256];
    char fname[256];

    if (argc == 3){
        strcpy(uname, argv[1]);
        strcpy(fname, argv[2]);

        FILE *fd = open("/proc/openhook", "w");
        fprintf(fd, uname, fname);
    }

}
