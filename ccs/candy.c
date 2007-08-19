/*
 * candy.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.3.2 2007/02/14
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

static const char *get_shell(void) {
	static char *shell = NULL;
	if (!shell) {
		struct passwd *pw = getpwuid(getuid());
		shell = pw ? pw->pw_shell : "/bin/sh";
	}
	return shell;
}

static int GetStartTime(pid_t pid, unsigned long long *t) {
	FILE *fp;
	int i;
	char *cp, buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%d/stat", pid);
	if ((fp = fopen(buffer, "r")) == NULL) return EOF;
	fgets(buffer, sizeof(buffer) - 1, fp);
	fclose(fp);
	for (i = 0; i < 21; i++) {
		cp = strchr(buffer, ' ');
		if (!cp) return EOF;
		memmove(buffer, cp + 1, strlen(cp + 1) + 1);
	}
	cp = strchr(buffer, ' ');
	if (!cp) return EOF;
	*cp = '\0';
	if (sscanf(buffer, "%llu", t) != 1) return EOF;
	return 0;
}
	
int main(int argc, char *argv[]) {
	static char buffer[1024];
	static const char *passwd = "CERBERUS\n";
	int trial;
	const char *shell = get_shell();
	for (trial = 0; trial < 3; trial++) {
		memset(buffer, 0, sizeof(buffer));
		printf("Password: ");
		fgets(buffer, sizeof(buffer) - 1, stdin);
		if (shell && strcmp(buffer, passwd) == 0) {
			unsigned long long t0, t1;
			if (GetStartTime(getppid(), &t0) == 0 && GetStartTime(getpid(), &t1) == 0) {
				if ((t1 - t0) < 1000) execlp(shell, shell, NULL); /* 10 sec */
			}
		}
		sleep(3);
	}
	printf("Authentication Failure\n");
	return 0;
}
