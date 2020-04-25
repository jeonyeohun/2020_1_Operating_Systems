#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <memory.h>
#include <limits.h>

int cities[51][51];                 // Map of city distance
int visited [51] = {0,};                   // Mark visited city
int path [51] = {0,};               // Record route
int minpath [51] = {0,};            // The best path for the shortest distance
int size;                           // The total number of cities
int childTotal = 0;
int childNum = 0;                   // Tracking the number of child process currently running

int length;                         // Current weight of distance
int min = INT_MAX;                  // Store minimum distance of traversed route

long long checkedRoute=0;           // Number of checked route by single process

pid_t pid = 1;                      // Get pid by calling fork()
int pipes[2];                        // Number of pipes each process will have a pair of pipes one for read, one for write

void parent_proc (){
    close(pipes[1]);                                                    // Close write pipe
    
    int childMin;  
    long long childCheckedRoute;
    int childMinPath [51] = {0,};
    
    read(pipes[0], &childMin, sizeof(min));                               // Read min distance from connected child
    read(pipes[0], &childCheckedRoute, sizeof(childCheckedRoute));        // Read number of routes from connected child
    read(pipes[0], &childMinPath, sizeof(childMinPath));        // Read minpath from connected child
    
    checkedRoute += childCheckedRoute;                                        // Update total number of checked routes.

    if (min > childMin){                                                      
        min = childMin;                                                       // Update the min distance in main process
        memcpy(minpath, childMinPath, sizeof(childMinPath[0])*size);        // Update the min path in main process
    }

    printf("Read Path: (");
	for (int i = 0 ; i < size ; i++) {
		printf("%d ", childMinPath[i]);
    }
	printf("%d)\n", childMinPath[0]); 
    
    //                                   close(pipes[0]);              // Close read pipe
    
}

/* write min, # of checked route, path of the min distance to pipe */
void child_proc(){
    close(pipes[0]);                                           // close reading pipe

    
    write(pipes[1], &min, sizeof(min));
    write(pipes[1], &checkedRoute, sizeof(checkedRoute));
    write(pipes[1], minpath, sizeof(minpath[0])*size);

    printf("Write Path: (");
	for (int i = 0 ; i < size ; i++) {
		printf("%d ", minpath[i]);
    }
	printf("%d)\n", minpath[0]); 
    //close(pipes[1]);                                          // close writing pipe 
    
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
    if (pid > 0){
        printf("Wait for all children to be terminated\n");
        pid_t p;
        int status;
        while((p = waitpid(-1, &status, 0))!= -1);
        printResult();                                       // Print the result
        exit(0);                                             // Terminate main process
    }
}

void sigchldHandler (int sig){
    printf("signal\n");
    parent_proc();
}

/* Recursively traverse all the possible routes and calculate the length */
void _travel (int idx, int taskID){     
    if (idx == size){     
        path[idx+1] = path[0];
        
        length += cities[path[size-1]][path[0]];             // Add the last city length 	
        checkedRoute++;                                      // Number of routes that the child process traversed  
        
        
        if (min > length){                                   // Check if the length of current permuation is the best    
            min = length;                                    // Set the best value
            memcpy(minpath, path, sizeof(path[0]) * (size+1) );  // Save the best path
        } 

        length -= cities[path[size-1]][path[0]];             // Remove the current city and return to try other permutation
        
    }
    else {
        for (int i = 0 ; i < size ; i++){
            if (visited[i] == 0){                            // Check if the route is already visited
                path[idx] = i;                               // Record the order of visiting
                visited[i] = 1;                              // Mark as visited
                length += cities[path[idx-1]][i];            // Add length
                _travel(idx+1, taskID);                      // Move to the next city
                length -= cities[path[idx-1]][i];            // Restore length to before visiting the city
                visited[i] = 0;                              // Reset the marking
            }       
        }
    }
}

void subtaskMaker (int idx, int childNumLimit){     
    if (idx == size-12){     
        for (int i = 0 ; i < idx ; i++){
            printf("%d ", path[i]);
        }
        printf("\n");
        pid = fork();    
        if (pid < 0){
            printf("Fork failed.\n");
            exit(0);
        }
        else if (pid == 0){
            checkedRoute=0;
            
            _travel(idx, childTotal);
            
            child_proc();
            exit(0);
        }
        else{
            
            childNum++;
            childTotal++;
            if (childNum == childNumLimit){                   // Check if the current process is up to the limit
                pid_t term = wait(NULL);                      // Wait for any child finishes his work
                childNum--;                                   // Update the number of child right after any child terminates
            }      
        }
    }
    else {
        for (int i = 0 ; i < size ; i++){
            if (visited[i] == 0){                             // Check if the route is already visited
                path[idx] = i;                                // Record the order of visiting
                visited[i] = 1;                               // Mark as visited
                length += cities[path[idx-1]][i];             // Add length
                subtaskMaker(idx+1, childNumLimit);           // Move to the next city
                length -= cities[path[idx-1]][i];             // Restore length to before visiting the city
                visited[i] = 0;                               // Reset the marking
            }       
        }
    }
}

int main (int argc, char* argv []){
    FILE * fp = fopen (argv[1], "r");
    int childNumLimit = atoi(argv[2]);                        // Limit number of child process
    signal(SIGINT, sigintHandler);                            // Set signal handler for SIGINT with defined function
    signal(SIGCHLD, sigchldHandler);
    
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
    pipe(pipes);

    subtaskMaker(0, childNumLimit);
    
    
    /* All tasks are distributed to children */
    for (int i = 0 ; i < childTotal ; i++){
        wait(NULL);                                           // Wait for all children are teminated
        parent_proc();                                       // Read data from pipes 
    }

    /* Printout the result */
    printResult();

    return 0;
}
