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

int parent [10] = {0,};
int nodeList_len = 0;
int level[10] = {0,};
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


typedef struct node{
	unsigned long int ownerThread;
	int* lockID;
}Node;
Node nodeList [10] = {0x0};

int find(int child){
	if(child == parent[child]) return child;
	return parent[child] = find(parent[child]);
}

int merge(int a, int b){
	a = find(a);
	b = find(b);

	if(a == b) return 1;

	if (level[a] < level[b]) parent[b] = a;
	else parent[a] = b;

	if (level[a] == level[b]) level[b]++;

	return 0;
}


void addNode(unsigned long int tid, int* lid){
	for (int i = 0 ; i < nodeList_len  ; i++){
		if (nodeList[i].lockID == lid) {
			if(nodeList[i].ownerThread == 0) 
				nodeList[i].ownerThread = tid;
			return;
		}	
	}
	nodeList[nodeList_len].ownerThread = tid;
	nodeList[nodeList_len].lockID = lid;
	nodeList_len++;
}


void deleteNode(unsigned long int tid, int* lid){
	for (int i = 0 ; i < nodeList_len ; i++){
		if (nodeList[i].lockID == lid){
			parent[i] = i;
			nodeList[i].ownerThread = 0;
		}
	}
}

int isHold(unsigned long int tid, int* lid){
	for (int i = 0 ; i < nodeList_len ; i++){
		if (nodeList[i].ownerThread == tid && nodeList[i].lockID != lid) {
			return i;
		}
	}
	return -1;
}



void lock_behavior(unsigned long int tid, int* lid){
	int target;
	int cur;
	addNode(tid, lid);
	
	for (int i = 0 ; i < nodeList_len ;i++){
		if (nodeList[i].lockID == lid){
			target = i;
			break;
		}
	}
	
	if ((cur = isHold(tid, lid)) != -1){
		if(merge(cur, target)){
			printf("==== DEADLOCK DETECTED =====\n");
			printf("Below threads and locks are involved in this deadlock\n");
			int idx = 1;
			for (int i = 0 ; i < nodeList_len ; i++){
				if(parent[cur] == parent[i]){
					printf("[%d] Thread ID: %lu Lock Address: %p\n", idx++, nodeList[i].ownerThread, nodeList[i].lockID);
				}
			}
		
		}
	
	}	
}

int main () {
	for (int i = 0 ; i < 10 ; i++){
		parent[i] = i;
	}
	int fd = open("channel", O_RDONLY | O_SYNC);
	while (1) {
		unsigned long int tid;
		int * lid;
		char buf[128];
		int len =0;
		int op;
		pthread_mutex_lock(&lock);
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
		pthread_mutex_unlock(&lock);
	}
	close(fd) ;
	return 0 ;
}
