/*
 * tomoyo_cond_test.c
 *
 * Testing program for fs/tomoyo_cond.c
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-rc   2009/03/04
 *
 */
#include "include.h"

static int domain_fd = EOF;
static char self_domain[4096];

static void try_open(const char *policy, const char *file, const int mode,
		     const char should_success) {
	FILE *fp;
	char buffer[8192];
	char *cp;
	int domain_found = 0;
	int policy_found = 0;
	int err = 0;
	int fd;
	memset(buffer, 0, sizeof(buffer));
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	fp = fopen(proc_policy_domain_policy, "r+");
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	printf("%s: ", policy);
	fflush(stdout);
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
	if (!fp) {
		printf("BUG: policy read failed\n");
		return;
	}
	fprintf(fp, "select pid=%d\n", pid);
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
	errno = 0;
	fd = open(file, mode, 0);
	err = errno;
	if (fd != EOF)
		close(fd);
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

static void stage_open_test(void)
{
	const pid_t pid = getppid();
	int i;
	char buffer[128];
	for (i = 0; i < 5; i++) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/mounts", pid);
		try_open("allow_read /etc/fstab", "/etc/fstab", O_RDONLY, 1);
		try_open("allow_write /etc/fstab", "/etc/fstab", O_WRONLY, 1);
		try_open("allow_write /etc/fstab", "/etc/fstab", O_RDONLY, 0);
		try_open("allow_read /etc/fstab", "/etc/fstab", O_WRONLY, 0);
		try_open("allow_read/write /etc/fstab", "/etc/fstab", O_RDWR,
			 1);
		try_open("allow_read/write /etc/fstab", "/etc/fstab", O_RDONLY,
			 1);
		try_open("allow_read/write /etc/fstab", "/etc/fstab", O_WRONLY,
			 1);
		try_open("allow_read /etc/fstab if task.uid=0 task.euid=0",
			 "/etc/fstab", O_RDONLY, 1);
		try_open("allow_read /etc/fstab "
			 "if task.uid=0 task.euid=0-4294967295", "/etc/fstab",
			 O_RDONLY, 1);
		try_open("allow_read /etc/fstab "
			 "if task.uid=0 task.euid!=0-4294967295", "/etc/fstab",
			 O_RDONLY, 0);
		try_open("allow_read /etc/fstab if task.uid=0 task.euid!=0",
			 "/etc/fstab", O_RDONLY, 0);
		try_open("allow_read /etc/fstab if exec.argc=0", "/etc/fstab",
			 O_RDONLY, 0);
		try_open("allow_read /etc/fstab if exec.envc=0", "/etc/fstab",
			 O_RDONLY, 0);
		try_open("allow_read /etc/fstab if exec.argv[0]=\"\"",
			 "/etc/fstab", O_RDONLY, 0);
		try_open("allow_read /etc/fstab if exec.argv[0]!=\"\"",
			 "/etc/fstab", O_RDONLY, 0);
		try_open("allow_read /etc/fstab if exec.envp[\"HOME\"]=\"\"",
			 "/etc/fstab", O_RDONLY, 0);
		try_open("allow_read /etc/fstab if exec.envp[\"HOME\"]!=\"\"",
			 "/etc/fstab", O_RDONLY, 0);
		try_open("allow_read /etc/fstab if exec.envp[\"HOME\"]=NULL",
			 "/etc/fstab", O_RDONLY, 0);
		try_open("allow_read /etc/fstab if exec.envp[\"HOME\"]!=NULL",
			 "/etc/fstab", O_RDONLY, 0);

		try_open("allow_read /proc/\\*/mounts", buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\@/mounts", buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\$/mounts", buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\X/mounts", buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\+/mounts", buffer, O_RDONLY,
			 pid >= 0 && pid < 10);
		try_open("allow_read /proc/\\+\\+/mounts", buffer, O_RDONLY,
			 pid >= 10 && pid < 100);
		try_open("allow_read /proc/\\+\\+\\+/mounts", buffer, O_RDONLY,
			 pid >= 100 && pid < 1000);
		try_open("allow_read /proc/\\+\\+\\+\\+/mounts", buffer,
			 O_RDONLY, pid >= 1000 && pid < 10000);
		try_open("allow_read /proc/\\+\\+\\+\\+\\+/mounts", buffer,
			 O_RDONLY, pid >= 10000 && pid < 100000);
		try_open("allow_read /proc/\\+\\+\\+\\+\\+\\+/mounts", buffer,
			 O_RDONLY, pid >= 100000 && pid < 1000000);

		try_open("allow_read /proc/\\x/mounts", buffer, O_RDONLY,
			 pid < 10);
		try_open("allow_read /proc/\\x\\x/mounts", buffer, O_RDONLY,
			 pid >= 10 && pid < 100);
		try_open("allow_read /proc/\\x\\x\\x/mounts", buffer, O_RDONLY,
			 pid >= 100 && pid < 1000);
		try_open("allow_read /proc/\\x\\x\\x\\x/mounts", buffer,
			 O_RDONLY, pid >= 1000 && pid < 10000);
		try_open("allow_read /proc/\\x\\x\\x\\x\\x/mounts", buffer,
			 O_RDONLY, pid >= 10000 && pid < 100000);
		try_open("allow_read /proc/\\x\\x\\x\\x\\x\\x/mounts", buffer,
			 O_RDONLY, pid >= 100000 && pid < 1000000);

		try_open("allow_read /proc/\\$\\*/mounts", buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\$\\@/mounts", buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\$\\*\\*/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\$\\@\\@/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\$\\*\\@/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\$\\@\\*/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\$\\*/mounts\\*", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\$\\@/mounts\\@", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\$\\*\\*/mounts\\*\\*", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\$\\@\\@/mounts\\@\\@", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\$\\*\\@/mounts\\*\\@", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\$\\@\\*/mounts\\@\\*", buffer,
			 O_RDONLY, 1);

		try_open("allow_read /proc/\\*\\$/mounts", buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\@\\$/mounts", buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\*\\*\\$/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\@\\@\\$/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\*\\@\\$/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\@\\*\\$/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\*\\$/\\*mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\@\\$/\\@mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\*\\*\\$/\\*\\*mounts", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\@\\@\\$/\\@\\@mounts", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\*\\@\\$/\\*\\@mounts", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\@\\*\\$/\\@\\*mounts", buffer,
			 O_RDONLY, 1);

		try_open("allow_read /proc/\\*\\$\\*/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\@\\$\\@/mounts", buffer, O_RDONLY,
			 1);
		try_open("allow_read /proc/\\*\\*\\$\\*\\*/mounts", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\@\\@\\$\\@\\@/mounts", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\*\\@\\$\\*\\@/mounts", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\@\\*\\$\\@\\*/mounts", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\*\\$\\*/\\*mounts\\*", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\@\\$\\@/\\@mounts\\@", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /proc/\\*\\*\\$\\*\\*/\\*\\*mounts\\*\\*",
			 buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\@\\@\\$\\@\\@/\\@\\@mounts\\@\\@",
			 buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\*\\@\\$\\*\\@/\\*\\@mounts\\*\\@",
			 buffer, O_RDONLY, 1);
		try_open("allow_read /proc/\\@\\*\\$\\@\\*/\\@\\*mounts\\@\\*",
			 buffer, O_RDONLY, 1);

		snprintf(buffer, sizeof(buffer) - 1, "/etc/fstab");
		try_open("allow_read /etc/fstab ; set task.state[0]=1", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /etc/fstab "
			 "if task.state[0]=0-2 ; set task.state[1]=3", buffer,
			 O_RDONLY, 1);
		try_open("allow_read /etc/fstab "
			 "if task.state[0]=1 task.state[1]=3 "
			 "; set task.state[1]=5", buffer, O_RDONLY, 1);
		try_open("allow_read /etc/fstab if task.state[0]!=1 "
			 "; set task.state[2]=254", buffer, O_RDONLY, 0);
		try_open("allow_read /etc/fstab "
			 "if task.state[0]!=2-255 task.state[1]=5-7 "
			 "; set task.state[2]=10", buffer, O_RDONLY, 1);
		try_open("allow_read /etc/fstab "
			 "if task.state[0]=4-255 task.state[1]=5-7 "
			 "; set task.state[2]=0", buffer, O_RDONLY, 0);
		try_open("allow_read /etc/fstab "
			 "if task.state[0]=1 task.state[1]=0-10 "
			 "task.state[2]!=0-9 ; set task.state[0]=0 "
			 "task.state[1]=0 task.state[2]=0", buffer, O_RDONLY,
			 1);
	}
}

static void try_signal(const char *condition, const unsigned char s0,
		       const unsigned char s1, const unsigned char s2) {
	char buffer[8192];
	int err = 0;
	int fd = open(proc_policy_process_status, O_RDWR);
	char *cp;
	int sig = (rand() % 10000) + 100;
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "select pid=%d\n", pid);
	write(domain_fd, buffer, strlen(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "allow_signal %d %s %s", sig,
		 "<kernel>", condition);
	printf("%s: ", buffer);
	fflush(stdout);
	write(domain_fd, buffer, strlen(buffer));
	write(domain_fd, "\n", 1);
	errno = 0;
	kill(1, sig);
	err = errno;
	snprintf(buffer, sizeof(buffer) - 1, "allow_signal %d %s %s", sig,
		 "<kernel>", condition);
	write(domain_fd, "delete ", 7);
	write(domain_fd, buffer, strlen(buffer));
	write(domain_fd, "\n", 1);
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "info %d\n", pid);
	write(fd, buffer, strlen(buffer));
	buffer[0] = '\0';
	read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	cp = strstr(buffer, " state[0]=");
	if (!cp || atoi(cp + 10) != s0)
		goto out;
	cp = strstr(buffer, " state[1]=");
	if (!cp || atoi(cp + 10) != s1)
		goto out;
	cp = strstr(buffer, " state[2]=");
	if (!cp || atoi(cp + 10) != s2)
		goto out;
	if (err == EINVAL)
		printf("OK. State changed.\n");
	else
		printf("BUG: failed (%d)\n", err);
	return;
 out:
	printf("BUG: state change failed: %s\n", buffer);
}

static void stage_signal_test(void)
{
	int i;
	for (i = 0; i < 5; i++) {
		try_signal("; set task.state[0]=0 task.state[1]=0 "
			   "task.state[2]=1", 0, 0, 1);
		try_signal("if task.state[0]=0 ; set task.state[0]=1", 1, 0, 1);
		try_signal("if task.state[0]=1 ; set task.state[0]=10", 10, 0,
			   1);
		try_signal("if task.state[0]=10 ; set task.state[0]=100", 100,
			   0, 1);
		try_signal("if task.state[0]=100 ; set task.state[1]=100", 100,
			   100, 1);
		try_signal("if task.state[1]=100 ; set task.state[2]=200", 100,
			   100, 200);
	}
}

int main(int argc, char *argv[])
{
	const char *cp;
	int self_fd;
	ccs_test_init();
	self_fd = open(proc_policy_self_domain, O_RDONLY);
	domain_fd = open(proc_policy_domain_policy, O_WRONLY);
	if (domain_fd == EOF && errno == ENOENT) {
		fprintf(stderr, "You can't use this program for this kernel."
			"\n");
		return 1;
	}
	memset(self_domain, 0, sizeof(self_domain));
	read(self_fd, self_domain, sizeof(self_domain) - 1);
	close(self_fd);
	write(domain_fd, self_domain, strlen(self_domain));
	write(domain_fd, "\n", 1);
	cp = "use_profile 255\n";
	write(domain_fd, cp, strlen(cp));
	cp = "ignore_global_allow_read\n";
	write(domain_fd, cp, strlen(cp));
	cp = "allow_read/write ";
	write(domain_fd, cp, strlen(cp));
	cp = proc_policy_domain_policy;
	write(domain_fd, cp, strlen(cp));
	write(domain_fd, "\n", 1);
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	stage_open_test();
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	cp = "255-MAC_FOR_SIGNAL=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	stage_signal_test();
	cp = "255-MAC_FOR_SIGNAL=disabled\n";
	write(profile_fd, cp, strlen(cp));
	clear_status();
	return 0;
}
