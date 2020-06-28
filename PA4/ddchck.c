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

#define uli unsigned long int

typedef struct Node
{
	int *lockAddress;
	uli tid;
	struct Node *next;
} Node;

Node *edges[20];
Node *deadlockList[20];
int edgeCount = 0;
int deadlockCount = 0;

int cycle = 0;
char target[128];
char addr[128];

int searchLock(int *lock)
{
	for (int i = 0; i < edgeCount; i++)
	{
		if (edges[i]->lockAddress == lock)
			return i;
	}
	return -1;
}

int searchThread(uli tid)
{
	for (int i = 0; i < edgeCount; i++)
	{
		if (edges[i]->tid == tid)
			return i;
	}
	return -1;
}

int getIndex(Node *ptr)
{
	return (ptr->lockAddress != NULL) ? searchLock(ptr->lockAddress) : searchThread(ptr->tid);
}

void dfs(int s, int *discovered, int *finished)
{
	discovered[s] = 1;
	deadlockList[deadlockCount++] = edges[s];
	Node *ptr = edges[s]->next;
	while (ptr != NULL)
	{
		int idx = getIndex(ptr);
		if (idx != -1)
		{
			if (!discovered[idx])
				dfs(idx, discovered, finished);
			else if (!finished[idx])
				cycle = 1;
		}
		ptr = ptr->next;
	}
	finished[s] = 1;
}

void detectCycle()
{
	for (int i = 0; i < edgeCount; i++)
	{
		int discovered[20] = {
			0,
		};
		int finished[20] = {
			0,
		};
		cycle = 0;
		dfs(i, discovered, finished);
		if (cycle)
			break;
		deadlockCount = 0;
	}

	if (cycle)
	{
		int tcount = 1;
		int mcount = 1;
		printf("===============  DEADLOCK DETECTED  ================\n");
		printf("Below threads and locks are involved in the deadlock\n");
		printf("____________________________________________________\n");
		for (int i = 0; i < deadlockCount; i++)
		{
			if (edges[i]->lockAddress == NULL)
			{
				printf("Thread[%d]: %lu\n", tcount++, edges[i]->tid);
			}
		}
		for (int i = 0; i < deadlockCount; i++)
		{
			if (edges[i]->tid == -1)
			{
				printf("Mutex[%d]: %p\n", mcount++, edges[i]->lockAddress);
			}
		}

		printf("====================================================\n");
		char *command = malloc(sizeof(char) * 128);
		strcpy(command, "addr2line -e ");
		strcat(command, target);
		strcat(command, " ");
		strcat(command, addr);

		FILE *fp = NULL;
		fp = popen(command, "r");
		if (!fp)
			printf("error to print line number");

		char result[128];
		fgets(result, 128, fp);
		printf("Faulty Line Number: %s\n", result);

		deadlockCount = 0;
		free(command);
	}
}

/* delete T-R Edge */
void releaseRequestEdge(uli T, int *R)
{
	int idx = searchThread(T);

	Node *head = edges[idx];
	head->next = head->next->next;

	if (head->next == NULL)
	{
		for (int i = idx; i < edgeCount - 1; i++)
		{
			edges[i] = edges[i + 1];
		}
		edgeCount--;
	}
}

/* T-R Edge */
void requestEdge(uli T, int *R)
{
	Node *nodeR = (Node *)malloc(sizeof(Node));
	nodeR->lockAddress = R;
	nodeR->tid = -1;
	nodeR->next = NULL;

	int idx = searchThread(T);

	if (idx < 0)
	{
		Node *nodeT = (Node *)malloc(sizeof(Node));
		nodeT->tid = T;
		nodeT->lockAddress = NULL;
		nodeT->next = nodeR;

		edges[edgeCount++] = nodeT;
		return;
	}

	Node *ptr = edges[idx];
	while (ptr->next != NULL)
	{
		ptr = ptr->next;
	}
	ptr->next = nodeR;
	detectCycle();
}

/* R-T Edge */
void assignmentEdge(uli T, int *R)
{
	if (searchLock(R) != -1)
	{
		requestEdge(T, R);
		detectCycle();
		return;
	}
	Node *nodeT = (Node *)malloc(sizeof(Node));
	nodeT->tid = T;
	nodeT->lockAddress = NULL;
	nodeT->next = NULL;

	Node *nodeR = (Node *)malloc(sizeof(Node));
	nodeR->lockAddress = R;
	nodeR->tid = -1;
	nodeR->next = nodeT;

	edges[edgeCount++] = nodeR;

	detectCycle();
}

/* delete R-T Edge */
void releaseAssignmentEdge(uli T, int *R)
{
	int idx = searchLock(R);

	for (int i = idx; i < edgeCount - 1; i++)
	{
		edges[i] = edges[i + 1];
	}
	edgeCount--;

	for (int i = 0; i < edgeCount; i++)
	{
		if (edges[i]->next->lockAddress == R)
		{
			uli tempT = edges[i]->tid;
			releaseRequestEdge(edges[i]->tid, edges[i]->next->lockAddress);
			assignmentEdge(tempT, R);
			break;
		}
	}
	detectCycle();
}

void processString(char *btrace, char *filename)
{
	char *ptr;
	ptr = strstr(btrace, filename);
	strncpy(target, ptr, strlen(filename));
	ptr = strstr(ptr, "[");
	char temp[20];
	int i = 0;
	ptr = ptr + 1;
	while (*ptr != ']')
	{
		temp[i++] = *ptr;
		ptr++;
	}
	temp[i] = '\0';
	strcpy(addr, temp);
}

int main(int argc, char *argv[])
{
	int cnt = 0;
	int fd = open(".ddtrace", O_RDONLY | O_SYNC);
	while (1)
	{
		uli tid;
		int *lid;
		char btrace[512];

		char buf[512];
		int len = 0;
		int op;

		if ((len = read(fd, buf, 512)) == -1)
			break;
		if (len > 0)
		{
			sscanf(buf, "%d", &op);
			switch (op)
			{
			case 0:
				sscanf(buf, "%d %lu %p", &op, &tid, &lid);
				assignmentEdge(tid, lid);
				break;
			case 1:
				sscanf(buf, "%d %lu %p", &op, &tid, &lid);
				releaseAssignmentEdge(tid, lid);
				break;
			case 2:
				strcpy(btrace, buf);
				processString(btrace, argv[1]);
				break;
			}
		}
	}
	close(fd);
	return 0;
}
