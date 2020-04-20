#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int cities[51][51];
int size;

int length;
int min = -1;

int * used;
int * task;
int * path;

pid_t  childList[12] = {0,};

int childLimit;

pid_t pid = 1;                      // Get pid after calling fork()
int childNum = 0;               // Tracking the number of child process currently running

int pipes[24] ;

void printChild(){
    
    for (int i = 0 ; i < childLimit ; i++){
        printf("%d ", childList[i]);
    }
    printf("\n");
}

void parent_proc (int f){
    for (int i=0 ; i < childLimit ; i++){
        if (childList[i] == f) f = i;
    }
    printf("P: %d\n", f);
    close(pipes[2*f+1]);
    read(pipes[2*f], &min, sizeof(min));
    printf("Parent(%d) recieved value: %d\n", getpid(), min);

    close(pipes[2*f]);
}

void child_proc(int f){
    printChild();
    for (int i=0 ; i < childLimit ; i++){
        if (childList[i] == f) f = i;
    }
    printf("C: %d\n", f);
    close(pipes[2*f]);
    write(pipes[2*f+1], &min, sizeof(min));
    printf("Child(%d) send value: %d\n", getpid(), min);

    close(pipes[2*f+1]);
}


/* Read line number from given file to figure out the number N */
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

void printPermu(){
    printf("%d (", length) ;
	for (int i = 0 ; i < size ; i++) {
		printf("%d ", path[i]) ;
    }
	printf("%d) by %d\n", path[0], getpid()) ; 
}

/* Recursively traverse all the possible routes and calculate the length */
void _travel (int idx){   
    int i;
    
    if (idx == size){                                // Traveling all the cities is done
        length += cities[path[size-1]][path[0]];     // Add the last city length 	
        //printPermu();
        if (min == -1 || min > length){              // Check if the length of current permuation is the best
            min = length;                    // Set the best value
        } 
        length -= cities[path[size-1]][path[0]];     // Remove the current city and return to try other permutation
        //exit(0);
    }

    else {
        for (int i = 0 ; i < size ; i++){
            if (pid > 0) {
                pid = fork();
            }    
            if (pid > 0) {
                childNum++;
                for (int i = 0 ; i < childLimit ; i++){
                    if (childList[i] == 0) {
                        childList[i] = pid;
                        break;
                    }
                }

                printChild();

                if (childNum == childLimit){
                    pid_t p = wait(NULL);
                    printf("%d out\n", p);
                    parent_proc(p);
                    for (int i = 0 ; i < childLimit ; i++){
                        if (childList[i] == p) {
                            childList[i] = 0;
                            break;
                        }
                    }
                    childNum--;                    
                } else{ 
                    for (int i = 0 ; i < childNum ; i++){
                        pid_t pd = waitpid(childList[i], NULL, WNOHANG);
                        if (pd > 0 && childList[i] == pd) {
                            parent_proc(pd);
                            //printf("%d out\n", pd);
                            childList[i] = 0;
                            childNum--;
                            break;
                        }
                    }
                }
            }
            else if (pid == -1){
                printf("fork failed");
                exit(-1);
            }
            else if (pid == 0){
                for (int i = 0 ; i < childLimit ; i++){
                    if (childList[i] == 0) {
                        childList[i] = getpid();
                        break;
                    }
                }
                if (used[i] == 0){                       // Check if the route is already visited
                    path[idx] = i;                       // Record the order of visiting
                    used[i] = 1;                         // Mark as visited
                    length += cities[path[idx-1]][i];    // Add length
                    _travel(idx+1);                      // Move to the next city
                    length -= cities[path[idx-1]][i];    // Restore length to before visiting the city
                    used[i] = 0;                         // Reset the marking
                }       
            }   
        }
    }
}

/* Back-tracking starter */
void travel (int start){
    path[0] = start ;
    used[start] = 1;                    
    _travel(1);
    used[start] = 0;
    if (pid == 0){
        child_proc(getpid());
        exit(0);
    }
}

int main (int argc, char* argv []){
    FILE * fp = fopen (argv[1], "r");
    childLimit = atoi(argv[2]);
    int i, j;

    printf("parent: %d\n\n", getpid());

    size = getNcities(argv[1]);
    path = (int *) malloc (sizeof(int) * size);     // Allocate memory for path recorder with the number of cities
    used = (int *) malloc (sizeof(int) * size);     // Allocate memory for visi recorder with the number of cities
    task = (int *) malloc (sizeof(int) * size);

    /* Put the length value into array from given tsp file */
    for (i = 0 ; i < size ; i++) {
        for (j = 0 ; j < size ; j++) {
            fscanf(fp, "%d", &cities[i][j]) ;
        }
    }
    fclose(fp);

    for (int i = 0 ; i <= childLimit ; i++){
        pipe(&pipes[2*i]);
    }

    for (int i = 0 ; i < size ; i++){
        travel(i);
    }
    
    for (int i = 0 ; i < childLimit ; i++){
        wait(NULL);
    }
    
    printf("All child processes are cleared.\n");
    
    free (path);
    free (task);
    free (used);
    return 0;
}