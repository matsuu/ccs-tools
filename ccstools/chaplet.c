/*
 * chaplet.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7   2009/04/01
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <time.h>

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
	static char seed[40];
	int i;
	int trial;
	const char *shell = get_shell();
	srand(time(NULL));
	for (trial = 0; trial < 3; trial++) {
		char *sp;
		char *dp;
		memset(seed, 0, sizeof(seed));
		for (i = 0; i < sizeof(seed) - 1; i++)
			seed[i] = (rand() % 64) + 33;
		printf("Challenge: %s\n", seed);
		dp = seed;
		sp = dp;
		while (1) {
			char c = *sp;
			*dp = c;
			if (!c)
				break;
			if (*sp < '0' || *sp > '9') {
				sp++;
				continue;
			}
			sp++;
			dp++;
		}
		/* printf("Answer: %s\n", seed); */
		memset(buffer, 0, sizeof(buffer));
		printf("Response: ");
		fgets(buffer, sizeof(buffer) - 1, stdin);
		dp = strchr(buffer, '\n');
		if (dp)
			*dp = '\0';
		if (!strcmp(buffer, seed)) {
			if (shell)
				execlp(shell, shell, NULL);
		}
		sleep(3);
	}
	printf("Authentication Failure\n");
	return 0;
}
