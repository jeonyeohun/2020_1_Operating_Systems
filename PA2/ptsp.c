#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <memory.h>

#define MAX_SUBTASK 12             

int cities[51][51];                 // Map of city distance
int visited [51] = {0,};            // Mark visited city
int path [51] = {0,};               // Record route
int minPath [51] = {0,};            // The best path for the shortest distance
int size;                           // The total number of cities
int childNum = 0;                   // Tracking the number of child process currently running

int length;                         // Current weight of distance
int min = -1;                       // Store minimum distance of traversed route

long long checkedRoute=0;           // Number of checked route by single process

pid_t pid = 1;                      // Variable to store pid after calling fork()
int pipes[2];                       // Pair of pipe two write and read

/* Read data from pipe and set the best solution */
void parent_proc (){
    int childMin;  
    long long childCheckedRoute;
    int childminPath [51] = {0,};
    
    read(pipes[0], &childMin, sizeof(childMin));                               // Read min distance from connected child
    read(pipes[0], &childCheckedRoute, sizeof(childCheckedRoute));             // Read number of routes from connected child
    read(pipes[0], &childminPath, sizeof(childminPath));                       // Read minPath from connected child
    
    checkedRoute += childCheckedRoute;                                         // Update total number of checked routes.

    printf("Read Path: (");
        for (int i = 0 ; i <= size ; i++) {
            printf("%d ", childminPath[i]);
        }
        printf(") min: %d\n", childMin); 

    if (min == -1 || min > childMin){                                                      
        min = childMin;                                                        // Update the min distance in main process
        memcpy(minPath, childminPath, sizeof(childminPath));                   // Update the min path in main process
    }    
}

/* Write best solution of the subtask to pipe */
void child_proc(){
    close(pipes[0]);                                                           // Close reading pipe

    write(pipes[1], &min, sizeof(min));                                        // Write min distance value
    write(pipes[1], &checkedRoute, sizeof(checkedRoute));                      // Write number of checked value
    write(pipes[1], minPath, sizeof(minPath));                                 // Write path of the min distance

    close(pipes[1]);                                                           // Close writing pipe 
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
		printf("%d ", minPath[i]);
    }
	printf("%d)\n", minPath[0]); 
    printf("The number of checked route is %lld.\n", checkedRoute);
}

/* Behavior when SIGINT invoked */
void sigintHandler (){                
    if (pid > 0){
        printf("Wait for all children to be terminated\n");
        while(wait(NULL) != -1);                             // Wait for termination of all running child processes.
        printResult();                                       // Print the result
        exit(0);                                             // Terminate main process
    }
}

/* Behavior when SIGCHLD invoked */
void sigchldHandler (){
    parent_proc();                                           // Let main process reads pipe and set the optimal solution 
    childNum--;                                              // Reduce the total number of running child process   
}

/* Recursively traverse all the possible routes and calculate the length */
void _travel (int idx){     
    if (idx == size){
        path[idx] = path[0];                                    // Set route from last city to starting city.

        length += cities[path[idx-1]][path[idx]];               // Add the last city length 	
        checkedRoute++;                                         // Number of routes that the child process traversed  
        
        if (min == -1 || min > length){                         // Check if the length of current permuation is the best    
            min = length;                                       // Set the best value
            memcpy(minPath, path, sizeof(minPath));             // Save the best path
        } 
        length -= cities[path[idx-1]][path[idx]];               // Remove the current city and return to try other permutation   
    }
    else {
        for (int i = 0 ; i < size ; i++){
            if (visited[i] == 0){                               // Check if the route is already visited
                path[idx] = i;                                  // Record the order of visiting
                visited[i] = 1;                                 // Mark as visited
                length += cities[path[idx-1]][i];               // Add length
                _travel(idx + 1);                               // Move to the next city
                length -= cities[path[idx-1]][i];               // Restore length to before visiting the city
                visited[i] = 0;                                 // Reset the marking
            }       
        }
    }
}

/* Create subtasks, create child process and assign the tasks to child process */
void subtaskMaker (int idx, int childNumLimit){     
    /* When prefix of the substask is created. */ 
    if (idx == size-MAX_SUBTASK){ 
        /* Main process behavior after forking */
        if ((pid = fork()) > 0){
            childNum++;                 
            if (childNum == childNumLimit){         // If the number of child process touches to limit
                wait(NULL);                         // Wait until any running child process is terminated
            }
        }
        /* forking failed  */
        if (pid < 0){
            printf("Fork failed.\n");
            exit(0);
        }
        /* Child process behavior after forking */
        if (pid == 0){
            checkedRoute = 0;           // Reset the route counter int
            min = -1;                   // Reset the min value inherited from main process
            _travel(idx);               // Start solving the substasks
            child_proc();               // Write optimal solution from the child process into pipe
            exit(0);                    // Terminate child process
        }     
    }
    else {
        for (int i = 0 ; i < size ; i++){
            if (visited[i] == 0){                             // Check if the route is already visited
                path[idx] = i;                                // Record the order of visiting
                visited[i] = 1;                               // Mark as visited
                subtaskMaker(idx+1, childNumLimit);           // Move to the next city
                visited[i] = 0;                               // Reset the marking
            }       
        }
    }
}

int main (int argc, char* argv []){
    FILE * fp = fopen (argv[1], "r");
    int childNumLimit = atoi(argv[2]);                        // Limit number of child process
    signal(SIGINT, sigintHandler);                            // Set signal handler for SIGINT when occurs interrupt signal detected
    signal(SIGCHLD, sigchldHandler);                          // Set signal handler for SIGCHLD when occurs child process terminated
    
    /* Get number of cities */
    size = getNcities(argv[1]);

    /* Put the length value into array from given tsp file */
    for (int i = 0 ; i < size ; i++) {
        for (int j = 0 ; j < size ; j++) {
            fscanf(fp, "%d", &cities[i][j]) ;
        }
    }
    fclose(fp);

    /* Initialize pipes */
    pipe(pipes);

    /* Start to make subtasks and give them to child processes */
    subtaskMaker(0, childNumLimit);

    /* Wait for all child processes are terminated */
    while((wait(NULL))!= -1);

    /* Printout the result */
    printResult();

    close(pipes[0]);
    close(pipes[1]);
    return 0;
}
