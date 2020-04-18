#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int cities[50][50];
int size;
int length;
int * used;
int * path;
int min = -1;


int getNcities (char * arg){
    FILE * fp = fopen(arg, "r");
    char temp[256];
    int line = 0 ;
    
    while(fgets(temp, 256, fp)!=NULL){
        line++;
    }
    
    fclose(fp);
    return line;
}

void _travel (int idx){
    if (idx == size){
        length += cities[path[size-1]][path[0]];
        if (min == -1 || min > length){
            min = length;

            printf("%d (", length) ;
            for (int i = 0 ; i < 17 ; i++)
                printf("%d ", path[i]);
            printf("%d)\n", path[0]);
        } 
        length -= cities[path[size-1]][path[0]];
    }
    else {
        for (int i = 0 ; i < 17 ; i++){
            if (used[i] == 0){
                path[idx] = i;
                used[i] = 1;
                length += cities[path[idx-1]][i];
                _travel(idx+1);
                length -= cities[path[idx=1]][i];
                used[i] = 0;
            }
        }
    }
}

void travel (int start){
    path[0] = start ;
    used[start] = 1;
    _travel(1);
    used[start] = 0;
}

int main (int argc, char* argv []){
    FILE * fp = fopen (argv[1], "r");
    int childLimit = atoi(argv[2]);
    int i, j;

    size = getNcities(argv[1]);
    path = (int *) malloc (sizeof(int) * size);
    used = (int *) malloc (sizeof(int) * size);


    for (i = 0 ; i < size ; i++) {
        for (j = 0 ; j < size ; j++) {
            fscanf(fp, "%d", &cities[i][j]) ;
        }
    }
    fclose(fp);

    pid_t pids [childLimit], pid=1;
    int childNum = 0;
    int status;

    /* Keep creating new child process */
    while(1){ 
        /* Stop making new child when the number of current child is full and make until any child terminates */
        if (childLimit == childNum){
            pid_t p = wait(NULL);
            printf("%d out\n", p);
            childNum--;
        }
        /* call fork() if there is space to make new child */   
        else{
            pid = fork();
            /* when forking fails */
            if (pid < 0){
                printf("Failed\n");
                exit(0);
            }
            /* Behavior of child process */
            else if (pid == 0){
                printf("%d in\n", getpid());
                sleep(10);
                exit(0);
            }
            /* Behavior of parent process */
            else{
                childNum++;
            }
        }
    }

    
    printf("done\n");
    free (path);
    free (used);

    return 0;
}
