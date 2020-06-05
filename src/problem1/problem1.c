#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <error.h>

int pipes[2];

char *
read_exec(char *exe)
{
	pipe(pipes);
	int fd = open("hello.out", O_WRONLY | O_CREAT, 0644);
	dup2(fd, 1);
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Wrong number of arguments\n");
		exit(1);
	}

	char *s;
	s = read_exec(argv[1]);
	printf("\"%s\"\n", s);
	free(s);
	exit(0);
}
