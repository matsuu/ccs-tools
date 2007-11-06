/*
 * sakura_filesystem_test.c
 *
 * Testing program for fs/sakura_mount.c fs/sakura_umount.c fs/sakura_maymount.c fs/sakura_chroot.c fs/sakura_pivot.c
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0   2007/09/20
 *
 */
#include "include.h"

static void ShowPrompt(const char *str, const int is_enforce) {
	printf("Testing %60s: (%s) ", str, is_enforce ? "must fail" : "should success");
	errno = 0;
}

#define TEST_DIR         "/tmp/mount/"
#define TEST_DIR_PATTERN "/tmp/\\?\\?\\?\\?\\?/"
#define TEST_DIR_BIND    "/tmp/mount_bind/"
#define TEST_DIR_MOVE    "/tmp/mount_move/"

#ifndef MS_MOVE
#define MS_MOVE         8192
#endif

static const char *pivot_root_dir = "/proc/"; 

static int child(void *arg) {
	errno = 0;
	pivot_root(pivot_root_dir, proc_policy_dir);
	return errno;
}

static int system_fd = EOF;

static void WritePolicy(const char *cp) {
	write(system_fd, cp, strlen(cp));			
}

int main(int argc, char *argv[]) {
	Init();
	if (strncmp(proc_policy_dir, "/proc/", 6)) pivot_root_dir = "/sys/kernel/security/";
	if ((system_fd = open(proc_policy_system_policy, O_RDWR)) == EOF) {
		fprintf(stderr, "Can't open %s .\n", proc_policy_system_policy);
		return 1;
	}
	if (write(system_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to %s to run this program.\n", proc_policy_manager);
		return 1;
	}
	
	mkdir(TEST_DIR, 0755);
	mkdir(TEST_DIR_BIND, 0755);
	mkdir(TEST_DIR_MOVE, 0755);
	
	// Test mount().
	{
		WriteStatus("RESTRICT_MOUNT=3\n");

		// Test standard case
		ShowPrompt("mount('none', '" TEST_DIR "', 'tmpfs') for '" TEST_DIR "'", 1);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));

		// Test device_name with pattern
		ShowPrompt("mount('/dev/ram0', '" TEST_DIR "', 'ext2') for '/dev/\\*'", 1);
		if (mount("/dev/ram0", TEST_DIR, "ext2", MS_RDONLY, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));
		
		// Test dir_name with pattern
		ShowPrompt("mount('none', '" TEST_DIR "', 'tmpfs') for '" TEST_DIR_PATTERN "'", 1);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));		
		
		// Test standard case
		WritePolicy("allow_mount none " TEST_DIR " tmpfs 0\n");
		ShowPrompt("mount('none', '" TEST_DIR "', 'tmpfs') for '" TEST_DIR "'", 0);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
		WritePolicy("delete allow_mount none " TEST_DIR " tmpfs 0\n");

		// Test device_name with pattern
		WritePolicy("allow_mount /dev/\\* " TEST_DIR " ext2 1\n");
		ShowPrompt("mount('/dev/ram0', '" TEST_DIR "', 'ext2') for '/dev/\\*'", 0);
		if (mount("/dev/ram0", TEST_DIR, "ext2", MS_RDONLY, NULL) == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
		WritePolicy("delete allow_mount /dev/\\* " TEST_DIR " ext2 1\n");
			
		// Test dir_name with pattern
		WritePolicy("allow_mount none " TEST_DIR_PATTERN " tmpfs 0\n");
		ShowPrompt("mount('none', '" TEST_DIR "', 'tmpfs') for '" TEST_DIR_PATTERN "'", 0);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
		WritePolicy("delete allow_mount none " TEST_DIR_PATTERN " tmpfs 0\n");

		WriteStatus("RESTRICT_MOUNT=0\n");
		while (umount(TEST_DIR) == 0);
	}
	
	// Test mount().
	{
		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		WriteStatus("RESTRICT_MOUNT=3\n");

		// Test remount case
		ShowPrompt("mount('" TEST_DIR "', MS_REMOUNT)", 1);
		if (mount("none", TEST_DIR, "tmpfs", MS_REMOUNT, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));

		// Test bind case
		ShowPrompt("mount('" TEST_DIR "', '" TEST_DIR_BIND "', MS_BIND)", 1);
		if (mount(TEST_DIR, TEST_DIR_BIND, NULL, MS_BIND, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));

		// Test move case
		ShowPrompt("mount('" TEST_DIR "', '" TEST_DIR_MOVE "', MS_MOVE)", 1);
		if (mount(TEST_DIR, TEST_DIR_MOVE, NULL, MS_MOVE, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));
		
		// Test remount case
		WritePolicy("allow_mount any " TEST_DIR " --remount 0\n");
		ShowPrompt("mount('" TEST_DIR "', MS_REMOUNT)", 0);
		if (mount("none", TEST_DIR, "tmpfs", MS_REMOUNT, NULL) == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
		WritePolicy("delete allow_mount any " TEST_DIR " --remount 0\n");

		// Test bind case
		WritePolicy("allow_mount " TEST_DIR " " TEST_DIR_BIND " --bind 0\n");
		ShowPrompt("mount('" TEST_DIR "', '" TEST_DIR_BIND "', MS_BIND)", 0);
		if (mount(TEST_DIR, TEST_DIR_BIND, NULL, MS_BIND, NULL) == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
		umount(TEST_DIR_BIND);
		WritePolicy("delete allow_mount " TEST_DIR " " TEST_DIR_BIND " --bind 0\n");

		// Test move case
		WritePolicy("allow_mount " TEST_DIR " " TEST_DIR_MOVE " --move 0\n");
		ShowPrompt("mount('" TEST_DIR "', '" TEST_DIR_MOVE "', MS_MOVE)", 0);
		if (mount(TEST_DIR, TEST_DIR_MOVE, NULL, MS_MOVE, NULL) == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
		umount(TEST_DIR_MOVE);
		WritePolicy("delete allow_mount " TEST_DIR " " TEST_DIR_MOVE " --move 0\n");

		WriteStatus("RESTRICT_MOUNT=0\n");
		while (umount(TEST_DIR) == 0);
	}
	
	// Test mount().
	{
		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		WriteStatus("DENY_CONCEAL_MOUNT=3\n");
		
		ShowPrompt("mount('none', '" TEST_DIR "', 'tmpfs')", 1);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));
		
		ShowPrompt("mount('none', '/tmp/', 'tmpfs')", 1);
		if (mount("none", "/tmp/", "tmpfs", 0, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));
		
		ShowPrompt("mount('none', '/', 'tmpfs')", 1);
		if (mount("none", "/", "tmpfs", 0, NULL) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));

		WriteStatus("DENY_CONCEAL_MOUNT=2\n");

		ShowPrompt("mount('none', '" TEST_DIR "', 'tmpfs')", 0);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));

		WriteStatus("DENY_CONCEAL_MOUNT=0\n");
		while (umount(TEST_DIR) == 0);
	}

	// Test umount().
	{
		WriteStatus("RESTRICT_UNMOUNT=3\n");
		
		// Test standard case
		WritePolicy("deny_unmount " TEST_DIR "\n");
		
		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		ShowPrompt("umount('" TEST_DIR "') for '" TEST_DIR "'", 1);
		if (umount(TEST_DIR) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));
		WritePolicy("delete deny_unmount " TEST_DIR "\n");
		
		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		ShowPrompt("umount('" TEST_DIR "') for '" TEST_DIR "'", 0);
		if (umount(TEST_DIR) == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno)); 
		
		// Test pattern
		WritePolicy("deny_unmount " TEST_DIR_PATTERN "\n");
		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		ShowPrompt("umount('" TEST_DIR "') for '" TEST_DIR_PATTERN "'", 1);
		if (umount(TEST_DIR) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));
		WritePolicy("delete deny_unmount " TEST_DIR_PATTERN "\n");

		WriteStatus("RESTRICT_UNMOUNT=0\n");
		while (umount(TEST_DIR) == 0);
	}

	// Test chroot().
	{
		WriteStatus("RESTRICT_CHROOT=3\n");
		
		// Test standard case
		WritePolicy("allow_chroot " TEST_DIR "\n");
		ShowPrompt("chroot('" TEST_DIR "') for '" TEST_DIR "'", 0); fflush(stdout);
		if (fork() == 0) {
			if (chroot(TEST_DIR) == 0) printf("OK\n");
			else printf("FAILED: %s\n", strerror(errno));
			fflush(stdout); _exit(0);
		}
		wait(NULL);
		WritePolicy("delete allow_chroot " TEST_DIR "\n");
		
		ShowPrompt("chroot('" TEST_DIR "') for '" TEST_DIR "'", 1); fflush(stdout);
		if (fork() == 0) {
			if (chroot(TEST_DIR) == EOF && errno == EPERM) printf("OK: Permission denied.\n");
			else printf("BUG: %s\n", strerror(errno)); 
			fflush(stdout); _exit(0);
		}
		wait(NULL);
		
		// Test pattern
		WritePolicy("allow_chroot " TEST_DIR_PATTERN "\n");
		ShowPrompt("chroot('" TEST_DIR "') for '" TEST_DIR_PATTERN "'", 0); fflush(stdout);
		if (fork() == 0) {
			if (chroot(TEST_DIR) == 0) printf("OK\n");
			else printf("FAILED: %s\n", strerror(errno));
			fflush(stdout); _exit(0);
		}
		wait(NULL);
		WritePolicy("delete allow_chroot " TEST_DIR_PATTERN "\n");

		WriteStatus("RESTRICT_CHROOT=0\n");
	}

	// Test pivot_root().
	{
		int error;
		char *stack = malloc(8192);
		WriteStatus("RESTRICT_PIVOT_ROOT=3\n");

		snprintf(stack, 8191, "allow_pivot_root %s %s\n", pivot_root_dir, proc_policy_dir);
		WritePolicy(stack);
		snprintf(stack, 8191, "pivot_root('%s', '%s')", pivot_root_dir, proc_policy_dir);
		ShowPrompt(stack, 0);
		{
			const pid_t pid = clone(child, stack + (8192 / 2), CLONE_NEWNS, NULL);
			while (waitpid(pid, &error, __WALL) == EOF && errno == EINTR);
		}
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		if (errno == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));

		snprintf(stack, 8191, "delete allow_pivot_root %s %s\n", pivot_root_dir, proc_policy_dir);
		WritePolicy(stack);
		snprintf(stack, 8191, "pivot_root('%s', '%s')", pivot_root_dir, proc_policy_dir);
		ShowPrompt(stack, 1);
		{
			const pid_t pid = clone(child, stack + (8192 / 2), CLONE_NEWNS, NULL);
			while (waitpid(pid, &error, __WALL) == EOF && errno == EINTR);
		}
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		if (errno == EPERM) printf("OK: Permission denied.\n");
		else printf("BUG: %s\n", strerror(errno));
		
		WriteStatus("RESTRICT_PIVOT_ROOT=2\n");
		snprintf(stack, 8191, "pivot_root('%s', '%s')", pivot_root_dir, proc_policy_dir);
		ShowPrompt(stack, 0);
		{
			const pid_t pid = clone(child, stack + (8192 / 2), CLONE_NEWNS, NULL);
			while (waitpid(pid, &error, __WALL) == EOF && errno == EINTR);
		}
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		if (errno == 0) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
		
		WriteStatus("RESTRICT_PIVOT_ROOT=0\n");

		free(stack);
	}
	
	rmdir(TEST_DIR_MOVE);
	rmdir(TEST_DIR_BIND);
	rmdir(TEST_DIR);
	
	close(system_fd);
	ClearStatus();
	return 0;
}
