#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <stdlib.h>

static int child(void *arg)
{
	char **argv = (char **) arg;
	argv++;
	mount("/tmp/", "/tmp/", "tmpfs", 0, NULL);
	execvp(argv[0], argv);
	_exit(1);
}

int main(int argc, char *argv[])
{
	char c = 0;
	char *stack = malloc(8192);
	const pid_t pid = clone(child, stack + (8192 / 2), CLONE_NEWNS,
				(void *) argv);
	while (waitpid(pid, NULL, __WALL) == EOF && errno == EINTR)
		c++; /* Dummy. */
	return 0;
}
