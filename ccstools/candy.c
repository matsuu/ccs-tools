/*
 * candy.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5-pre 2008/10/20
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>

static const char *get_shell(void)
{
	static char *shell = NULL;
	if (!shell) {
		struct passwd *pw = getpwuid(getuid());
		shell = pw ? pw->pw_shell : "/bin/sh";
	}
	return shell;
}

static int GetStartTime(pid_t pid, unsigned long long *t)
{
	FILE *fp;
	int i;
	char *cp;
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%d/stat", pid);
	fp = fopen(buffer, "r");
	if (!fp)
		return EOF;
	fgets(buffer, sizeof(buffer) - 1, fp);
	fclose(fp);
	for (i = 0; i < 21; i++) {
		cp = strchr(buffer, ' ');
		if (!cp)
			return EOF;
		memmove(buffer, cp + 1, strlen(cp + 1) + 1);
	}
	cp = strchr(buffer, ' ');
	if (!cp)
		return EOF;
	*cp = '\0';
	if (sscanf(buffer, "%llu", t) != 1)
		return EOF;
	return 0;
}

int main(int argc, char *argv[])
{
	static char buffer[1024];
	static const char *passwd = "CERBERUS\n";
	int trial;
	const char *shell = get_shell();
	for (trial = 0; trial < 3; trial++) {
		memset(buffer, 0, sizeof(buffer));
		printf("Password: ");
		fgets(buffer, sizeof(buffer) - 1, stdin);
		if (shell && !strcmp(buffer, passwd)) {
			unsigned long long t0;
			unsigned long long t1;
			if (GetStartTime(getppid(), &t0) == 0 &&
			    GetStartTime(getpid(), &t1) == 0) {
				/* 10 sec */
				if ((t1 - t0) < 1000)
					execlp(shell, shell, NULL);
			}
		}
		sleep(3);
	}
	printf("Authentication Failure\n");
	return 0;
}
