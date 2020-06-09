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
	Node *ptr = edges[s]->next;
	while (ptr != NULL)
	{
		int idx = getIndex(ptr);
		//		printf("iudx: %d %d\n", idx, ptr->vertex);
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

void printg()
{
	for (int i = 0; i < edgeCount; i++)
	{
		Node *ptr = edges[i];

		while (ptr != NULL)
		{
			ptr->lockAddress != NULL ? printf("%p->", ptr->lockAddress) : printf("%ld->", ptr->tid);
			ptr = ptr->next;
		}
		printf("NULL \n");
	}
}

int detectCycle()
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
		printf("====DEADLOCK DETECTED====\n");
		printf("Below threads and locks are involved in the deadlock \n");
		printg();
		printf("====\n");
		char *command = malloc(sizeof(char) * 128);
		strcpy(command, "addr2line -e ");
		strcat(command, target);
		strcat(command, addr);

		FILE *fp = NULL;
		fp = popen(command, "r");
		if (!fp)
			printf("error to print line number");

		char result[128];
		fgets(result, 128, fp);
		printf("Faulty line: %s\n", result);

		deadlockCount = 0;
		free(command);

//		exit(0);
		return 1;
	}
	return 0;
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
	//	printg();
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
			int tempT = edges[i]->tid;
			releaseRequestEdge(edges[i]->tid, edges[i]->next->lockAddress);
			assignmentEdge(tempT, R);
			break;
		}
	}

	detectCycle();
	//	printg();
}

int main(int argc, char *argv[])
{
	int cnt = 0;
	int fd = open(".ddtrace", O_RDONLY | O_SYNC);
	while (1)
	{
		uli tid;
		int *lid;

		char buf[128];
		int len = 0;
		int op;

		if ((len = read(fd, buf, 128)) == -1)
			break;
		if (len > 0)
		{
			sscanf(buf, "%d %lu %p", &op, &tid, &lid);
			printf("%d %lu %p\n", op, tid, lid);
			if (op == 2)
			{
				int stackTopFlag = 0;
				for (int i = 0; i < tid; i++)
				{
					read(fd, buf, 128);
					if (stackTopFlag == 0)
					{
						sscanf(buf, "%s %s", target, addr);
						printf("%s %s\n", target, addr);
						if (!strncmp(argv[1], target, strlen(argv[1])))
						{
							for (int i = 0; i < strlen(addr) - 1; i++)
							{
								addr[i] = addr[i + 1];
							}
							target[strlen(target) - 2] = ' ';
							target[strlen(target) - 1] = '\0';
							addr[strlen(addr) - 2] = '\0';
							stackTopFlag = 1;
						}
					}
				}
			}
			else if (op == 0)
			{
				assignmentEdge(tid, lid);
				printg();
				printf("\n");
			}
			else if (op == 3)
			{
				releaseAssignmentEdge(tid, lid);
				printg();
				printf("\n");
				cnt++;
				printf("cnt: %d\n", cnt);
			}
		}
	}
	close(fd);
	return 0;
}
