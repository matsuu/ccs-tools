/*
 * env_chk.c
 *
 * An example program for execute_handler .
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5   2008/11/11
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void unescape(unsigned char *dest)
{
	unsigned char *src = dest;
	unsigned char c;
	unsigned char d;
	unsigned char e;
	while (1) {
		c = *src++;
		if (!c)
			break;
		if (c != '\\') {
			*dest++ = c;
			continue;
		}
		c = *src++;
		if (c == '\\') {
			*dest++ = c;
			continue;
		}
		if (c < '0' || c > '3')
			break;
		d = *src++;
		if (d < '0' || d > '7')
			break;
		e = *src++;
		if (e < '0' || e > '7')
			break;
		*dest++ = ((c - '0') << 6) + ((d - '0') << 3) + (e - '0');
	}
	*dest = '\0';
}

int main(int raw_argc, char *raw_argv[])
{
	int i;
	int argc;
	int envc;
	char *filename;
	char **argv;
	char **envp;
	{ /* Check that I'm an execute handler process.  */
		int fd = open("/proc/ccs/.execute_handler", 0);
		close(fd);
		if (fd == EOF) {
			fprintf(stderr, "FATAL: I'm not execute_handler.\n");
			return 1;
		}
	}
	if (raw_argc < 7)
		return 1;
	filename = raw_argv[4];
	argc = atoi(raw_argv[5]);
	envc = atoi(raw_argv[6]);
	if (raw_argc != argc + envc + 7)
		return 1;
	for (i = 5; i < argc + 5; i++)
		raw_argv[i] = raw_argv[i + 2];
	raw_argv[argc + 5] = NULL;
	for (i = argc + 6; i < argc + envc + 6; i++)
		raw_argv[i] = raw_argv[i + 1];
	raw_argv[argc + envc + 6] = NULL;
	argv = raw_argv + 5;
	envp = raw_argv + argc + 6;
	/*
	 * "/usr/sbin/sshd" executes "/usr/sbin/sshd -R".
	 * So, don't check environment variables.
	 */
	unescape(raw_argv[2]);
	if (argc == 2 && !strcmp(argv[1], "-R") &&
	    !strcmp(raw_argv[2], filename)) {
		execve(filename, argv, envp);
		return 1;
	}
	/*
	 * Check environment variables passed to execve() request
	 * and execute the program if it has "CERBERUS=sftp" environment.
	 */
	for (i = 0; i < envc; i++) {
		if (strcmp(envp[i], "CERBERUS=sftp"))
			continue;
		while (i < envc) {
			envp[i] = envp[i + 1];
			i++;
		}
		execve(filename, argv, envp);
		break;
	}
	return 1;
}
