#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <memory.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_SUBTASK 11
#define MAX_THREADS 8
#define MAX_CITIES 50

int cities[MAX_CITIES + 1][MAX_CITIES + 1]; // Map of city distance
int minPath[MAX_CITIES + 1] = {0};          // The best path for the shortest distance
int size;                                   // The total number of cities
int min = -1;                               // Store minimum distance of traversed route
int threadLimit;
int runningThread = 0;
long long totalRoute = 0;

pthread_t producer;
pthread_t consumer[MAX_THREADS];

long long checkedRoute[MAX_THREADS] = {0}; // Number of checked route by single process
int isProducerAlive = 0;

typedef struct
{
    pthread_mutex_t lock;
    int **elem;
    int capacity;
    int num;
    int front;
    int rear;
} stopped_prefix;

stopped_prefix *queue = 0x0;

void stopped_prefix_init(stopped_prefix *queue)
{
    pthread_mutex_init(&(queue->lock), 0x0);
    queue->capacity = 100;
    queue->elem = (int **)calloc(sizeof(int *), 100);
    queue->num = 0;
    queue->front = 0;
    queue->rear = 0;
}

void stopped_prefix_queue(stopped_prefix *queue, int *prefix)
{
    pthread_mutex_lock(&(queue->lock));

    (queue->elem)[queue->rear] = (int *)malloc(sizeof(prefix[0]) * size - MAX_SUBTASK);
    memcpy((queue->elem)[queue->rear], prefix, sizeof(prefix[0]) * size - MAX_SUBTASK);
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->num += 1;

    pthread_mutex_unlock(&(queue->lock));
}

int *stopped_prefix_dequeue(stopped_prefix *queue)
{
    int *r = 0x0;
    pthread_mutex_lock(&(queue->lock));

    r = queue->elem[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->num -= 1;

    pthread_mutex_unlock(&(queue->lock));

    return r;
}

typedef struct
{
    sem_t filled;
    sem_t empty;
    pthread_mutex_t lock;
    int **elem;
    int capacity;
    int num;
    int front;
    int rear;
} bounded_buffer;

bounded_buffer *buf = 0x0;

void bounded_buffer_init(bounded_buffer *buf, int capacity)
{
    sem_init(&(buf->filled), 0, 0);
    sem_init(&(buf->empty), 0, capacity);
    pthread_mutex_init(&(buf->lock), 0x0);
    buf->capacity = capacity;
    buf->elem = (int **)calloc(sizeof(int *), capacity);
    buf->num = 0;
    buf->front = 0;
    buf->rear = 0;
}

void bounded_buffer_queue(bounded_buffer *buf, int *msg, int idx)
{
    sem_wait(&(buf->empty));
    pthread_mutex_lock(&(buf->lock));

    (buf->elem)[buf->rear] = (int *)malloc(sizeof(msg[0]) * idx);
    memcpy((buf->elem)[buf->rear], msg, sizeof(msg[0]) * idx);
    buf->rear = (buf->rear + 1) % buf->capacity;
    buf->num += 1;

    pthread_mutex_unlock(&(buf->lock));
    sem_post(&(buf->filled));
}

int *bounded_buffer_dequeue(bounded_buffer *buf)
{
    int *r = 0x0;
    sem_wait(&(buf->filled));
    pthread_mutex_lock(&(buf->lock));

    r = buf->elem[buf->front];
    buf->front = (buf->front + 1) % buf->capacity;
    buf->num -= 1;

    pthread_mutex_unlock(&(buf->lock));
    sem_post(&(buf->empty));
    return r;
}

/* Read line number from given file to figure out the number N */
int getNcities(char *arg)
{
    FILE *fp = fopen(arg, "r");
    char temp[256];
    int line = 0;

    while (fgets(temp, 256, fp) != NULL)
    {
        line++;
    }

    fclose(fp);
    return line;
}

/* Print min distance, path and number of checked route */
void printResult()
{
    printf("\nThe shortest distance: %d\n", min);
    printf("Path: (");
    for (int i = 0; i < size; i++)
    {
        printf("%d ", minPath[i]);
    }
    printf("%d)\n", minPath[0]);
    printf("The number of checked route is %lld.\n", totalRoute);
}

/* Behavior when SIGINT invoked */
void sigintHandler()
{
    printResult();
    exit(0);
}

/* Recursively traverse all the possible routes and calculate the length */
void _travel(int idx, int *visited, int *path, int length, int tidx)
{
    if (idx == size)
    {
        path[idx] = path[0]; // Set route from last city to starting city.
        pthread_mutex_t lock;
        pthread_mutex_init(&lock, 0x0);

        length += cities[path[idx - 1]][path[idx]]; // Add the last city length
        pthread_mutex_lock(&(buf->lock));
        checkedRoute[tidx]++; // Number of routes that the child process traversed
        totalRoute++;
        pthread_mutex_unlock(&(buf->lock));
        if (min == -1 || min > length)
        {                                           // Check if the length of current permuation is the best
            min = length;                           // Set the best value
            memcpy(minPath, path, sizeof(minPath)); // Save the best path
        }
        length -= cities[path[idx - 1]][path[idx]]; // Remove the current city and return to try other permutation
    }
    else
    {
        for (int i = 0; i < size; i++)
        {
            if (visited[i] == 0)
            {                                                  // Check if the route is already visited
                path[idx] = i;                                 // Record the order of visiting
                visited[i] = 1;                                // Mark as visited
                length += cities[path[idx - 1]][i];            // Add length
                _travel(idx + 1, visited, path, length, tidx); // Move to the next city
                length -= cities[path[idx - 1]][i];            // Restore length to before visiting the city
                visited[i] = 0;                                // Reset the marking
            }
        }
    }
}

/* Create subtasks, create child process and assign the tasks to child process */
void subtaskMaker(int idx, int size, int *visited, int *path)
{
    /* When prefix of the substask is created. */
    if (idx == size - MAX_SUBTASK)
    {
        bounded_buffer_queue(buf, path, idx);
    }
    else
    {
        for (int i = 0; i < size; i++)
        {
            if (visited[i] == 0)
            {                                               // Check if the route is already visited
                path[idx] = i;                              // Record the order of visiting
                visited[i] = 1;                             // Mark as visited
                subtaskMaker(idx + 1, size, visited, path); // Move to the next city
                visited[i] = 0;                             // Reset the marking
            }
        }
    }
}

void *producer_func(void *ptr)
{
    isProducerAlive = 1;
    int path[51] = {0};
    int visited[51] = {0};
    subtaskMaker(0, size, visited, path);
    isProducerAlive = 0;
    return 0x0;
}

void cleanup_handler(void *arg)
{
    printf("handler on\n");
    int prefix[size - MAX_SUBTASK];
    memcpy(prefix, arg, sizeof(prefix));

    stopped_prefix_queue(queue, prefix);
}

void *consumer_func(void *ptr)
{
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    runningThread++;
    int idx = *(int *)ptr;
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, 0x0);
    checkedRoute[idx] = 0;
    while (1)
    {
        int *prefix;
        int path[51] = {0};
        int visited[51] = {0};
        int length = 0;
        pthread_mutex_lock(&lock);
        if (queue->num > 0)
        {
            prefix = stopped_prefix_dequeue(queue);
            pthread_mutex_unlock(&lock);
        }
        else
        {
            if (!isProducerAlive && buf->num == 0)
            {
                pthread_mutex_unlock(&(buf->lock));
                prinf("break\n");
                break;
            }
            pthread_mutex_unlock(&lock);
            prefix = bounded_buffer_dequeue(buf);
        }
        pthread_cleanup_push(cleanup_handler, prefix);

        for (int i = 0; i < size - MAX_SUBTASK; i++)
        {
            visited[prefix[i]] = 1;
            path[i] = prefix[i];
        }

        _travel(size - MAX_SUBTASK, visited, path, length, idx);
        pthread_cleanup_pop(0);
    }
    runningThread--;
    return 0x0;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigintHandler);
    FILE *fp = fopen(argv[1], "r");
    threadLimit = atoi(argv[2]); // Limit number of child process

    buf = malloc(sizeof(bounded_buffer));
    queue = malloc(sizeof(stopped_prefix));
    bounded_buffer_init(buf, 8);
    stopped_prefix_init(queue);

    /* Get number of cities */
    size = getNcities(argv[1]);

    /* Put the length value into array from given tsp file */
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            fscanf(fp, "%d", &cities[i][j]);
        }
    }
    fclose(fp);

    pthread_create(&producer, 0x0, producer_func, 0x0);
    for (int i = 0; i < threadLimit; i++)
    {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&(consumer[i]), 0x0, consumer_func, arg);
    }

    while (runningThread > 0)
    {
        char op[10];
        printf("input option(stat, threads, num N): ");
        scanf("%s", op);

        if (op[0] == 's')
        {
            printResult();
        }
        else if (op[0] == 't')
        {
            for (int i = 0; i < threadLimit; i++)
            {
                printf("tid : %lu | # checked route : %lld\n", consumer[i], checkedRoute[i]);
            }
        }
        else if (op[0] == 'n')
        {
            int newN;
            scanf("%d", &newN);
            if (threadLimit < newN)
            {
                for (int i = threadLimit; i < newN; i++)
                {
                    int *arg = malloc(sizeof(*arg));
                    *arg = i;
                    pthread_create(&(consumer[i]), 0x0, consumer_func, arg);
                }
            }
            else if (threadLimit > newN)
            {
                for (int i = threadLimit - 1; i >= newN; i--)
                {
                    printf("cancel\n");
                    pthread_cancel(consumer[i]);
                }
            }
            threadLimit = newN;
        }
    }

    pthread_join(producer, 0x0);
    for (int i = 0; i < threadLimit; i++)
    {
        pthread_join(consumer[i], 0x0);
    }

    printResult();
    return 0;
}