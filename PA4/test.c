#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

pthread_mutex_t	x = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t y = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t z = PTHREAD_MUTEX_INITIALIZER ;

void *print_message_function1 (void * ptr)
{
	char *message;
	message = (char *) ptr;
		printf("Thread 1 calls lock x\n");
		pthread_mutex_lock(&y) ;
		printf("Thread 1 gets lock x(%p)\n", &x);
		printf("Thread 1 calls lock y\n");
		pthread_mutex_lock(&x) ;
		printf("Thread 1 gets lock y(%p)\n", &y);
		pthread_mutex_lock(&z);
		printf("%s\n", message) ;

		pthread_mutex_unlock(&z);
		pthread_mutex_unlock(&x) ;
		printf("Thread 1 release lock y(%p)\n", &y);
		pthread_mutex_unlock(&x) ;
		printf("Thread 1 release lock x(%p)\n", &x);
}

void *print_message_function2 (void * ptr)
{
	char *message;
	message = (char *) ptr;

	int i = 0 ;
		printf("Thread 2 calls lock y\n");
		pthread_mutex_lock(&x) ;
		printf("Thread 2 gets lock y(%p)\n", &y);
		printf("Thread 2 calls lock x\n");
		pthread_mutex_lock(&z) ;
		printf("Thread 2 gets lock x(%p)\n", &x);
		pthread_mutex_lock(&y);

		printf("%s\n", message) ;
		pthread_mutex_unlock(&y);
		pthread_mutex_unlock(&z) ;
		printf("Thread 2 releases lock x(%p)\n", &x);
		pthread_mutex_unlock(&x) ;
		printf("Thread 2 releases lock y(%p)\n", &y);
}

int
main ()
{
	pthread_t thread1, thread2;

	char *message1 = "Thread 1 is in CS";
	char *message2 = "Thread 2 is in CS";

	printf("Start Program\n\n");
	pthread_create(&thread1, NULL, print_message_function1, (void*) message1);
//	sleep(3);
	pthread_create(&thread2, NULL, print_message_function2, (void*) message2);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL); 

	exit(0);
}

