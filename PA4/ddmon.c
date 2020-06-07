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

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;

int pthread_mutex_lock(pthread_mutex_t *mutex){
	int (*pthread_lock_orig)(pthread_mutex_t *mutex);
	static __thread int n_malloc = 0;
	if (mkfifo("channel", 0666)) {
		if (errno != EEXIST) {
			perror("fail to open fifo: ") ;
			exit(1) ;
		}
	}
	
	n_malloc++;
	printf("== N: %d | pid: %lu | lock: %p \n", n_malloc, pthread_self(), mutex);
	if(n_malloc == 1){
		pthread_mutex_lock(&lock);
		printf("==== %lu in\n", pthread_self());
        	int fd = open("channel", O_WRONLY | O_SYNC);	
		char buf [128];
		sprintf(buf, "0 %lu %p\n", pthread_self(), mutex);
		printf("====== sprintf: %s\n", buf);
		write(fd, buf, 128);
		close(fd);		
   		
		printf("======== %lu out\n", pthread_self());
		pthread_mutex_unlock(&lock);

	}
	n_malloc--;
/*	if (n_malloc == 0){
		void *arr[10];
		char **stack;
		printf("write done\n");			
		size_t sz = backtrace(arr, 10);
		stack = backtrace_symbols(arr, sz);
		printf("backtrace\n");
		for (int i = 0; i < sz; i++)
			printf("[%d] %s\n", i, stack[i]);
	}		
*/	
	pthread_lock_orig = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	int result = pthread_lock_orig(mutex);
	return result;
}


int pthread_mutex_unlock(pthread_mutex_t *mutex){
	
	int (*pthread_unlock_orig)(pthread_mutex_t *mutex);
	pthread_unlock_orig = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	
	if(mutex != &lock){
	   pthread_mutex_lock(&lock);
		char buf [128];	
		int fd = open("channel", O_WRONLY | O_SYNC) ;
		sprintf(buf, "1 %lu %p", pthread_self(), mutex);
		write(fd, buf, 128);
		close(fd);
   	   pthread_mutex_unlock(&lock);
	}
	int result = pthread_unlock_orig(mutex);
	return result;
}
