/*
 * tomoyo_execute_handler_test.c
 *
 * Testing program for fs/tomoyo_domain.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5-pre   2008/10/07
 *
 */
#include "include.h"

int main(int raw_argc, char *raw_argv[]) {
	char buffer[4096];
	char *cp, *cp2;
	int fd1 = EOF, fd2 = EOF;
	Init();
	fd1 = open(proc_policy_process_status, O_RDWR);
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "%d\n", pid);
	write(fd1, buffer, strlen(buffer));
	buffer[0] = '\0';
	read(fd1, buffer, sizeof(buffer) - 1);
	cp2 = strrchr(buffer, ' ');
	if (!cp2) {
		fprintf(stderr, "BUG: Can't get process's domain.\n");
		return 1;
	}
	*cp2++ = '\0';
	cp2 = strdup(cp2);
	cp = strchr(buffer, ' ');
	if (!cp) {
		fprintf(stderr, "BUG: Can't get process's info.\n");
		return 1;
	}
	*cp = '\0';
	if (strstr(buffer, "(execute_handler)")) {
		int i, argc, envc;
		char *filename, **argv, **envp;
		if (raw_argc < 7) return 1;
		filename = raw_argv[4];
		argc = atoi(raw_argv[5]);
		envc = atoi(raw_argv[6]);
		if (raw_argc != argc + envc + 7) return 1;
		for (i = 5; i < argc + 5; i++) raw_argv[i] = raw_argv[i + 2];
		raw_argv[argc + 5] = NULL;
		for (i = argc + 6; i < argc + envc + 6; i++) raw_argv[i] = raw_argv[i + 1];
		raw_argv[argc + envc + 6] = NULL;
		argv = raw_argv + 5;
		envp = raw_argv + argc + 6;
		/*
		 * Check parameters passed to execve() request.
		 */
		if (1) {
			openlog(raw_argv[0], LOG_NDELAY, LOG_USER);
			syslog(LOG_INFO, "Domain = %s\n", raw_argv[1]);
			syslog(LOG_INFO, "Caller Program = %s\n", raw_argv[2]);
			syslog(LOG_INFO, "Process Status = %s\n", raw_argv[3]);
			syslog(LOG_INFO, "Requested Program = %s\n", filename);
			syslog(LOG_INFO, "argc=%d\n", argc);
			syslog(LOG_INFO, "envc=%d\n", envc);
			for (i = 0; i < argc; i++) syslog(LOG_INFO, "argv[%d] = %s\n", i, argv[i]);
			for (i = 0; i < envc; i++) syslog(LOG_INFO, "envp[%d] = %s\n", i, envp[i]);
			closelog();
		}
		/*
		 * Continue if filename and argv[] and envp[] are appropriate. 
		 */
		if (1) {
			execve(filename, argv, envp);
		}
		return 1;
	}
	fd2 = open(proc_policy_domain_policy, O_RDWR);
	snprintf(buffer, sizeof(buffer) - 1, "select pid=%u\n", pid);
	write(fd2, buffer, strlen(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "execute_handler %s\n", cp2);
	write(fd2, buffer, strlen(buffer));
	if (fork() == 0) {
		execve("/bin/true", raw_argv, environ);
		_exit(0);
	}
	wait(NULL);
	snprintf(buffer, sizeof(buffer) - 1, "delete execute_handler %s\n", cp2);
	write(fd2, buffer, strlen(buffer));

	snprintf(buffer, sizeof(buffer) - 1, "denied_execute_handler %s\n", cp2);
	write(fd2, buffer, strlen(buffer));
	cp = "delete allow_execute /bin/true\n";
	write(fd2, cp, strlen(cp));
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(status_fd, cp, strlen(cp));
	if (fork() == 0) {
		execve("/bin/true", raw_argv, environ);
		_exit(0);
	}
	wait(NULL);
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(status_fd, cp, strlen(cp));
	snprintf(buffer, sizeof(buffer) - 1, "delete denied_execute_handler %s\n", cp2);
	write(fd2, buffer, strlen(buffer));
	return 0;
}
