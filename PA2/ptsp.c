#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <memory.h>
#include <limits.h>

#define MAX_SUBTASK 12


int cities[51][51];                 // Map of city distance
int visited [51] = {0,};            // Mark visited city
int path [51] = {0,};               // Record route
int minpath [51] = {0,};            // The best path for the shortest distance
int size;                           // The total number of cities
int childNum = 0;                   // Tracking the number of child process currently running

int length;                         // Current weight of distance
int min = INT_MAX;                  // Store minimum distance of traversed route

long long checkedRoute=0;           // Number of checked route by single process

pid_t pid = 1;                      // Get pid by calling fork()
int pipes[2];                        // Number of pipes each process will have a pair of pipes one for read, one for write

void parent_proc (){
    //close(pipes[1]);                                                    // Close write pipe
    
    int childMin;  
    long long childCheckedRoute;
    int childMinPath [51] = {0,};
    
    read(pipes[0], &childMin, sizeof(childMin));                               // Read min distance from connected child
    read(pipes[0], &childCheckedRoute, sizeof(childCheckedRoute));        // Read number of routes from connected child
    read(pipes[0], &childMinPath, sizeof(childMinPath));        // Read minpath from connected child
    
    checkedRoute += childCheckedRoute;                                        // Update total number of checked routes.

    printf("%d, %lld\n", childMin, childCheckedRoute);

     printf("Read Path: (");
        for (int i = 0 ; i <= size ; i++) {
            printf("%d ", childMinPath[i]);
        }
        printf(") min: %d\n", childMin); 

    int sum = 0;
    for(int i = 0 ; i < size ; i++){
        sum += cities[childMinPath[i]][childMinPath[i+1]];
    }
    printf("Check : %d\n", sum);
    if (childMin > -1 && min > childMin){                                                      
        min = childMin;                                                       // Update the min distance in main process
        memcpy(minpath, childMinPath, sizeof(childMinPath));          // Update the min path in main process
    }    
}

/* write min, # of checked route, path of the min distance to pipe */
void child_proc(){
    close(pipes[0]);                                           // close reading pipe

    printf("=======  %d  ========\n", pipes[1]);
    write(pipes[1], &min, sizeof(min));
    write(pipes[1], &checkedRoute, sizeof(checkedRoute));
    write(pipes[1], minpath, sizeof(minpath));

    printf("Write Path: (");
	for (int i = 0 ; i <= size ; i++) {
		printf("%d ", minpath[i]);
    }
	printf(") min: %d\n", min); 
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
        while(wait(NULL) != -1);
        printResult();                                       // Print the result
        exit(0);                                             // Terminate main process
    }
}

void sigchldHandler (int sig){
    pid_t p;
    int status;

    p = wait(&status);
    parent_proc();
    printf("child %d done\n", p);
    childNum--;   
    
}

/* Recursively traverse all the possible routes and calculate the length */
void _travel (int idx){     
    if (idx == size){
        path[idx] = path[0];     

        length += cities[path[idx-1]][path[idx]];                // Add the last city length 	
        checkedRoute++;                                         // Number of routes that the child process traversed  
        
        if (min > length || min == -1){                         // Check if the length of current permuation is the best    
            min = length;                                       // Set the best value
            memcpy(minpath, path, sizeof(minpath));  // Save the best path
        } 
        length -= cities[path[idx-1]][path[idx]];                 // Remove the current city and return to try other permutation   
    }
    else {
        for (int i = 0 ; i < size ; i++){
            if (visited[i] == 0){                             // Check if the route is already visited
                path[idx] = i;                                // Record the order of visiting
                visited[i] = 1;                               // Mark as visited
                length += cities[path[idx-1]][i];             // Add length
                _travel(idx+1);                               // Move to the next city
                length -= cities[path[idx-1]][i];             // Restore length to before visiting the city
                visited[i] = 0;                               // Reset the marking
            }       
        }
    }
}

void subtaskMaker (int idx, int childNumLimit){     
    if (idx == size-MAX_SUBTASK){ 
        pid = fork();
        
        if (pid < 0){
            printf("Fork failed.\n");
            exit(0);
        }

        if (pid > 0){
            childNum++;
            if (childNum == childNumLimit){
                printf("============ Start Wainting ==============\n");
                wait(NULL);
                printf("============ Resume ==============\n");
            }
        }

        if (pid == 0){
            checkedRoute=0;
            min = -1;
            _travel(idx);
            child_proc();
            exit(0);
        }     
    }
    else {
        for (int i = 0 ; i < size ; i++){
            if (visited[i] == 0){                             // Check if the route is already visited
                path[idx] = i;                                // Record the order of visiting
                visited[i] = 1;                               // Mark as visited
                length += cities[path[idx-1]][i];             // Add length
                if (idx == 0) length = 0;
                subtaskMaker(idx+1, childNumLimit);           // Move to the next city
                length -= cities[path[idx-1]][i];             // Restore length to before visiting the city
                if (idx == 0) length = 0;
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
