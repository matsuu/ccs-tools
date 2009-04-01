/*
 * generate_execve_log.c
 *
 * Testing program for fs/tomoyo_audit.c
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7   2009/04/01
 *
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc0, char *argv0[])
{
	char *argv[128];
	char *envp[128];
	char buffer[16384];
	memset(argv, 0, sizeof(argv));
	memset(envp, 0, sizeof(envp));

	if (fork() == 0) {
		execve("/bin/true", NULL, NULL);
		_exit(0);
	}
	wait(NULL);

	if (fork() == 0) {
		execve("/bin/true", argv, NULL);
		_exit(0);
	}
	wait(NULL);

	if (fork() == 0) {
		execve("/bin/true", NULL, envp);
		_exit(0);
	}
	wait(NULL);

	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	argv[0] = "";
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	argv[0] = NULL;
	envp[0] = "";
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	argv[0] = "\xFF\xFE\x01\x02";
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	memset(buffer, ' ', 1000);
	argv[1] = buffer;
	envp[0] = "envp[0]";
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	argv[2] = buffer;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	argv[3] = buffer;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	envp[1] = "envp[1]";
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	envp[2] = buffer;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	return 0;
	/*
	  memset(argv, 0, sizeof(argv));
	  memset(envp, 0, sizeof(envp));
	*/
	memset(buffer, 'a', sizeof(buffer) - 1);
	argv[0] = "true";
	argv[1] = buffer;
	argv[2] = NULL;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	/*
	  memset(argv, 0, sizeof(argv));
	  memset(envp, 0, sizeof(envp));
	*/
	buffer[4000] = '\0';
	argv[0] = buffer;
	argv[1] = buffer;
	argv[2] = buffer;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	argv[2] = NULL;
	envp[0] = buffer;
	envp[1] = buffer;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	argv[1] = NULL;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	argv[0] = NULL;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	envp[1] = NULL;
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		_exit(0);
	}
	wait(NULL);

	return 0;
}
