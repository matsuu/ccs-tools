/*
 * include.h
 *
 * Common functions for testing TOMOYO Linux's kernel.
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0-pre   2007/08/13
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

static const char *proc_policy_dir    = "/proc/ccs/",
	*proc_policy_domain_policy    = "/proc/ccs/domain_policy",
	*proc_policy_exception_policy = "/proc/ccs/exception_policy",
	*proc_policy_system_policy    = "/proc/ccs/system_policy",
	*proc_policy_profile          = "/proc/ccs/profile",
	*proc_policy_manager          = "/proc/ccs/manager",
	*proc_policy_query            = "/proc/ccs/query",
	*proc_policy_grant_log        = "/proc/ccs/grant_log",
	*proc_policy_reject_log       = "/proc/ccs/reject_log",
	*proc_policy_domain_status    = "/proc/ccs/.domain_status",
	*proc_policy_process_status   = "/proc/ccs/.process_status",
	*proc_policy_self_domain      = "/proc/ccs/self_domain";

static void PreInit(void) {
	if (access("/proc/tomoyo/", F_OK) == 0) {
		proc_policy_dir              = "/proc/tomoyo/";
		proc_policy_domain_policy    = "/proc/tomoyo/domain_policy";
		proc_policy_exception_policy = "/proc/tomoyo/exception_policy";
		proc_policy_system_policy    = "/proc/tomoyo/system_policy";
		proc_policy_profile          = "/proc/tomoyo/profile";
		proc_policy_manager          = "/proc/tomoyo/manager";
		proc_policy_query            = "/proc/tomoyo/query";
		proc_policy_grant_log        = "/proc/tomoyo/grant_log";
		proc_policy_reject_log       = "/proc/tomoyo/reject_log";
		proc_policy_domain_status    = "/proc/tomoyo/.domain_status";
		proc_policy_process_status   = "/proc/tomoyo/.process_status";
		proc_policy_self_domain      = "/proc/tomoyo/self_domain";
	}
}

static int status_fd = EOF;
static int is_kernel26 = 0;
static pid_t pid = 0;

static void WriteStatus(const char *cp) {
	write(status_fd, "255-", 4); write(status_fd, cp, strlen(cp));
}

static void ClearStatus(void) {
	FILE *fp = fopen(proc_policy_profile, "r");
	static char buffer[4096];
	if (!fp) {
		fprintf(stderr, "Can't open %s\n", proc_policy_profile);
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
	PreInit();
	pid = getpid();
	if (access(proc_policy_dir, F_OK)) {
		fprintf(stderr, "You can't use this program for this kernel.\n");
		exit(1);
	}
	if ((status_fd = open(proc_policy_profile, O_WRONLY)) == EOF) {
		fprintf(stderr, "Can't open %s .\n", proc_policy_profile);
		exit(1);
	}
	if (write(status_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to %s to run this program.\n", proc_policy_manager);
		exit(1);
	}
	ClearStatus();
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
		FILE *fp = fopen(proc_policy_self_domain, "r");
		memset(buffer, 0, sizeof(buffer));
		if (fp) {
			fgets(buffer, sizeof(buffer) - 1, fp);
			fclose(fp);
		} else exit(1);
		fp = fopen(proc_policy_domain_status, "w");
		if (fp) {
			fprintf(fp, "255 %s\n", buffer);
			fclose(fp);
		} else exit(1);
	}
}
