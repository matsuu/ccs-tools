/*
 * tomoyo_signal_test.c
 *
 * Testing program for fs/tomoyo_signal.c
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0-rc   2007/09/20
 *
 */
#include "include.h"

static int is_enforce = 0;

static void ShowPrompt(const char *str) {
	printf("Testing %6s: (%s) ", str, is_enforce ? "must fail" : "should success");
	errno = 0;
}

static void ShowResult(int result) {
	if (is_enforce) {
		if (result == EOF) {
			if (errno == EPERM) printf("OK: Permission denied.\n");
			else printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG!\n");
		}
	} else {
		if (result != EOF) printf("OK\n");
		else printf("%s\n", strerror(errno));
	}
	fflush(stdout);
}

static int Child(void) {
	char c = 0;
	signal(SIGTERM, SIG_IGN);
	if (write(1, &c, 1) != 1) {
		fprintf(stderr, "Can't write to pipe.\n"); fflush(stderr);
		_exit(1);
	}
	while (read(0, &c, 1));
	return 0;
}

static int Parent(const char *self) {
	int i, j;
	for (i = 0; i < 2; i++) {
		if (i == 0) {
			WriteStatus("MAC_FOR_SIGNAL=3\n");
			is_enforce = 1;
			printf("***** Testing signal hooks in enforce mode. *****\n"); fflush(stdout);
		} else {
			WriteStatus("MAC_FOR_SIGNAL=2\n");
			is_enforce = 0;
			printf("***** Testing signal hooks in permissive mode. *****\n"); fflush(stdout);
		}
		for (j = 0; j < 3; j++) {
			int pipe_fd[2] = { EOF, EOF };
			pid_t pid;
			char c = 0;
			if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_fd) || fcntl(pipe_fd[1], F_SETFL, 0)) {
				fprintf(stderr, "FAILED to create socketpair.\n"); fflush(stderr);
				exit(1);
			}
			pid = fork();
			switch(pid) {
			case 0:
				if (close(pipe_fd[0]) == 0 && close(0) == 0 && close(1) == 0 && dup2(pipe_fd[1], 0) != EOF && dup2(pipe_fd[1], 1) != EOF)
					execlp(self, self, "--", NULL);
				fprintf(stderr, "Can't exec()\n"); fflush(stderr);
				_exit(1);
			case -1:
				fprintf(stderr, "Can't fork()\n"); fflush(stderr);
				exit(1);
			}
			close(pipe_fd[1]);
			read(pipe_fd[0], &c, 1);
			switch (j) {
			case 0:
				ShowPrompt("kill");
				ShowResult(kill(pid, SIGTERM));
				break;
			case 1:
				ShowPrompt("tkill");
				ShowResult(tkill(pid, SIGTERM));
				break;
			case 2:
#ifdef __NR_tgkill
				if (is_kernel26) {
					ShowPrompt("tgkill");
					ShowResult(tgkill(pid, pid, SIGTERM));
				}
#endif
				break;
			}
			close(pipe_fd[0]);
			while (wait(NULL) != EOF || errno == EINTR);
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
	PreInit();
	if (access(proc_policy_dir, F_OK)) {
		fprintf(stderr, "You can't use this program for this kernel.\n");
		return 1;
	}
	if (argc > 1) return Child();
	Init();
	Parent(argv[0]);
	ClearStatus();
	return 0;
}
