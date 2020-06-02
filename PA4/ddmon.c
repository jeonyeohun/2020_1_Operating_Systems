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

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER ;

int pthread_mutex_lock(pthread_mutex_t *mutex){
	int (*pthread_lock_orig)(pthread_mutex_t *mutex);

	if (mkfifo("channel", 0666)) {
		if (errno != EEXIST) {
			perror("fail to open fifo: ") ;
			exit(1) ;
		}
	}

	if(mutex != &lock){
	   pthread_mutex_lock(&lock);
		int fd = open("channel", O_WRONLY | O_SYNC) ;
		char buf [128];		
		sprintf(buf, "0 %lu %p", pthread_self(), mutex);
		write(fd, buf, 128);
		close(fd);
   	   pthread_mutex_unlock(&lock);
	}

	pthread_lock_orig = dlsym(RTLD_NEXT, "pthread_mutex_lock");
	int result = pthread_lock_orig(mutex);

	return result;
}


int pthread_mutex_unlock(pthread_mutex_t *mutex){
	
	int (*pthread_unlock_orig)(pthread_mutex_t *mutex);
	pthread_unlock_orig = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
	
	if(mutex != &lock){
	   pthread_mutex_lock(&lock);
		int fd = open("channel", O_WRONLY | O_SYNC) ;
		char buf [128];		
		sprintf(buf, "1 %lu %p", pthread_self(), mutex);
		write(fd, buf, 128);
		close(fd);
   	   pthread_mutex_unlock(&lock);
	}
	int result = pthread_unlock_orig(mutex);

	return result;
}
