#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int cities[51][51];
int path[51];
int visited[51] = {0,};
int childLimit;
int size;

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

void printPermu(int length){
	for (int i = 0 ; i < size ; i++) {
		printf("%d ", path[i]) ;
    }
	printf("%d) by %d\n", path[0], getpid()) ; 
}

void childAction(){}

void dfs (int idx){
    if (idx == size){
        printPermu(size);
        return;
    }

    for (int i = 1 ; i < size ; i++){
        if (!visited[i]){
            visited[i] = 1;
            path[idx] = i;
            dfs(idx+1);
            visited[i] = 0; 
        }

    }
}


int main (int argc, char* argv []){
    FILE * fp = fopen (argv[1], "r");
    childLimit = atoi(argv[2]);
    size = getNcities(argv[1]);

    printf("%d", size);

    /* Put the length value into array from given tsp file */
    for (int i = 0 ; i < size ; i++) {
        for (int j = 0 ; j < size ; j++) {
            fscanf(fp, "%d", &cities[i][j]) ;
        }
    }
    fclose(fp);

    dfs(1);

    return 0;
}