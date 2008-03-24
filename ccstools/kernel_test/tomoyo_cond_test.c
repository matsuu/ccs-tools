/*
 * tomoyo_cond_test.c
 *
 * Testing program for fs/tomoyo_cond.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0-pre   2008/03/24
 *
 */
#include "include.h"

static int domain_fd = EOF;
static char self_domain[4096];

static void try_open(const char *policy, const char *file, const int mode, const char should_success) {
	FILE *fp = fopen(proc_policy_domain_policy, "r");
	char buffer[8192];
	char *cp;
	int domain_found = 0;
	int policy_found = 0;
	int err = 0;
	int fd;
	memset(buffer, 0, sizeof(buffer));
	printf("%s: ", policy);
	fflush(stdout);
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
	if (!fp) {
		printf("BUG: policy read failed\n");
		return;
	}
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		cp = strchr(buffer, '\n');
		if (cp) *cp = '\0';
		if (!strncmp(buffer, "<kernel>", 8)) domain_found = !strcmp(self_domain, buffer);
		if (domain_found) {
			//printf("<%s>\n", buffer);
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
	const pid_t pid = getppid();
	char buffer[128];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/mounts", pid);
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

	try_open("allow_read /proc/\\*/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\X/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\+/mounts", buffer, O_RDONLY, pid >= 0 && pid < 10);
	try_open("allow_read /proc/\\+\\+/mounts", buffer, O_RDONLY, pid >= 10 && pid < 100);
	try_open("allow_read /proc/\\+\\+\\+/mounts", buffer, O_RDONLY, pid >= 100 && pid < 1000);
	try_open("allow_read /proc/\\+\\+\\+\\+/mounts", buffer, O_RDONLY, pid >= 1000 && pid < 10000);
	try_open("allow_read /proc/\\+\\+\\+\\+\\+/mounts", buffer, O_RDONLY, pid >= 10000 && pid < 100000);
	try_open("allow_read /proc/\\+\\+\\+\\+\\+\\+/mounts", buffer, O_RDONLY, pid >= 100000 && pid < 1000000);

	try_open("allow_read /proc/\\x/mounts", buffer, O_RDONLY, pid < 10);
	try_open("allow_read /proc/\\x\\x/mounts", buffer, O_RDONLY, pid >= 10 && pid < 100);
	try_open("allow_read /proc/\\x\\x\\x/mounts", buffer, O_RDONLY, pid >= 100 && pid < 1000);
	try_open("allow_read /proc/\\x\\x\\x\\x/mounts", buffer, O_RDONLY, pid >= 1000 && pid < 10000);
	try_open("allow_read /proc/\\x\\x\\x\\x\\x/mounts", buffer, O_RDONLY, pid >= 10000 && pid < 100000);
	try_open("allow_read /proc/\\x\\x\\x\\x\\x\\x/mounts", buffer, O_RDONLY, pid >= 100000 && pid < 1000000);

	try_open("allow_read /proc/\\$\\*/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*\\*/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@\\@/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*\\@/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@\\*/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*/mounts\\*", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@/mounts\\@", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*\\*/mounts\\*\\*", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@\\@/mounts\\@\\@", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\*\\@/mounts\\*\\@", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\$\\@\\*/mounts\\@\\*", buffer, O_RDONLY, 1);

	try_open("allow_read /proc/\\*\\$/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\$/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\*\\$/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\@\\$/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\@\\$/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\*\\$/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\$/\\*mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\$/\\@mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\*\\$/\\*\\*mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\@\\$/\\@\\@mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\@\\$/\\*\\@mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\*\\$/\\@\\*mounts", buffer, O_RDONLY, 1);

	try_open("allow_read /proc/\\*\\$\\*/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\$\\@/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\*\\$\\*\\*/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\@\\$\\@\\@/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\@\\$\\*\\@/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\*\\$\\@\\*/mounts", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\$\\*/\\*mounts\\*", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\$\\@/\\@mounts\\@", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\*\\$\\*\\*/\\*\\*mounts\\*\\*", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\@\\$\\@\\@/\\@\\@mounts\\@\\@", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\*\\@\\$\\*\\@/\\*\\@mounts\\*\\@", buffer, O_RDONLY, 1);
	try_open("allow_read /proc/\\@\\*\\$\\@\\*/\\@\\*mounts\\@\\*", buffer, O_RDONLY, 1);
}

int main(int argc, char *argv[]) {
	const char *cp;
	int profile_fd;
	int self_fd;
	Init();
	profile_fd = open(proc_policy_profile, O_WRONLY);
	self_fd = open(proc_policy_self_domain, O_RDONLY);
	domain_fd = open(proc_policy_domain_policy, O_WRONLY);
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
	StageOpenTest();
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	ClearStatus();
	return 0;
}
