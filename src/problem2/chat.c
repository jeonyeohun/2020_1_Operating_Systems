#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

/* TODO: you can freely edit this file */

void *sender(void *ptr)
{
	char *channel = *(char *)ptr;
	if (mkfifo(channel, 0666))
	{
		if (errno != EEXIST)
		{
			perror("fail to open fifo: ");
			exit(1);
		}
	}

	int fd = open(channel, O_WRONLY | O_SYNC | O_NONBLOCK);

	while (1)
	{
		char s[256];
		fgets(s, 256, stdin);
		char newline;

		int i = 0;
		while (i < 128 && ((s[i] = getchar()) != EOF && s[i++] != '\n'))
			;
		s[i - 1] = 0x0;

		if (s[0] == 0x0)
			break;

		for (int i = 0; i < 128;)
		{
			i += write(fd, s + i, 128);
		}
	}
	close(fd);
}

void *reciever(void *ptr)
{
	char *channel = *(char *)ptr;
	int fd = open(channel, O_RDONLY | O_SYNC | O_NONBLOCK);

	while (1)
	{
		char s[256];
		int len;
		len = read(fd, s, 128);
		if (len > 0)
			printf("%s\n", s);
	}
	close(fd);
}

void handler()
{
	pthread_cancel(threads[0]);
	pthread_cancel(threads[1]);
	printf("Goodbye\n");
}

int main(int argc, char **argv)
{
	singal(SIGINT, handler);
	pthread_t threads[2];
	if (argc != 3)
	{
		fprintf(stderr, "Wrong number of arguments\n");
		exit(1);
	}

	char *r_fifo = argv[1];
	char *w_fifo = argv[2];

	pthread_create(threads[0], 0x0, sender, w_fifo);
	pthread_create(threads[0], 0x0, reciever, r_fifo);

	pthread_join(threads[0], 0x0);
	pthread_join(threads[1], 0x0);
}
