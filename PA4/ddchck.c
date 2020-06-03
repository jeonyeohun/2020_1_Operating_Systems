#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>



typedef struct node{
	unsigned long int ownerThread;
	int* lockID;
	struct node* next;
}Node;

Node* nodeList [10] = {0x0};
int visited[10] = {0};

int nodeCount = 0;
int targetIdx;

int isNew(int* lid){
	for (int i = 0 ; i < nodeCount ; i++){
		if (nodeList[i]->lockID == lid){ 
			targetIdx = i;
			return 0;
		}
	}
	return 1;
}

void addNode(unsigned long int tid, int* lid){
	Node *head = (Node*)malloc(sizeof(Node));
	head->ownerThread = tid;
	head->lockID = lid;
	head->next = NULL;
	
	nodeList[nodeCount] = head;
	nodeCount++;
}

void addEdge(unsigned long int tid, int* lid){
	Node* curr = (Node*)malloc(sizeof(Node));
	curr->ownerThread = tid;
	curr->lockID = lid;
	curr->next = NULL;

	Node* ptr = nodeList[targetIdx];

	while(ptr->next != NULL){
		ptr = ptr->next;
	}
	
	ptr->next = curr;
}

void deleteNode(unsigned long int tid, int* lid){
}

int dfs(int s){
	visited[s]++;

}

int isCycle(unsigned long int tid, int* lid){
	for (int i = 0 ; i < nodeCount ; i++){
		if(dfs(s) == 1) return 1;	
	}
	return 0;
}


void lock_behavior(unsigned long int tid, int* lid){
	if(isNew(lid)) addNode(tid, lid);
	else addEdge(tid, lid);
}

void unlock_behavior(unsigned long int tid, int* lid){

}

int main () {
	int fd = open("channel", O_RDONLY | O_SYNC);
	while (1) {
		unsigned long int tid;
		int * lid;
		char buf[128];
		int len =0;
		int op;
		if ((len = read(fd, buf, 128)) == -1)
			break ;
		if (len > 0){
			sscanf(buf, "%d %lu %p", &op, &tid, &lid);	
			if (op == 0) {
				lock_behavior(tid, lid);
			}
			else{
				deleteNode(tid, lid);
			}
 			
		} 
	}
	close(fd) ;
	return 0 ;
}
