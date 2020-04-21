#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int cities[51][51];                 // Map of city distance
int used [51];                      // Mark visited city
int path [51];                      // Record route
int size;                           // The total number of cities
int taskPrefix;                     // Task prefix. This prefix will be the identifier of single process
int results [51];                   

int length;                         // Current weight of distance
int min = -1;                       // Store minimum distance of traversed route
int checkedRoute = 0;               // Number of checked route by single process

pid_t pid = 1;                      // Get pid after calling fork()

int childLimit;                     // The Maximum number of child process 
int childNum = 0;                   // Tracking the number of child process currently running
int pipes[26] ;                     // Number of pipes each process will have a pair of pipes one for read, one for write

void parent_proc (int f){
    close(pipes[2*f+1]);
    int n, m;
    read(pipes[2*f], &n, sizeof(min));
    read(pipes[2*f], &m, sizeof(min));

    results[n] = m;
    close(pipes[2*f]);
}

void child_proc(int f){
    close(pipes[2*f]);
    write(pipes[2*f+1], &taskPrefix, sizeof(min));
    write(pipes[2*f+1], &min, sizeof(min));

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
    printf("%d (", length);
	for (int i = 0 ; i < size ; i++) {
		printf("%d ", path[i]);
    }
	printf("%d) by %d\n", path[0], getpid()); 
}

/* Recursively traverse all the possible routes and calculate the length */
void _travel (int idx){     
    if (idx == size){                                   // Traveling all the cities is done
        length += cities[path[size-1]][path[0]];        // Add the last city length 	
        printPermu();                                   // Print route 
        checkedRoute++;                                 // Number of routes that the child process traversed  
        if (min == -1 || min > length){                 // Check if the length of current permuation is the best
            min = length;                               // Set the best value
        } 
        length -= cities[path[size-1]][path[0]];        // Remove the current city and return to try other permutation
    }
    else {
        for (int i = 1 ; i < size ; i++){
            /* Main process behavior */
            if (pid > 0) {
                if ((pid = fork()) < 0) idx++ ;
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
    for (int i = 0 ; i <13 ; i++){
        pipe(&pipes[2*i]);
    }
    time_t start = time(NULL);

    /* Start traverse routes from 0 */
    travel(0);
    
    for (int i = 1 ; i <= size ; i++){
        wait(NULL);                         // Wait for all children are teminated
        parent_proc(i);                     // Read data from pipes 
    }
    
    for (int i = 1 ; i < size ; i++){
        printf("%d ", results[i]);
    }
    printf("\n");

    printf("All child processes are cleared.\n");
    time_t end = time(NULL);
    printf("The computing time is: %ld seconds \n", (end-start));
    
    return 0;
}