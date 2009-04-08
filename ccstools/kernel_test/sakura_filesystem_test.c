/*
 * sakura_filesystem_test.c
 *
 * Testing program for fs/sakura_mount.c fs/sakura_umount.c fs/sakura_maymount.c
 * fs/sakura_chroot.c fs/sakura_pivot.c
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7+   2009/04/08
 *
 */
#define _GNU_SOURCE
#include "include.h"

static void show_prompt(const char *str, const int is_enforce)
{
	printf("Testing %60s: (%s) ", str,
	       is_enforce ? "must fail" : "should success");
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

static int child(void *arg)
{
	errno = 0;
	pivot_root(pivot_root_dir, proc_policy_dir);
	return errno;
}

static int system_fd = EOF;

static void write_policy(const char *cp)
{
	write(system_fd, cp, strlen(cp));
}

int main(int argc, char *argv[])
{
	char c = 0;
	ccs_test_init();
	if (strncmp(proc_policy_dir, "/proc/", 6))
		pivot_root_dir = "/sys/kernel/security/";
	system_fd = open(proc_policy_system_policy, O_RDWR);
	if (system_fd == EOF) {
		fprintf(stderr, "You can't use this program for this kernel."
			"\n");
		return 1;
	}
	if (write(system_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to %s to "
			"run this program.\n", proc_policy_manager);
		return 1;
	}

	mkdir(TEST_DIR, 0755);
	mkdir(TEST_DIR_BIND, 0755);
	mkdir(TEST_DIR_MOVE, 0755);

	/* Test mount(). */
	{
		static char buf[4096];
		char *dev_ram_path = canonicalize_file_name("/dev/ram0");
		if (!dev_ram_path)
			dev_ram_path = canonicalize_file_name("/dev/ram");
		if (!dev_ram_path) {
			dev_ram_path = "/dev/ram0";
			mknod(dev_ram_path, S_IFBLK, MKDEV(1, 0));
		}
		memset(buf, 0, sizeof(buf));
		write_status("RESTRICT_MOUNT=enforcing\n");

		/* Test standard case */
		show_prompt("mount('none', '" TEST_DIR "', 'tmpfs') for '"
			   TEST_DIR "'", 1);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == EOF &&
		    errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		/* Test device_name with pattern */
		snprintf(buf, sizeof(buf) - 1, "mount('%s', '" TEST_DIR
			 "', 'ext2') for '%s\\*'", dev_ram_path, dev_ram_path);
		show_prompt(buf, 1);
		if (mount(dev_ram_path, TEST_DIR, "ext2", MS_RDONLY, NULL)
		    == EOF && errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		/* Test dir_name with pattern */
		show_prompt("mount('none', '" TEST_DIR "', 'tmpfs') for '"
			   TEST_DIR_PATTERN "'", 1);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == EOF &&
		    errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		/* Test standard case */
		write_policy("allow_mount none " TEST_DIR " tmpfs 0\n");
		show_prompt("mount('none', '" TEST_DIR "', 'tmpfs') for '"
			   TEST_DIR "'", 0);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
		write_policy("delete allow_mount none " TEST_DIR " tmpfs 0\n");

		/* Test device_name with pattern */
		snprintf(buf, sizeof(buf) - 1, "allow_mount %s\\* " TEST_DIR
			 " ext2 1\n", dev_ram_path);
		write_policy(buf);
		snprintf(buf, sizeof(buf) - 1, "mount('%s', '" TEST_DIR
			 "', 'ext2') for '%s\\*'", dev_ram_path, dev_ram_path);
		show_prompt(buf, 0);
		if (mount(dev_ram_path, TEST_DIR, "ext2", MS_RDONLY, NULL) == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
		snprintf(buf, sizeof(buf) - 1, "delete allow_mount %s\\* "
			 TEST_DIR " ext2 1\n", dev_ram_path);
		write_policy(buf);

		/* Test dir_name with pattern */
		write_policy("allow_mount none " TEST_DIR_PATTERN " tmpfs 0\n");
		show_prompt("mount('none', '" TEST_DIR "', 'tmpfs') for '"
			   TEST_DIR_PATTERN "'", 0);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
		write_policy("delete allow_mount none " TEST_DIR_PATTERN
			    " tmpfs 0\n");

		write_status("RESTRICT_MOUNT=disabled\n");
		while (umount(TEST_DIR) == 0)
			c++; /* Dummy. */
	}

	/* Test mount(). */
	{
		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		write_status("RESTRICT_MOUNT=enforcing\n");

		/* Test remount case */
		show_prompt("mount('" TEST_DIR "', MS_REMOUNT)", 1);
		if (mount("none", TEST_DIR, "tmpfs", MS_REMOUNT, NULL) == EOF
		    && errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		/* Test bind case */
		show_prompt("mount('" TEST_DIR "', '" TEST_DIR_BIND
			   "', MS_BIND)", 1);
		if (mount(TEST_DIR, TEST_DIR_BIND, NULL, MS_BIND, NULL) == EOF
		    && errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		/* Test move case */
		show_prompt("mount('" TEST_DIR "', '" TEST_DIR_MOVE
			   "', MS_MOVE)", 1);
		if (mount(TEST_DIR, TEST_DIR_MOVE, NULL, MS_MOVE, NULL) == EOF
		    && errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		/* Test remount case */
		write_policy("allow_mount any " TEST_DIR " --remount 0\n");
		show_prompt("mount('" TEST_DIR "', MS_REMOUNT)", 0);
		if (mount("none", TEST_DIR, "tmpfs", MS_REMOUNT, NULL) == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
		write_policy("delete allow_mount any " TEST_DIR
			    " --remount 0\n");

		/* Test bind case */
		write_policy("allow_mount " TEST_DIR " " TEST_DIR_BIND
			    " --bind 0\n");
		show_prompt("mount('" TEST_DIR "', '" TEST_DIR_BIND
			   "', MS_BIND)", 0);
		if (mount(TEST_DIR, TEST_DIR_BIND, NULL, MS_BIND, NULL) == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
		umount(TEST_DIR_BIND);
		write_policy("delete allow_mount " TEST_DIR " " TEST_DIR_BIND
			    " --bind 0\n");

		/* Test move case */
		write_policy("allow_mount " TEST_DIR " " TEST_DIR_MOVE
			    " --move 0\n");
		show_prompt("mount('" TEST_DIR "', '" TEST_DIR_MOVE
			   "', MS_MOVE)", 0);
		if (mount(TEST_DIR, TEST_DIR_MOVE, NULL, MS_MOVE, NULL) == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
		umount(TEST_DIR_MOVE);
		write_policy("delete allow_mount " TEST_DIR " " TEST_DIR_MOVE
			    " --move 0\n");

		write_status("RESTRICT_MOUNT=disabled\n");
		while (umount(TEST_DIR) == 0)
			c++; /* Dummy. */
	}

	/* Test mount(). */
	{
		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		write_status("DENY_CONCEAL_MOUNT=enforcing\n");

		show_prompt("mount('none', '" TEST_DIR "', 'tmpfs')", 1);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == EOF &&
		    errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		show_prompt("mount('none', '/tmp/', 'tmpfs')", 1);
		if (mount("none", "/tmp/", "tmpfs", 0, NULL) == EOF &&
		    errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		show_prompt("mount('none', '/', 'tmpfs')", 1);
		if (mount("none", "/", "tmpfs", 0, NULL) == EOF &&
		    errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		write_status("DENY_CONCEAL_MOUNT=permissive\n");

		show_prompt("mount('none', '" TEST_DIR "', 'tmpfs')", 0);
		if (mount("none", TEST_DIR, "tmpfs", 0, NULL) == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));

		write_status("DENY_CONCEAL_MOUNT=disabled\n");
		while (umount(TEST_DIR) == 0)
			c++; /* Dummy. */
	}

	/* Test umount(). */
	{
		write_status("RESTRICT_UNMOUNT=enforcing\n");

		/* Test standard case */
		write_policy("deny_unmount " TEST_DIR "\n");

		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		show_prompt("umount('" TEST_DIR "') for '" TEST_DIR "'", 1);
		if (umount(TEST_DIR) == EOF && errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));
		write_policy("delete deny_unmount " TEST_DIR "\n");

		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		show_prompt("umount('" TEST_DIR "') for '" TEST_DIR "'", 0);
		if (umount(TEST_DIR) == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));

		/* Test pattern */
		write_policy("deny_unmount " TEST_DIR_PATTERN "\n");
		mount("none", TEST_DIR, "tmpfs", 0, NULL);
		show_prompt("umount('" TEST_DIR "') for '" TEST_DIR_PATTERN "'",
			   1);
		if (umount(TEST_DIR) == EOF && errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));
		write_policy("delete deny_unmount " TEST_DIR_PATTERN "\n");

		write_status("RESTRICT_UNMOUNT=disabled\n");
		while (umount(TEST_DIR) == 0)
			c++; /* Dummy. */
	}

	/* Test chroot(). */
	{
		write_status("RESTRICT_CHROOT=enforcing\n");

		/* Test standard case */
		write_policy("allow_chroot " TEST_DIR "\n");
		show_prompt("chroot('" TEST_DIR "') for '" TEST_DIR "'", 0);
		fflush(stdout);
		if (fork() == 0) {
			if (chroot(TEST_DIR) == 0)
				printf("OK\n");
			else
				printf("FAILED: %s\n", strerror(errno));
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
		write_policy("delete allow_chroot " TEST_DIR "\n");

		show_prompt("chroot('" TEST_DIR "') for '" TEST_DIR "'", 1);
		fflush(stdout);
		if (fork() == 0) {
			if (chroot(TEST_DIR) == EOF && errno == EPERM)
				printf("OK: Permission denied.\n");
			else
				printf("BUG: %s\n", strerror(errno));
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);

		/* Test pattern */
		write_policy("allow_chroot " TEST_DIR_PATTERN "\n");
		show_prompt("chroot('" TEST_DIR "') for '" TEST_DIR_PATTERN "'",
			   0);
		fflush(stdout);
		if (fork() == 0) {
			if (chroot(TEST_DIR) == 0)
				printf("OK\n");
			else
				printf("FAILED: %s\n", strerror(errno));
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
		write_policy("delete allow_chroot " TEST_DIR_PATTERN "\n");

		write_status("RESTRICT_CHROOT=disabled\n");
	}

	/* Test pivot_root(). */
	{
		int error;
		char *stack = malloc(8192);
		write_status("RESTRICT_PIVOT_ROOT=enforcing\n");

		snprintf(stack, 8191, "allow_pivot_root %s %s\n",
			 pivot_root_dir, proc_policy_dir);
		write_policy(stack);
		snprintf(stack, 8191, "pivot_root('%s', '%s')", pivot_root_dir,
			 proc_policy_dir);
		show_prompt(stack, 0);
		{
			const pid_t pid = clone(child, stack + (8192 / 2),
						CLONE_NEWNS, NULL);
			while (waitpid(pid, &error, __WALL) == EOF &&
			       errno == EINTR)
				c++; /* Dummy. */
		}
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		if (errno == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));

		snprintf(stack, 8191, "delete allow_pivot_root %s %s\n",
			 pivot_root_dir, proc_policy_dir);
		write_policy(stack);
		snprintf(stack, 8191, "pivot_root('%s', '%s')", pivot_root_dir,
			 proc_policy_dir);
		show_prompt(stack, 1);
		{
			const pid_t pid = clone(child, stack + (8192 / 2),
						CLONE_NEWNS, NULL);
			while (waitpid(pid, &error, __WALL) == EOF &&
			       errno == EINTR)
				c++; /* Dummy. */
		}
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		if (errno == EPERM)
			printf("OK: Permission denied.\n");
		else
			printf("BUG: %s\n", strerror(errno));

		write_status("RESTRICT_PIVOT_ROOT=permissive\n");
		snprintf(stack, 8191, "pivot_root('%s', '%s')", pivot_root_dir,
			 proc_policy_dir);
		show_prompt(stack, 0);
		{
			const pid_t pid = clone(child, stack + (8192 / 2),
						CLONE_NEWNS, NULL);
			while (waitpid(pid, &error, __WALL) == EOF &&
			       errno == EINTR)
				c++; /* Dummy. */
		}
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		if (errno == 0)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));

		write_status("RESTRICT_PIVOT_ROOT=disabled\n");

		free(stack);
	}

	rmdir(TEST_DIR_MOVE);
	rmdir(TEST_DIR_BIND);
	rmdir(TEST_DIR);

	close(system_fd);
	clear_status();
	return 0;
}
