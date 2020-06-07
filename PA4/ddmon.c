#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <string.h>

static __thread int n_malloc = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;

int pthread_mutex_lock(pthread_mutex_t *mutex){
	int (*pthread_lock_orig)(pthread_mutex_t *mutex);
	if (mkfifo("channel", 0666)) {
		if (errno != EEXIST) {
			perror("fail to open fifo: ") ;
			exit(1) ;
		}
	}

	n_malloc++;
	if(n_malloc == 1){
		pthread_mutex_lock(&lock);
        	int fd = open("channel", O_WRONLY | O_SYNC);	
		char buf [128];
		sprintf(buf, "0 %lu %p\n", pthread_self(), mutex);
		write(fd, buf, 128);
		close(fd);		
		pthread_mutex_unlock(&lock);

		void *arr[10];
		char **stack;
		printf("write done\n");			
		size_t sz = backtrace(arr, 10);
		stack = backtrace_symbols(arr, sz);
		printf("backtrace\n");
		for (int i = 0; i < sz; i++)
			printf("[%d] %s\n", i, stack[i]);


	}
	
	pthread_lock_orig = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	n_malloc--;
	return pthread_lock_orig(mutex);
}


int pthread_mutex_unlock(pthread_mutex_t *mutex){	
	int (*pthread_unlock_orig)(pthread_mutex_t *mutex);
	pthread_unlock_orig = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	
	if(n_malloc == 0){
		char buf [128];	
		int fd = open("channel", O_WRONLY | O_SYNC) ;
		sprintf(buf, "1 %lu %p", pthread_self(), mutex);
		write(fd, buf, 128);
		close(fd);
	}

	int result = pthread_unlock_orig(mutex);
	return result;
}
