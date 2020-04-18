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
int childLimit;
int min = -1;
pid_t pid;                      // Get pid after calling fork()
int childNum = 0;               // Tracking the number of child process currently running

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

/* Recursively traverse all the possible routes and calculate the length */
void _travel (int idx){
    if (idx == size){                                // Traveling all the cities is done
        length += cities[path[size-1]][path[0]];     // Add the last city length 
        if (min == -1 || min > length){              // Check if the length of current permuation is the best
            min = length;                            // Set the best value
        } 
        length -= cities[path[size-1]][path[0]];     // Remove the current city and return to try other permutation
    }
    else {
        for (int i = 0 ; i < size ; i++){
            if (used[i] == 0){                       // Check if the route is already visited
                path[idx] = i;                       // Record the order of visiting
                used[i] = 1;                         // Mark as visited
                length += cities[path[idx-1]][i];    // Add length
                _travel(idx+1);                      // Move to the next city
                length -= cities[path[idx=1]][i];    // Restore length to before visiting the city
                used[i] = 0;                         // Reset the marking
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
}

int main (int argc, char* argv []){
    FILE * fp = fopen (argv[1], "r");
    childLimit = atoi(argv[2]);
    int i, j;

    size = getNcities(argv[1]);
    path = (int *) malloc (sizeof(int) * size);     // Allocate memory for path recorder with the number of cities
    used = (int *) malloc (sizeof(int) * size);     // Allocate memory for visi recorder with the number of cities

    /* Put the length value into array from given tsp file */
    for (i = 0 ; i < size ; i++) {
        for (j = 0 ; j < size ; j++) {
            fscanf(fp, "%d", &cities[i][j]) ;
        }
    }
    fclose(fp);

    

    /* Keep creating new child process */
    while(1){ 
        /* Stop making new child when the number of current child is full and make until any child terminates */
        if (childLimit == childNum){
            pid_t p = wait(NULL);
            printf("%d out\n", p);
            childNum--;                     // reduce the number of running process since child process is terminated and return code to wait().
        }
        /* Call fork() if there is space to make new child */   
        else{
            pid = fork();                   // create new child process. Save the result of forking.
            /* Fork fails */
            if (pid < 0){
                printf("Failed\n");
                exit(0);
            }
            /* Behavior of child process */
            else if (pid == 0){
                printf("%d in\n", getpid());
                sleep(10);
                exit(0);                  // exit() should be conducted. Otherwise, the child process keeps running in the infinite loop.
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
