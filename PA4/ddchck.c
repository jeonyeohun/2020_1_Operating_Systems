#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dlfcn.h>
#include <execinfo.h>

typedef struct node{
	unsigned long int ownerThread;
	int* lockID;
	struct node* next;
}Node;

Node* nodeList [10] = {0x0};

Node* deadlockList[10];
int deadlockcount= 0;
int visited[10] = {0};
int done[10] = {0};
int nodeCount = 0;
int targetIdx = -1;
int cycle = 0;

char target[128];
char addr[128];
	

int isNew(unsigned long int tid){
	for (int i = 0 ; i < nodeCount ; i++){
		if (nodeList[i]->ownerThread == tid){ 
			return 0;
		}
	}
	return 1;
}

void printg(){
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



int searchNode(int *lid ){
	for (int i = 0 ; i < nodeCount ; i++){
		if (nodeList[i]->lockID == lid) return i;
	}
	return -1;	
}

int searchThread(unsigned long int tid){
	for (int i = 0 ; i < nodeCount ; i++){
		if (nodeList[i]->ownerThread == tid) return i;
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

void addEdge(unsigned long int tid, int* lid){	
	Node* curr = (Node*)malloc(sizeof(Node));
	curr->ownerThread = tid;
	curr->lockID = lid;
	curr->next = NULL;
	
	targetIdx = searchThread(tid);	
	Node* ptr;
	ptr =  nodeList[targetIdx];
	
	while(ptr->next != NULL){
		ptr = ptr->next;
	}	
	
	ptr->next = curr;

}

void  dfs(int s){
	visited[s] = 1;
	Node* ptr = nodeList[s];
	deadlockList[deadlockcount++] = nodeList[s];
	while(ptr->next != NULL){
		int idx = searchNode(ptr->next->lockID);
		if(idx != -1){
			if(!visited[idx]) dfs(idx);
			else if(!done[idx]) cycle = 1;
		}
		ptr = ptr->next;
	}
	done[s] = 1;

}

int isCycle(unsigned long int tid, int* lid){
	for (int i = 0 ; i < 10 ; i++){
		visited[i] = 0;
		done[i] = 0;
	}	
	for (int i = 0 ; i < nodeCount ; i++){
		cycle = 0;
		dfs(i);
		
		if(cycle) return i;
		deadlockcount = 0;	
	}

	return -1;
}


void lock_behavior(unsigned long int tid, int* lid){
	if(isNew(tid)) {
		addNode(tid, lid);
	}
	
	else {
		int s;
		addEdge(tid, lid);
		if((s = isCycle(tid, lid)) >= 0){
			printf("====DEADLOCK DETECTED====\n");
			printf("Below threads and locks are involved in the deadlock \n");
			for(int i = 0 ;i < deadlockcount ;i++){
				printf("[%d] Thread: %lu  Lock Address: %p\n",i+1, deadlockList[i]->ownerThread,  deadlockList[i]->lockID);
			}
			
			char * command = malloc(sizeof(char) * 128);
			strcpy(command, "addr2line -e ");
			strcat(command, target);
			strcat(command, addr);

			FILE *fp = NULL;
			fp = popen(command, "r");
			if(!fp) printf("error to print line number");	
			
			char result[128];
			fgets(result, 128, fp);
			printf("Faulty line: %s\n", result);

			deadlockcount = 0;
			free(command);
		}
 
	}
}

void unlock_behavior(unsigned long int tid, int* lid){
	for(int i = 0 ; i < nodeCount ; i++){
		if(nodeList[i]->lockID == lid && nodeCount != i){
			for(int j = i ; j < nodeCount-1 ; j++){
				nodeList[j] = nodeList[j+1];
			
			}
			nodeCount--;

		}
		else if(nodeCount == i) nodeCount--;
	}

	for(int i = 0 ; i < nodeCount ; i++){
		Node* curr = nodeList[i]->next;
		Node* prev = nodeList[i];
//		printf("1\n");
		if(curr == NULL) continue;
		if(curr->lockID == lid) {
			prev->next = curr->next;
			continue;
		}	
		while(curr != NULL && curr->lockID != lid){
			curr = curr->next;
		}
		if(curr != NULL){
			while(prev->next != curr){
				prev = prev->next;
			}
		}
		if(curr == NULL) prev->next = NULL;
		else prev->next = curr->next;
//		printg();
		free(curr);
	}
}

int main (int argc, char* argv[]) {
	int fd = open(".ddtrace", O_RDONLY | O_SYNC);
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
//			if(op!=2) printf("%d %lu %p\n", op, tid, lid);
			if (op == 0) {
				lock_behavior(tid, lid);
//				printg();printf("\n");
			}
			else if(op == 1){
				unlock_behavior(tid, lid);
//				printg();printf("\n");
			}
			else if(op == 2){
				int stackTopFlag = 0;
				for (int i = 0 ; i < tid ; i++){
					read(fd, buf, 128);
					if(stackTopFlag == 0){
						sscanf(buf, "%s %s", target, addr);
						if(!strncmp(argv[1], target, strlen(argv[1]))){
							for(int i = 0 ; i< strlen(addr)-1 ; i++){
								addr[i] = addr[i+1];
							}
							target[strlen(target)-2] = ' ';
							target[strlen(target)-1] = '\0';
							addr[strlen(addr)-2] = '\0';
							stackTopFlag = 1;
						}
					}
						
				}
//				printg();printf("\n");
			}

		}
//		printg(); 
	}
	close(fd) ;
	return 0 ;
}
