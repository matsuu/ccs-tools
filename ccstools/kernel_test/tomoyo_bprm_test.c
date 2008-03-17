/*
 * tomoyo_bprm_test.c
 *
 * Testing program for fs/tomoyo_cond.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0-pre   2008/03/17
 *
 */
#include "include.h"

static int domain_fd = EOF;

static void try_exec(const char *policy, char *argv[], char *envp[], const char should_success) {
	int err = 0;
	int pipe_fd[2] = { EOF, EOF };
	pipe(pipe_fd);
	printf("%s: ", policy);
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
	fflush(stdout); fflush(stderr);
	if (fork() == 0) {
		execve("/bin/true", argv, envp);
		err = errno;
		write(pipe_fd[1], &err, sizeof(err));
		_exit(0);
	}
	close(pipe_fd[1]);
	read(pipe_fd[0], &err, sizeof(err));
	close(pipe_fd[0]);
	write(domain_fd, "delete ", 7);
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
	if (should_success) {
		if (!err) printf("OK\n");
		else printf("BUG: failed (%d)\n", err);
	} else {
		if (err == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: failed (%d)\n", err);
	}
}

static void StageExecTest(void) {
	static char *argv[128], *envp[128];
	memset(argv, 0, sizeof(argv)); memset(envp, 0, sizeof(envp));

	argv[0] = "/bin/true";
	try_exec("allow_execute /bin/true if exec.argc=1", argv, envp, 1);
	argv[0] = NULL;
	try_exec("allow_execute /bin/true if exec.argc=1", argv, envp, 0);

	envp[0] = "";
	try_exec("allow_execute /bin/true if exec.envc=1", argv, envp, 1);
	envp[0] = NULL;
	try_exec("allow_execute /bin/true if exec.envc=1", argv, envp, 0);

	argv[0] = "/bin/true";
	argv[1] = "--";
	try_exec("allow_execute /bin/true if exec.argc=1-5", argv, envp, 1);
	try_exec("allow_execute /bin/true if exec.argc!=1-5", argv, envp, 0);

	envp[0] = "";
	envp[1] = "";
	try_exec("allow_execute /bin/true if exec.envc=1-5", argv, envp, 1);
	try_exec("allow_execute /bin/true if exec.envc!=1-5", argv, envp, 0);

	argv[0] = "/bin/true";
	argv[1] = "--";
	try_exec("allow_execute /bin/true if exec.argv[1]=\"--\"", argv, envp, 1);
	try_exec("allow_execute /bin/true if exec.argv[1]!=\"--\"", argv, envp, 0);

	argv[0] = "/bin/true";
	argv[1] = "-";
	try_exec("allow_execute /bin/true if exec.argv[1]=\"--\"", argv, envp, 0);
	try_exec("allow_execute /bin/true if exec.argv[1]!=\"--\"", argv, envp, 1);

	envp[0] = "HOME=/";
	try_exec("allow_execute /bin/true if exec.envp[\"HOME\"]!=NULL", argv, envp, 1);
	try_exec("allow_execute /bin/true if exec.envp[\"HOME\"]=NULL", argv, envp, 0);
	try_exec("allow_execute /bin/true if exec.envp[\"HOME\"]=\"/\"", argv, envp, 1);
	try_exec("allow_execute /bin/true if exec.envp[\"HOME\"]!=\"/\"", argv, envp, 0);

	envp[0] = "HOME2=/";
	try_exec("allow_execute /bin/true if exec.envp[\"HOME\"]!=NULL", argv, envp, 0);
	try_exec("allow_execute /bin/true if exec.envp[\"HOME\"]=NULL", argv, envp, 1);
	try_exec("allow_execute /bin/true if exec.envp[\"HOME\"]=\"/\"", argv, envp, 0);
	try_exec("allow_execute /bin/true if exec.envp[\"HOME\"]!=\"/\"", argv, envp, 1);
}

int main(int argc, char *argv[]) {
	const char *cp;
	static char self_domain[4096];
	int profile_fd;
	int self_fd;
	Init();
	profile_fd = open("/proc/ccs/profile", O_WRONLY);
	self_fd = open("/proc/ccs/self_domain", O_RDONLY);
	domain_fd = open("/proc/ccs/domain_policy", O_WRONLY);
	memset(self_domain, 0, sizeof(self_domain));
	read(self_fd, self_domain, sizeof(self_domain) - 1);
	close(self_fd);
	write(domain_fd, self_domain, strlen(self_domain));
	cp = " /bin/true\n";
	write(domain_fd, cp, strlen(cp));
	cp = "use_profile 255\n";
	write(domain_fd, cp, strlen(cp));
	write(domain_fd, self_domain, strlen(self_domain));
	write(domain_fd, "\n", 1);
	cp = "use_profile 255\n";
	write(domain_fd, cp, strlen(cp));
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	StageExecTest();
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	ClearStatus();
	return 0;
}
