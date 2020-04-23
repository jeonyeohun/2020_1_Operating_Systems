#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <memory.h>
#include <limits.h>

int cities[51][51];                 // Map of city distance
int visited [51];                      // Mark visited city
int path [51] = {0,};               // Record route
int minpath [51] = {0,};            // The best path for the shortest distance
int size;                           // The total number of cities

int length;                         // Current weight of distance
int min = INT_MAX;                  // Store minimum distance of traversed route

long long checkedRoute=0;           // Number of checked route by single process

pid_t pid = 1;                      // Get pid by calling fork()
int * pipes;                        // Number of pipes each process will have a pair of pipes one for read, one for write

int killFlag = 0;                   // flag variable to distinguish user sent SIGINT

void parent_proc (int idx){
    close(pipes[2*idx+1]);                                                    // Close write pipe

    int childMin;  
    long long childCheckedRoute;
    int childMinPath [51];
    
    read(pipes[2*idx], &childMin, sizeof(min));                               // Read min distance from connected child
    read(pipes[2*idx], &childCheckedRoute, sizeof(childCheckedRoute));        // Read number of routes from connected child
    for (int i = 0 ; i < size ; i++){           
        read(pipes[2*idx], &childMinPath[i], sizeof(childMinPath[0]));        // Read minpath from connected child
    }
    
    checkedRoute += childCheckedRoute;                                        // Update total number of checked routes.

    if (min > childMin){                                                      
        min = childMin;                                                       // Update the min distance in main process
        memcpy(minpath, childMinPath, sizeof(childMinPath[0]) * size);        // Update the min path in main process
    }
    
    close(pipes[2*idx]);                                                        // Close read pipe
}

/* write min, # of checked route, path of the min distance to pipe */
void child_proc(int idx){
    close(pipes[2*idx]);                                           // close reading pipe

    write(pipes[2*idx+1], &min, sizeof(min));
    write(pipes[2*idx+1], &checkedRoute, sizeof(checkedRoute));
    write(pipes[2*idx+1], minpath, sizeof(minpath));
    
    close(pipes[2*idx+1]);                                         // close writing pipe 
    
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

/* Print min distance, path and number of checked route */
void printResult(){
    printf("\nThe shortest distance: %d\n", min);
    printf("Path: (");
	for (int i = 0 ; i < size ; i++) {
		printf("%d ", minpath[i]);
    }
	printf("%d)\n", minpath[0]); 
    printf("The number of checked route is %lld.\n", checkedRoute);
    
}

/* Catch invoked signals */
void sigintHandler (int sig){                
    /* Child process behavior when signal passed */ 
    if (pid == 0){                           
        killFlag = 1;                               // Change kill flag to true
        return;                                     // Get back to work
    }
    /* Main process behavior for signal */
    else{
        for (int i = 0 ; i < size ; i++){
            wait(NULL);                         // Wait for all children are teminated
            parent_proc(i);                     // Read data from pipes with each children's file director
        }
        printResult();                          // Print the result
        exit(0);                                // Terminate main process
    }
}

/* Recursively traverse all the possible routes and calculate the length */
void _travel (int idx, int taskPrefix){     
    if (idx == size){     
        length += cities[path[size-1]][path[0]];             // Add the last city length 	
        checkedRoute++;                                      // Number of routes that the child process traversed  
        printf("Path: (");
	for (int i = 0 ; i < size ; i++) {
		printf("%d ", path[i]);
    }
	printf("%d) by %d\n", path[0], getpid()); 
        if (min > length){                                   // Check if the length of current permuation is the best    
            min = length;                                    // Set the best value
            memcpy(minpath, path, sizeof(path[0]) * size );  // Save the best path
        } 

        length -= cities[path[size-1]][path[0]];             // Remove the current city and return to try other permutation
        if (killFlag == 1) {                                 // SINGINT signal is invoked. Child processes needs to be terminated after trying last combination
            child_proc(taskPrefix);                          // Write data of current state into pipe
            exit(0);                                         // Terminate child process
        }
    }
    else {
        for (int i = 0 ; i < size ; i++){
            /* Child process behavior */
            if (visited[i] == 0){                           // Check if the route is already visited
                path[idx] = i;                              // Record the order of visiting
                visited[i] = 1;                             // Mark as visited
                length += cities[path[idx-1]][i];           // Add length
                _travel(idx+1, taskPrefix);                 // Move to the next city
                length -= cities[path[idx-1]][i];           // Restore length to before visiting the city
                visited[i] = 0;                             // Reset the marking
            }       
        }
    }
}

int main (int argc, char* argv []){
    int childNumLimit;                                     // The Maximum number of child process 
    int childNum = 0;                                   // Tracking the number of child process currently running
    int childTotal = 0;

    FILE * fp = fopen (argv[1], "r");
    childNumLimit = atoi(argv[2]);                         // Limit number of child process
    signal(SIGINT, sigintHandler);                      // Set signal handler for SIGINT with defined function

    /* Get number of cities */
    size = getNcities(argv[1]);

    /* Put the length value into array from given tsp file */
    for (int i = 0 ; i < size ; i++) {
        for (int j = 0 ; j < size ; j++) {
            fscanf(fp, "%d", &cities[i][j]) ;
        }
    }

    fclose(fp);

    /* Allocate and initialize pipes */
    pipes = (int*)malloc(sizeof(int) * (size*(size-1)));
    for (int i = 0 ; i <size ; i++){
        pipe(&pipes[2*i]);
    }

    /* Start traverse routes from 0 */
    for (int i = 0 ; i < size ; i++){
        visited[i] = 1;
        path[0] = i;                                      // Mark start node as visited 
        for (int j = 0 ; j < size ; j++){
            if (i == j) continue;

            checkedRoute = 0;
            visited[j] = 1;
            path[1] = j;
            length = cities[path[i]][j]; 

            pid = fork();         // Create child process
            /* Forking failed */
            if (pid == -1){ 
                printf("fork failed\n");
                exit(-1);
            }
            /* Child process behavior */
            else if (pid == 0){
                _travel(2, i*size + j);
                child_proc(i*size + j);                                // If child finishes his task, write data into given pipe and terminates
                exit(0);
            }
            /* Main process behavior. Child never gets to this point */
            childNum++; 
            childTotal++; 
            if (childNum == childNumLimit){                      // Check if the current process is up to the limit
                wait(NULL);                                   // Wait for any child finishes his work
                childNum--;                                   // Update the number of child right after any child terminates
            }
            visited[j] = 0;
            
        }
        visited[i] = 0;    
    }
    
    /* All tasks are distributed to children */
    for (int i = 1 ; i <= childTotal ; i++){
        wait(NULL);                         // Wait for all children are teminated
        parent_proc(i);                     // Read data from pipes 
    }

    printf("total number of utilized child process is : %d", childTotal);

    /* Printout the result */
    printResult();

    return 0;
}
