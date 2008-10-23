/*
 * tomoyo_bprm_test.c
 *
 * Testing program for fs/tomoyo_cond.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5-pre   2008/10/20
 *
 */
#include "include.h"

static int domain_fd = EOF;
static char self_domain[4096];

static void try_exec(const char *policy, char *argv[], char *envp[],
		     const char should_success) {
	FILE *fp;
	char buffer[8192];
	char *cp;
	int domain_found = 0;
	int policy_found = 0;
	int err = 0;
	int pipe_fd[2] = { EOF, EOF };
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	fp = fopen(proc_policy_domain_policy, "r");
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	pipe(pipe_fd);
	printf("%s: ", policy);
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
	fflush(stdout);
	if (!fp) {
		printf("BUG: policy read failed\n");
		return;
	}
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		cp = strchr(buffer, '\n');
		if (cp)
			*cp = '\0';
		if (!strncmp(buffer, "<kernel>", 8))
			domain_found = !strcmp(self_domain, buffer);
		if (domain_found) {
			/* printf("<%s>\n", buffer); */
			if (!strcmp(buffer, policy)) {
				policy_found = 1;
				break;
			}
		}
	}
	fclose(fp);
	if (!policy_found) {
		printf("BUG: policy write failed\n");
		return;
	}
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
		if (!err)
			printf("OK\n");
		else
			printf("BUG: failed (%d)\n", err);
	} else {
		if (err == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: failed (%d)\n", err);
	}
}

static void stage_exec_test(void)
{
	int i;
	static char *argv[128];
	static char *envp[128];
	for (i = 0; i < 10; i++) {
		memset(argv, 0, sizeof(argv));
		memset(envp, 0, sizeof(envp));

		argv[0] = "/bin/true";
		try_exec("allow_execute /bin/true "
			 "if task.gid=0-100 exec.argc=1", argv, envp, 1);
		argv[0] = NULL;
		try_exec("allow_execute /bin/true "
			 "if task.gid=0-100 exec.argc=1", argv, envp, 0);

		envp[0] = "";
		try_exec("allow_execute /bin/true if task.gid!=100 task.euid=0 "
			 "path1.uid=0 path1.parent.uid=0 exec.envc=1", argv,
			 envp, 1);
		envp[0] = NULL;
		try_exec("allow_execute /bin/true if task.gid!=100 task.euid=0 "
			 "path1.uid=0 path1.parent.uid=0 exec.envc=1", argv,
			 envp, 0);

		argv[0] = "/bin/true";
		argv[1] = "--";
		try_exec("allow_execute /bin/true if 0=0 exec.argc=1-5", argv,
			 envp, 1);
		try_exec("allow_execute /bin/true if 0=0 exec.argc!=1-5", argv,
			 envp, 0);

		envp[0] = "";
		envp[1] = "";
		try_exec("allow_execute /bin/true if task.uid=0 "
			 "task.gid!=1-100 path1.parent.uid!=1 path1.gid=0 "
			 "exec.envc=1-5", argv, envp, 1);
		try_exec("allow_execute /bin/true if task.uid=0 "
			 "task.gid!=1-100 path1.parent.uid!=1 path1.gid=0 "
			 "exec.envc!=1-5", argv, envp, 0);

		argv[0] = "/bin/true";
		argv[1] = "--";
		try_exec("allow_execute /bin/true if task.uid=0 task.gid=0 "
			 "path1.parent.uid=0 path1.uid=0 exec.argv[1]=\"--\"",
			 argv, envp, 1);
		try_exec("allow_execute /bin/true if task.uid=0 task.gid=0 "
			 "path1.parent.uid=0 path1.uid=0 exec.argv[1]!=\"--\"",
			 argv, envp, 0);

		argv[0] = "/bin/true";
		argv[1] = "-";
		try_exec("allow_execute /bin/true if 1!=0 exec.argv[1]=\"--\"",
			 argv, envp, 0);
		try_exec("allow_execute /bin/true if 1!=0 exec.argv[1]!=\"--\"",
			 argv, envp, 1);

		envp[0] = "HOME=/";
		try_exec("allow_execute /bin/true if task.euid=0 "
			 "exec.envp[\"HOME\"]!=NULL", argv, envp, 1);
		try_exec("allow_execute /bin/true if task.euid=0 "
			 "exec.envp[\"HOME\"]=NULL", argv, envp, 0);
		try_exec("allow_execute /bin/true if 0!=1 "
			 "exec.envp[\"HOME\"]=\"/\"", argv, envp, 1);
		try_exec("allow_execute /bin/true if 0!=1 "
			 "exec.envp[\"HOME\"]!=\"/\"", argv, envp, 0);

		envp[0] = "HOME2=/";
		try_exec("allow_execute /bin/true if path1.uid=0 "
			 "exec.envp[\"HOME\"]!=NULL", argv, envp, 0);
		try_exec("allow_execute /bin/true if path1.uid=0 "
			 "exec.envp[\"HOME\"]=NULL", argv, envp, 1);
		try_exec("allow_execute /bin/true if 100=1-1000 "
			 "exec.envp[\"HOME\"]=\"/\"", argv, envp, 0);
		try_exec("allow_execute /bin/true if 100=1-1000 "
			 "exec.envp[\"HOME\"]!=\"/\"", argv, envp, 1);
		try_exec("allow_execute /bin/true if path1.parent.gid!=100 "
			 "exec.envp[\"HOME\"]!=NULL exec.envp[\"HOME3\"]=NULL",
			 argv, envp, 0);
		try_exec("allow_execute /bin/true if path1.parent.gid!=100 "
			 "exec.envp[\"HOME\"]=NULL exec.envp[\"HOME3\"]=NULL",
			 argv, envp, 1);
		try_exec("allow_execute /bin/true if path1.parent.gid=0 "
			 "exec.envp[\"HOME\"]=\"/\" exec.envp[\"HOME3\"]=NULL",
			 argv, envp, 0);
		try_exec("allow_execute /bin/true if path1.parent.gid=0 "
			 "exec.envp[\"HOME\"]!=\"/\" exec.envp[\"HOME3\"]=NULL",
			 argv, envp, 1);
	}
}

int main(int argc, char *argv[])
{
	const char *cp;
	int self_fd;
	ccs_test_init();
	self_fd = open(proc_policy_self_domain, O_RDONLY);
	domain_fd = open(proc_policy_domain_policy, O_WRONLY);
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
	cp = "allow_read/write ";
	write(domain_fd, cp, strlen(cp));
	cp = proc_policy_domain_policy;
	write(domain_fd, cp, strlen(cp));
	write(domain_fd, "\n", 1);
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	stage_exec_test();
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	clear_status();
	return 0;
}
