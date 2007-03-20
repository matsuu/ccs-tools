/*
 * include.h
 *
 * Common functions for testing TOMOYO Linux's kernel.
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.3.1   2006/12/08
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/kdev_t.h>
struct module;
#include <linux/reboot.h>
#include <linux/unistd.h>
#include <netinet/in.h>
#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <sched.h>

#ifndef __NR_sys_kexec_load
#ifdef __NR_kexec_load
#define __NR_sys_kexec_load  __NR_kexec_load
#endif
#endif

static pid_t gettid(void) { return syscall(__NR_gettid); }
static int uselib(const char *library) { return syscall(__NR_uselib, library); }
static caddr_t create_module(const char *name, size_t size) { return (caddr_t) syscall(__NR_create_module, name, size); }
static int pivot_root(const char *new_root, const char *put_old) { return syscall(__NR_pivot_root, new_root, put_old); }
static int tkill(int tid, int sig) { return syscall(__NR_tkill, tid, sig); }
#ifdef __NR_tgkill
static int tgkill(int tgid, int tid, int sig) { return syscall(__NR_tgkill, tgid, tid, sig); }
#endif
#ifdef __NR_sys_kexec_load
struct kexec_segment;
static long sys_kexec_load(unsigned long entry, unsigned long nr_segments, struct kexec_segment *segments, unsigned long flags) { return (long) syscall(__NR_sys_kexec_load, entry, nr_segments, segments, flags); }
#endif
int reboot(int magic, int magic2, int flag, void *arg);
int init_module(const char *name, struct module *image);
int delete_module(const char *name);

static int status_fd = EOF;
static int is_kernel26 = 0;
static pid_t pid = 0;

static void WriteStatus(const char *cp) {
	write(status_fd, "255-", 4); write(status_fd, cp, strlen(cp));
}

static void ClearStatus(void) {
	FILE *fp = fopen("/proc/ccs/status", "r");
	static char buffer[4096];
	if (!fp) {
		fprintf(stderr, "Can't open /proc/ccs/status\n");
		exit(1);
	}
	while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 10, fp)) {
		char *cp = strchr(buffer, '=');
		if (!cp) continue; *cp = '\0';
		cp = strchr(buffer, '-');
		if (!cp) continue; *cp++ = '\0';
		if (strcmp(buffer, "0")) continue;
		//if (strcmp(cp, "TOMOYO_VERBOSE") == 0) continue;
		write(status_fd, "255-", 4);
		write(status_fd, cp, strlen(cp));
		if (strcmp(cp, "COMMENT") == 0) {
			const char *cmd = "=Profile for kernel test\n";
			write(status_fd, cmd, strlen(cmd)); continue;
		}
		write(status_fd, "=0\n", 3);
	}
	fclose(fp);
}

static void Init(void) {
	pid = getpid();
	if (access("/proc/ccs/", F_OK)) {
		fprintf(stderr, "You can't use this program for this kernel.\n");
		exit(1);
	}
	if ((status_fd = open("/proc/ccs/status", O_WRONLY)) == EOF) {
		fprintf(stderr, "Can't open /proc/ccs/status .\n");
		exit(1);
	}
	if (write(status_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to /proc/ccs/policy/manager to run this program.\n");
		exit(1);
	}
	ClearStatus();
	{
		FILE *fp = fopen("/proc/ccs/info/trusted_pids", "r");
		if (fp) {
			const pid_t self = getpid();
			unsigned int pid;
			while (fscanf(fp, "%u", &pid) == 1) {
				if (self == pid) {
					fprintf(stderr, "You can't use this program inside trusted domain.\n");
					exit(1);
				}
			}
			fclose(fp);
		}
	}
	{
		FILE *fp = fopen("/proc/sys/kernel/osrelease", "r");
		int version = 0;
		if (!fp || fscanf(fp, "2.%d.", &version) != 1) {
			fprintf(stderr, "Can't read /proc/sys/kernel/osrelease\n");
			exit(1);
		}
		fclose(fp);
		if (version == 6) is_kernel26 = 1;
	}
	{
		char buffer[4096];
		FILE *fp = fopen("/proc/ccs/info/self_domain", "r");
		memset(buffer, 0, sizeof(buffer));
		if (fp) {
			fgets(buffer, sizeof(buffer) - 1, fp);
			fclose(fp);
		} else exit(1);
		fp = fopen("/proc/ccs/policy/.domain_status", "w");
		if (fp) {
			fprintf(fp, "255 %s\n", buffer);
			fclose(fp);
		} else exit(1);
	}
}
