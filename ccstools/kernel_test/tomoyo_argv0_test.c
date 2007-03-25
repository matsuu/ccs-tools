/*
 * tomoyo_argv0_test.c
 *
 * Testing program for fs/tomoyo_exec.c
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.4   2007/04/01
 *
 */
#include "include.h"

static int is_enforce = 0;

static void ShowPrompt(const char *str) {
	printf("Testing %40s: (%s) ", str, is_enforce ? "must fail" : "should success");
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
}

static void StageArgv0Test(void) {
	static char buffer[1024];
	char *argv[2] = { "false", NULL };
	int status = 0;
	memset(buffer, 0, sizeof(buffer));
	{
		is_enforce = 0; WriteStatus("MAC_FOR_ARGV0=2\n");
		if (fork() == 0) {
			execv("/bin/true", argv);
			_exit(errno);
		}
		snprintf(buffer, sizeof(buffer) - 1, "Executing /bin/true in permissive mode");
		ShowPrompt(buffer);
		wait(&status);
		errno = WEXITSTATUS(status);
		ShowResult(errno ? EOF : 0);

		is_enforce = 1; WriteStatus("MAC_FOR_ARGV0=3\n");
		if (fork() == 0) {
			execv("/bin/true", argv);
			_exit(errno);
		}
		snprintf(buffer, sizeof(buffer) - 1, "Executing /bin/true in enforce mode");
		ShowPrompt(buffer);
		wait(&status);
		errno = WEXITSTATUS(status);
		ShowResult(errno ? EOF : 0);

		is_enforce = 0; 
		if (fork() == 0) {
			argv[0] = "";
			execv("/bin/true", argv);
			_exit(errno);
		}
		snprintf(buffer, sizeof(buffer) - 1, "Executing /bin/true in enforce mode");
		ShowPrompt(buffer);
		wait(&status);
		errno = WEXITSTATUS(status);
		ShowResult(errno ? EOF : 0);
	}	
}

int main(int argc, char *argv[]) {
	Init();
	StageArgv0Test();
	ClearStatus();
	return 0;
}
