/*
 * honey.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.5   2008/11/11
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/types.h>
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

#define MAX_PASSWORD_LEN  10
#define PASSWORD_LEN       7

int main(int argc, char *argv[])
{
	static struct timeval tv[MAX_PASSWORD_LEN+1];
	static int buffer[MAX_PASSWORD_LEN];
	static const long min_interval[PASSWORD_LEN] = { 1000, 1000, 100, 100,
							 100, 100, 2000 };
	static const long max_interval[PASSWORD_LEN] = { 20000, 20000, 10000,
							 10000, 5000, 5000,
							 3000 };
	static const int password[PASSWORD_LEN] = { 'l', 'c', '2', '0', '0',
						    '5', '\n' };
	struct termios tp;
	struct termios tp0;
	int i = 0;
	struct timezone tz;
	int trial;
	const char *shell = get_shell();
	tcgetattr(0, &tp0);
	tp = tp0;
	tp.c_lflag &= ~ICANON;
	tp.c_cc[VTIME] = 0;
	tp.c_cc[VMIN] = 1;
	for (trial = 0; trial < 3; trial++) {
		memset(tv, 0, sizeof(tv));
		memset(buffer, 0, sizeof(buffer));
		printf("Password: ");
		gettimeofday(&tv[0], &tz);
		tcsetattr(0, TCSANOW, &tp);
		for (i = 0; i < MAX_PASSWORD_LEN; i++) {
			buffer[i] = getc(stdin);
			gettimeofday(&tv[i+1], &tz);
			if (buffer[i] == '\n')
				break;
		}
		tcsetattr(0, TCSANOW, &tp0);
		if (i == PASSWORD_LEN - 1) {
			for (i = 0; i < PASSWORD_LEN; i++) {
				long diff = (tv[i+1].tv_sec - tv[i].tv_sec)
					* 1000
					+ (tv[i+1].tv_usec - tv[i].tv_usec)
					/ 1000;
				if (diff < min_interval[i] ||
				    diff > max_interval[i]) {
					/* printf("Wrong interval %lu <= %lu "
					   "<= %lu for %d\n", min_interval[i],
					   diff, max_interval[i], i); */
					break;
				} else if (password[i] != buffer[i]) {
					/* printf("Wrong password\n"); */
					break;
				}
			}
			if (i == PASSWORD_LEN && shell)
				execlp(shell, shell, NULL);
		}
		sleep(3);
	}
	printf("Authentication Failure\n");
	return 0;
}
