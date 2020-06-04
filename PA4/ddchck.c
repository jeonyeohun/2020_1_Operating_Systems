#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

typedef struct node{
	unsigned long int ownerThread;
	int* lockID;
	struct node* next;
}Node;

Node* nodeList [10] = {0x0};

int visited[10] = {0};
int nodeCount = 0;
int targetIdx = -1;

int isNew(int* lid){
	for (int i = 0 ; i < nodeCount ; i++){
		if (nodeList[i]->lockID == lid){ 
			return 0;
		}
	}
	return 1;
}

int searchNode(int *lid ){
	for (int i = 0 ; i < nodeCount ; i++){
		if (nodeList[i]->lockID == lid) return i;
	}
	return -1;	
}

void addNode(unsigned long int tid, int* lid){
	Node *head = (Node*)malloc(sizeof(Node));
	head->ownerThread = tid;
	head->lockID = lid;
	head->next = NULL;
	
	nodeList[nodeCount] = head;
	nodeCount++;
}

int searchThread(unsigned long int tid){
	for (int i = 0 ; i < nodeCount ; i++){
		if (nodeList[i]->ownerThread == tid) return i;
	}
	return -1;
}

void addEdge(unsigned long int tid, int* lid){
	Node* curr = (Node*)malloc(sizeof(Node));
	curr->ownerThread = tid;
	curr->lockID = lid;
	curr->next = NULL;

	if((targetIdx = searchThread(tid)) > 0){
		Node* ptr;
		ptr =  nodeList[targetIdx];
	
		while(ptr->next != NULL){
			ptr = ptr->next;
		}	
		
		ptr->next = curr;
	}
}

void deleteNode(unsigned long int tid, int* lid){
	
}


int dfs(int s){
	visited[s]++;
	if(visited[s] > 1) return 1;
	Node* ptr = nodeList[s];

	while(ptr != NULL){
		int idx = searchNode(ptr->next->lockID);
		dfs(idx);
		ptr = ptr->next;
	}
	return 0;

}

int isCycle(unsigned long int tid, int* lid){
	for (int i = 0 ; i < 10 ; i++){
		visited[i] = 0;
	}	
	for (int i = 0 ; i < nodeCount ; i++){
		if(dfs(i) == 1) return 1;	
	}

	return 0;
}


void lock_behavior(unsigned long int tid, int* lid){
	if(isNew(lid)) addNode(tid, lid);
	else {
		addEdge(tid, lid);
		if(isCycle(tid, lid)) printf("deadlock\n");
	}
}

void unlock_behavior(unsigned long int tid, int* lid){
	int delIdx;
	for (int i = 0 ; i < nodeCount ; i++){
		if(nodeList[i]->lockID == lid){
			delIdx = i;	
		}
	}
	for (int i = delIdx ; i < nodeCount-1 ; i++){
		nodeList[i] = nodeList[i+1];
	}
	nodeCount--;

	for(int i = 0 ; i < nodeCount ; i++){
		Node* curr = nodeList[i];
		Node* prev = nodeList[i];
		while(curr != NULL && curr->next->lockID != lid ){
			curr = curr->next;
		}
		
		if(curr != NULL){
			while(prev->next == curr){
				prev = prev->next;
			}
		}

		prev->next = curr->next;
		free(curr);
	}
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
//				unlock_behavior(tid, lid);
			}
			printf("\nop: %d tid: %lu, lid: %p\n", op, tid, lid);
			for(int i = 0 ; i < nodeCount ; i++){
				printf("%p(%lu) -> ", nodeList[i]->lockID, nodeList[i]->ownerThread);
				Node* ptr = nodeList[i]->next;
				while(ptr != NULL){
					printf("%p(%lu) -> ", ptr->lockID, ptr->ownerThread);
					ptr = ptr->next;
				}
				printf("NULL\n");
			}	
		} 
	}
	close(fd) ;
	return 0 ;
}
