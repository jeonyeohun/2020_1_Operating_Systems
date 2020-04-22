#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

int cities[51][51];                 // Map of city distance
int used [51];                      // Mark visited city
int path [51];                      // Record route
int size;                           // The total number of cities
int taskPrefix;                     // Task prefix. This prefix will be the identifier of single process

int length;                         // Current weight of distance
int min = INT32_MAX;                       // Store minimum distance of traversed route
int minpath [51] = {0,};
int checkedRoute=0;               // Number of checked route by single process

pid_t pid = 1;                      // Get pid after calling fork()

int childLimit;                     // The Maximum number of child process 
int childNum = 0;                   // Tracking the number of child process currently running
int pipes[101] ;                     // Number of pipes each process will have a pair of pipes one for read, one for write

int kill = 0;

void printPermu();

void parent_proc (int f){
    close(pipes[2*f+1]);
    int m;
    int p [51];
    int nr;
    read(pipes[2*f], &m, sizeof(min));
    read(pipes[2*f], &nr, sizeof(nr));
    for (int i = 0 ; i < size ; i++){
        read(pipes[2*f], &p[i], sizeof(int));
    }
    
    checkedRoute += nr;

    if (min > m){
        min = m;
        memcpy(minpath, p, sizeof(p[0]) * size );
    }
    
    close(pipes[2*f]);
}

void child_proc(int f){
    close(pipes[2*f]);
    write(pipes[2*f+1], &min, sizeof(min));
    write(pipes[2*f+1], &checkedRoute, sizeof(checkedRoute));
    write(pipes[2*f+1], minpath, sizeof(minpath));
    
    close(pipes[2*f+1]);
    
}


/* Read line number from given file to figure out the number N */
int getNcities (char * arg){
    FILE * fp = fopen(arg, "r");
    char temp[256];
    int line = 0;

    while(fgets(temp, 256, fp) != NULL){
        line++;
    } 

    fclose(fp);
    return line;
}

/* Print permutation */
void printPermu(){
    printf("%d (", min);
	for (int i = 0 ; i < size ; i++) {
		printf("%d ", minpath[i]);
    }
	printf("%d)\n", minpath[0]); 
    printf("The number of checked route is %d.\n", checkedRoute);
}

void sigintHandler (int sig){
    if (pid == 0){
        kill = 1;
        return;
    }
    else{
        for (int i = 1 ; i < size ; i++){
            wait(NULL);                         // Wait for all children are teminated
        }

        for (int i = 1 ; i < size ; i++){
            parent_proc(i);                     // Read data from pipes 
        }
        printPermu();
        exit(0);
    }
}

/* Recursively traverse all the possible routes and calculate the length */
void _travel (int idx){     
    if (idx == size){                                   // Traveling all the cities is done
        length += cities[path[size-1]][path[0]];        // Add the last city length 	
        checkedRoute+=1;                                 // Number of routes that the child process traversed  
        
        if (min > length){                 // Check if the length of current permuation is the best    
            min = length;                               // Set the best value
            memcpy(minpath, path, sizeof(path[0]) * size );
        } 
        length -= cities[path[size-1]][path[0]];        // Remove the current city and return to try other permutation
        if (kill == 1) {
            child_proc(taskPrefix);
            exit(0);
        }
    }
    else {
        for (int i = 1 ; i < size ; i++){
            /* Main process behavior */
            if (pid > 0) {
                if ((pid = fork()) < 0) {
                    checkedRoute = 0;
                    idx++ ;
                }
                taskPrefix = i;
                childNum++;
            }
            /* Main process checker */    
            if (pid > 0) {
                /* Wait if the number of child process is max */
                if (childNum == childLimit){
                    wait(NULL) ;
                    childNum-- ;        
                }              
            }
            /* If forking fails, terminates */
            else if (pid == -1){
                printf("fork failed");
                exit(-1);
            }
            /* Child process behavior */
            else if (pid == 0){
                if (used[i] == 0){                       // Check if the route is already visited
                    path[idx] = i;                       // Record the order of visiting
                    used[i] = 1;                         // Mark as visited
                    length += cities[path[idx-1]][i];    // Add length
                    _travel(idx+1);                      // Move to the next city
                    length -= cities[path[idx-1]][i];    // Restore length to before visiting the city
                    if (i == taskPrefix) return;         // If the backtracking comes back to the prefix, terminates child
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
    if (pid == 0){
        child_proc(taskPrefix);
        exit(0);
    }
}

int main (int argc, char* argv []){
    FILE * fp = fopen (argv[1], "r");
    childLimit = atoi(argv[2]);
    signal(SIGINT, sigintHandler);
    int i, j;

    /* Get number of cities */
    size = getNcities(argv[1]);

    /* Put the length value into array from given tsp file */
    for (i = 0 ; i < size ; i++) {
        for (j = 0 ; j < size ; j++) {
            fscanf(fp, "%d", &cities[i][j]) ;
        }
    }
    fclose(fp);

    /* Allocate pipes */
    for (int i = 0 ; i <=size ; i++){
        pipe(&pipes[2*i]);
    }
    time_t start = time(NULL);

    /* Start traverse routes from 0 */
    travel(0);
    
    for (int i = 1 ; i < size ; i++){
        wait(NULL);                         // Wait for all children are teminated
        parent_proc(i);                     // Read data from pipes 
    }

    printPermu();

    printf("All child processes are cleared.\n");
    time_t end = time(NULL);
    printf("The computing time is: %ld seconds \n", (end-start));
    
    return 0;
}