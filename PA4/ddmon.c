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

#define gettid() syscall(SYS_gettid)

static __thread int n_malloc = 0;
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

	n_malloc++;
	if(n_malloc == 1 && n_unlock == 0){
		pthread_mutex_lock(&lock);
        	int fd = open(".ddtrace", O_WRONLY | O_SYNC);	
		char buf [128];
		sprintf(buf, "0 %lu %p", pthread_self(), mutex);
		write(fd, buf, 128);
		printf("ddmon: %s\n",buf);

		void *arr[10];
		char **stack;
		size_t sz = backtrace(arr, 10);
		stack = backtrace_symbols(arr, sz);
		char tot_size[128];
		sprintf(tot_size, "2 %lu", sz);
		write(fd, tot_size,128);

		for (int i = 0; i < sz; i++){
			sprintf(tot_size, "%s", stack[i]);
			write(fd, tot_size, 128);
		}

		close(fd);
	
		pthread_mutex_unlock(&lock);
	}
	
	pthread_lock_orig = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	n_malloc--;
	int r = pthread_lock_orig(mutex);
	return r;
}


int pthread_mutex_unlock(pthread_mutex_t *mutex){	
	int (*pthread_unlock_orig)(pthread_mutex_t *mutex);
	pthread_unlock_orig = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	n_unlock++;	
	if(n_malloc == 0 && n_unlock == 1){
	pthread_mutex_lock(&unlock);
		char buf [128];	
		int fd = open(".ddtrace", O_WRONLY | O_SYNC) ;
		sprintf(buf, "3 %lu %p", pthread_self(), mutex);
		write(fd, buf, 128);
		printf("ddmon: %s\n",buf);
		close(fd);
	pthread_mutex_unlock(&unlock);
	}
	n_unlock--;
	return pthread_unlock_orig(mutex);
}
