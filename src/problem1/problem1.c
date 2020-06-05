#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
//#include <error.h>
#include <fcntl.h>

int pipes[2];

void parent_proc()
{
	char *buf = 0x0;
	ssize_t s;
	size_t len = 0;

	close(pipes[0]);

	while ((s = getline(&buf, &len, stdin)) != -1)
	{
		buf[s - 1] = 0x0;

		ssize_t sent = 0;
		char *data = buf;

		while (sent < s)
		{
			sent += write(pipes[1], buf + sent, s - sent);
		}

		free(buf);
		buf = 0x0;
		len = 0;
	}
	close(pipes[1]);
}

char *child_proc()
{
	char buf[32];
	ssize_t s;

	close(pipes[1]);

	while ((s = read(pipes[0], buf, 31)) > 0)
	{
		buf[s + 1] = 0x0;
	}

	return buf;
}

char *
read_exec(char *exe)
{
	int fd = open("./hello", O_WRONLY | O_CREAT, 0644);
	dup2(fd, pipes[1]);
	char *r = child_proc();
	close(fd);
	return r;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Wrong number of arguments\n");
		exit(1);
	}
	pipe(pipes);

	char *s;
	s = read_exec(argv[1]);
	printf("\"%s\"\n", s);

	//free(s);
	exit(0);
}
