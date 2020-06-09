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
#include <sys/syscall.h>

#define BUFF_MAX 512 

static __thread int n_lock = 0;
static __thread int n_unlock = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;
pthread_mutex_t unlock = PTHREAD_MUTEX_INITIALIZER ;
int pthread_mutex_lock(pthread_mutex_t *mutex){
	int (*pthread_lock_orig)(pthread_mutex_t *mutex);
	if (mkfifo(".ddtrace", 0666)) {
		if (errno != EEXIST) {
			perror("fail to open fifo: ") ;
			exit(1) ;
		}
	}

	n_lock++;
	if(n_lock == 1 && n_unlock == 0){
	pthread_mutex_lock(&lock);
        	int fd = open(".ddtrace", O_WRONLY | O_SYNC);	
		char buf [BUFF_MAX];
		sprintf(buf, "0 %lu %p", pthread_self(), mutex);
		write(fd, buf, BUFF_MAX);

		void *arr[10];
		char **stack;
		size_t sz = backtrace(arr, 10);
		stack = backtrace_symbols(arr, sz);
		
		char result[BUFF_MAX]= "2 ";
		for (int i = 0; i < sz; i++){
			strcat(result, stack[i]);
			strcat(result, "\n");		
		}
		write(fd, result, BUFF_MAX);

		close(fd);
	pthread_mutex_unlock(&lock);
	}
	
	pthread_lock_orig = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	n_lock--;
	int r = pthread_lock_orig(mutex);
	return r;
}


int pthread_mutex_unlock(pthread_mutex_t *mutex){	
	int (*pthread_unlock_orig)(pthread_mutex_t *mutex);
	pthread_unlock_orig = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	n_unlock++;
	if(n_lock == 0 && n_unlock ==1){
	pthread_mutex_lock(&lock);
		char buf [BUFF_MAX];	
		int fd = open(".ddtrace", O_WRONLY | O_SYNC) ;
		sprintf(buf, "1 %lu %p", pthread_self(), mutex);
		write(fd, buf, BUFF_MAX);
		close(fd);
	pthread_mutex_unlock(&lock);
	}
	n_unlock--;

	return pthread_unlock_orig(mutex);
}
