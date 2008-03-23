/*
 * tomoyo_cond_test.c
 *
 * Testing program for fs/tomoyo_cond.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0-pre   2008/03/23
 *
 */
#include "include.h"

static int domain_fd = EOF;

static void try_open(const char *policy, const char *file, const int mode, const char should_success) {
	int err = 0;
	int fd;
	printf("%s: ", policy);
	fflush(stdout);
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
	errno = 0;
	fd = open(file, mode, 0);
	err = errno;
	if (fd != EOF) close(fd);
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

static void StageOpenTest(void) {
	const pid_t pid = getpid();
	try_open("allow_read /etc/fstab", "/etc/fstab", O_RDONLY, 1);
	try_open("allow_write /etc/fstab", "/etc/fstab", O_WRONLY, 1);
	try_open("allow_write /etc/fstab", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab", "/etc/fstab", O_WRONLY, 0);
	try_open("allow_read/write /etc/fstab", "/etc/fstab", O_RDWR, 1);
	try_open("allow_read/write /etc/fstab", "/etc/fstab", O_RDONLY, 1);
	try_open("allow_read/write /etc/fstab", "/etc/fstab", O_WRONLY, 1);
	try_open("allow_read /etc/fstab if task.uid=0 task.euid=0", "/etc/fstab", O_RDONLY, 1);
	try_open("allow_read /etc/fstab if task.uid=0 task.euid=0-4294967295", "/etc/fstab", O_RDONLY, 1);
	try_open("allow_read /etc/fstab if task.uid=0 task.euid!=0-4294967295", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if task.uid=0 task.euid!=0", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if exec.argc=0", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if exec.envc=0", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if exec.argv[0]=\"\"", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if exec.argv[0]!=\"\"", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if exec.envp[\"HOME\"]=\"\"", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if exec.envp[\"HOME\"]!=\"\"", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if exec.envp[\"HOME\"]=NULL", "/etc/fstab", O_RDONLY, 0);
	try_open("allow_read /etc/fstab if exec.envp[\"HOME\"]!=NULL", "/etc/fstab", O_RDONLY, 0);

	try_open("allow_read /proc/\\*/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\X/mounts", "/proc/mounts", O_RDONLY, 1);

	try_open("allow_read /proc/\\+/mounts", "/proc/mounts", O_RDONLY, pid < 10);
	try_open("allow_read /proc/\\+\\+/mounts", "/proc/mounts", O_RDONLY, pid >= 10 && pid < 100);
	try_open("allow_read /proc/\\+\\+\\+/mounts", "/proc/mounts", O_RDONLY, pid >= 100 && pid < 1000);
	try_open("allow_read /proc/\\+\\+\\+\\+/mounts", "/proc/mounts", O_RDONLY, pid >= 1000 && pid < 10000);
	try_open("allow_read /proc/\\+\\+\\+\\+\\+/mounts", "/proc/mounts", O_RDONLY, pid >= 10000 && pid < 100000);
	try_open("allow_read /proc/\\+\\+\\+\\+\\+\\+/mounts", "/proc/mounts", O_RDONLY, pid >= 100000 && pid < 1000000);

	try_open("allow_read /proc/\\x/mounts", "/proc/mounts", O_RDONLY, pid < 10);
	try_open("allow_read /proc/\\x\\x/mounts", "/proc/mounts", O_RDONLY, pid >= 10 && pid < 100);
	try_open("allow_read /proc/\\x\\x\\x/mounts", "/proc/mounts", O_RDONLY, pid >= 100 && pid < 1000);
	try_open("allow_read /proc/\\x\\x\\x\\x/mounts", "/proc/mounts", O_RDONLY, pid >= 1000 && pid < 10000);
	try_open("allow_read /proc/\\x\\x\\x\\x\\x/mounts", "/proc/mounts", O_RDONLY, pid >= 10000 && pid < 100000);
	try_open("allow_read /proc/\\x\\x\\x\\x\\x\\x/mounts", "/proc/mounts", O_RDONLY, pid >= 100000 && pid < 1000000);

	try_open("allow_read /proc/\\$\\*/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*\\*/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@\\@/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*\\@/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@\\*/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*/mounts\\*", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@/mounts\\@", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*\\*/mounts\\*\\*", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@\\@/mounts\\@\\@", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*\\@/mounts\\*\\@", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@\\*/mounts\\@\\*", "/proc/mounts", O_RDONLY, 1);

	try_open("allow_read /proc/\\*\\$/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\$/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\*\\$/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\@\\$/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\@\\$/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\*\\$/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\$/\\*mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\$/\\@mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\*\\$/\\*\\*mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\@\\$/\\@\\@mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\@\\$/\\*\\@mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\*\\$/\\@\\*mounts", "/proc/mounts", O_RDONLY, 1);

	try_open("allow_read /proc/\\*\\$\\*/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\$\\@/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\*\\$\\*\\*/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\@\\$\\@\\@/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\@\\$\\*\\@/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\*\\$\\@\\*/mounts", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\$\\*/\\*mounts\\*", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\$\\@/\\@mounts\\@", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\*\\$\\*\\*/\\*\\*mounts\\*\\*", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\@\\$\\@\\@/\\@\\@mounts\\@\\@", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\@\\$\\*\\@/\\*\\@mounts\\*\\@", "/proc/mounts", O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\*\\$\\@\\*/\\@\\*mounts\\@\\*", "/proc/mounts", O_RDONLY, 1);
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
