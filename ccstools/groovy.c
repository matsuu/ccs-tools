/*
 * groovy.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-rc   2009/03/04
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

int main(int argc, char *argv[])
{
	static char buffer[1024];
	static const char *lockfile = "/tmp/.lockme";
	int trial;
	const char *shell = get_shell();
	for (trial = 0; trial < 3; trial++) {
		memset(buffer, 0, sizeof(buffer));
		printf("Password: ");
		fgets(buffer, sizeof(buffer) - 1, stdin);
		if (shell) {
			int fd;
			fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0600);
			if (fd != EOF) {
				close(fd);
				execlp(shell, shell, NULL);
			}
		}
		sleep(3);
	}
	printf("Authentication Failure\n");
	return 0;
}
