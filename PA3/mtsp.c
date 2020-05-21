#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <memory.h>
#include <pthread.h>

#define MAX_SUBTASK 11            

int cities[51][51];                 // Map of city distance
int visited [51] = {0,};            // Mark visited city
int path [51] = {0,};               // Record route
int minPath [51] = {0,};            // The best path for the shortest distance
int size;                           // The total number of cities
int childNum = 0;                   // Tracking the number of child process currently running

int length;                         // Current weight of distance
int min = -1;                       // Store minimum distance of traversed route

long long checkedRoute=0;           // Number of checked route by single process

int buffer[100];
int in = -1;
int out = -1;


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
    
}

/* Behavior when SIGCHLD invoked */
void sigchldHandler (){                                    // Let main process reads pipe and set the optimal solution 

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



   
    return 0;
}
