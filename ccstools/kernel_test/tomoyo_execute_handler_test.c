/*
 * tomoyo_execute_handler_test.c
 *
 * Testing program for fs/tomoyo_domain.c
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7   2009/04/01
 *
 */
#include "include.h"

int main(int raw_argc, char *raw_argv[])
{
	char buffer[4096];
	char *cp;
	char *cp2;
	FILE *fp;
	int error;
	if (access(proc_policy_domain_policy, F_OK)) {
		fprintf(stderr, "You can't use this program for this kernel."
			"\n");
		return 1;
	}
	ccs_test_init();
	memset(buffer, 0, sizeof(buffer));
	fp = fopen(proc_policy_process_status, "r+");
	if (!fp)
		return 1;
	fprintf(fp, "info %d\n", pid);
	fflush(fp);
	fgets(buffer, sizeof(buffer) - 1, fp);
	fclose(fp);
	if (strstr(buffer, " execute_handler=yes")) {
		int i;
		int argc;
		int envc;
		char *filename;
		char **argv;
		char **envp;
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
		 * Check parameters passed to execve() request.
		 */
		if (0) {
			fprintf(stderr, "Domain = %s\n", raw_argv[1]);
			fprintf(stderr, "Caller Program = %s\n", raw_argv[2]);
			fprintf(stderr, "Process Status = %s\n", raw_argv[3]);
			fprintf(stderr, "Requested Program = %s\n", filename);
			fprintf(stderr, "argc=%d\n", argc);
			fprintf(stderr, "envc=%d\n", envc);
			for (i = 0; i < argc; i++)
				fprintf(stderr, "argv[%d] = %s\n", i, argv[i]);
			for (i = 0; i < envc; i++)
				fprintf(stderr, "envp[%d] = %s\n", i, envp[i]);
			fprintf(stderr, "\n");
		}
		/*
		 * Continue if filename and argv[] and envp[] are appropriate.
		 */
		if (1)
			execve(filename, argv, envp);
		return 1;
	}
	memset(buffer, 0, sizeof(buffer));
	fp = fopen(proc_policy_self_domain, "r");
	if (!fp)
		return 1;
	fgets(buffer, sizeof(buffer) - 1, fp);
	fclose(fp);
	cp2 = strrchr(buffer, ' ');
	if (!cp2)
		return 1;
	cp2++;
	fp = fopen(proc_policy_domain_policy, "r+");
	if (!fp)
		return 1;
	fprintf(fp, "select pid=%u\n", pid);
	fprintf(fp, "execute_handler %s\n", cp2);
	fflush(fp);
	if (fork() == 0) {
		char *arg[3] = { "echo", "OK: execute handler succeeded",
				 NULL };
		char *env[2] = { "execute_handler", NULL };
		execve("/bin/echo", arg, env);
		_exit(1);
	}
	wait(&error);
	error = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
	if (error) {
		printf("BUG: execute handler failed\n");
		fflush(stdout);
	}
	fprintf(fp, "delete execute_handler %s\n", cp2);
	fprintf(fp, "denied_execute_handler %s\n", cp2);
	fprintf(fp, "delete allow_execute /bin/echo\n");
	fprintf(fp, "%s %s\n", buffer, cp2);
	fprintf(fp, "use_profile 0\n");
	fflush(fp);
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	if (fork() == 0) {
		char *arg[3] = { "echo", "OK: denied execute handler succeeded",
				 NULL };
		char *env[2] = { "denied_execute_handler", NULL };
		execve("/bin/echo", arg, env);
		_exit(1);
	}
	wait(&error);
	error = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
	if (error) {
		printf("BUG: execute handler failed\n");
		fflush(stdout);
	}
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	fprintf(fp, "delete denied_execute_handler %s\n", cp2);
	fclose(fp);
	return 0;
}
