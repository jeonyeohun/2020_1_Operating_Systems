#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

pthread_mutex_t	x = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t y = PTHREAD_MUTEX_INITIALIZER ;


void *print_message_function1 (void * ptr)
{
	char *message;
	message = (char *) ptr;

		pthread_mutex_lock(&x) ;
		printf("Thread 1 gets lock x\n");
		pthread_mutex_lock(&y) ;
		printf("Thread 1 gets lock y\n");


		printf("%s\n", message) ;


		pthread_mutex_unlock(&y) ;
		printf("Thread 1 release lock y\n");
		pthread_mutex_unlock(&x) ;
		printf("Thread 1 release lock x\n");
}

void *print_message_function2 (void * ptr)
{
	char *message;
	message = (char *) ptr;

	int i = 0 ;
		pthread_mutex_lock(&y) ;
		printf("Thread 2 gets lock y\n");
		pthread_mutex_lock(&x) ;
		printf("Thread 2 gets lock x\n");
	

		printf("%s\n", message) ;

		pthread_mutex_unlock(&x) ;
		printf("Thread 2 releases lock x\n");
		pthread_mutex_unlock(&y) ;
		printf("Thread 2 releases lock y\n");
}

int
main ()
{
	pthread_t thread1, thread2;

	char *message1 = "Thread 1 is in CS";
	char *message2 = "Thread 2 is in CS";

	printf("Start Program\n\n");
	pthread_create(&thread1, NULL, print_message_function1, (void*) message1);
	sleep(0.2);
	pthread_create(&thread2, NULL, print_message_function2, (void*) message2);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL); 

	exit(0);
}


