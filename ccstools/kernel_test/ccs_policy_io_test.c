/*
 * policy_io_test.c
 *
 * Testing program for policy parser.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0-rc   2008/03/27
 *
 */
#include "include.h"

static int fd = EOF;
static const char *policy_file = "";

static void try_io(const char *policy, const char should_success) {
	FILE *fp = fopen(policy_file, "r");
	char buffer[8192];
	int policy_found = 0;
	memset(buffer, 0, sizeof(buffer));
	printf("%s: ", policy);
	write(fd, policy, strlen(policy));
	write(fd, "\n", 1);
	if (!fp) {
		printf("BUG: policy read failed\n");
		return;
	}
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		char *cp = strchr(buffer, '\n');
		if (cp) *cp = '\0';
		if (!strcmp(buffer, policy)) {
			policy_found = 1;
			break;
		}
	}
	fclose(fp);
	if (should_success) {
		if (policy_found) printf("OK\n");
		else printf("BUG: policy write failed\n");
	} else {
		if (!policy_found) printf("OK : write rejected.\n");
		else printf("BUG: policy write not rejected.\n");
	}
	write(fd, "delete ", 7);
	write(fd, policy, strlen(policy));
	write(fd, "\n", 1);
}

static void StagePolicyIOTest(void) {
	policy_file = proc_policy_system_policy;
	fd = open(policy_file, O_WRONLY);
	try_io("allow_chroot /", 1);
	try_io("allow_chroot ", 0);
	try_io("allow_chroot *", 0);
	try_io("allow_chroot /mnt0/", 1);
	try_io("allow_chroot /var1/chroot2/", 1);
	try_io("allow_chroot /bin3", 0);
	try_io("allow_chroot 4foo/", 0);
	try_io("allow_chroot bar5", 0);
	try_io("allow_chroot /mnt0/", 1);
	try_io("allow_chroot /mnt0/", 1);
	try_io("allow_chroot /mnt0/", 1);
	try_io("allow_chroot /mnt\\?\\*/", 1);
	try_io("allow_chroot /mnt\\?\\*/", 1);
	try_io("deny_autobind 0-65535", 1);
	try_io("deny_autobind 0-65536", 0);
	try_io("deny_autobind 65-100", 0);
	try_io("deny_autobind 100-65", 0);
	try_io("deny_autobind 500", 1);
	try_io("deny_autobind 65535", 1);
	try_io("deny_autobind 65536", 0);
	try_io("deny_autobind *", 0);
	try_io("deny_autobind 500", 1);
	try_io("deny_autobind 0-65535", 1);
	try_io("deny_autobind 500", 1);
	try_io("deny_autobind 0-65535", 1);
	try_io("deny_unmount /", 1);
	try_io("deny_unmount /proc0", 0);
	try_io("deny_unmount /sys1/", 1);
	try_io("deny_unmount /initrd2/", 1);
	try_io("deny_unmount /initrd/dev3/", 1);
	try_io("deny_unmount /initrd4/root", 0);
	try_io("deny_unmount foo5/", 0);
	try_io("deny_unmount bar6", 0);
	try_io("deny_unmount /initrd/\\*\\+/", 1);
	try_io("deny_unmount /initrd/\\@\\*/", 1);
	try_io("deny_unmount *", 0);
	try_io("deny_unmount /initrd2/", 1);
	try_io("allow_pivot_root / /proc3/", 1);
	try_io("allow_pivot_root / /proc3", 0);
	try_io("allow_pivot_root /foo /proc3/", 0);
	try_io("allow_pivot_root /sys5/ /proc3/", 1);
	try_io("allow_pivot_root bar *", 0);
	try_io("allow_pivot_root /sys/", 0);
	try_io("allow_pivot_root *", 0);
	try_io("allow_pivot_root /sys5/ /proc3/", 1);
	try_io("allow_mount / / --bind 0xD", 1);
	try_io("allow_mount / / --move 0xF", 1);
	try_io("allow_mount / --remount", 0);
	try_io("allow_mount /", 0);
	try_io("allow_mount none /tmp/ tmpfs 0", 1);
	try_io("allow_mount none /tmp/ tmpfs", 0);
	try_io("allow_mount none /tmp/ nonexistent 0", 1);
	try_io("allow_mount none /proc/ proc 0x0", 1);
	try_io("allow_mount none /selinux/ selinuxfs 0x0", 1);
	try_io("allow_mount /proc/bus/usb /proc/bus/usb/ usbfs 0x0", 1);
	try_io("allow_mount none /dev/pts/ devpts 0x0", 1);
	try_io("allow_mount any / --remount 0xC00", 1);
	try_io("allow_mount /dev/sda1 /boot/ ext3 0xC00", 1);
	try_io("allow_mount none /dev/shm/ tmpfs 0x0", 1);
	try_io("allow_mount none /proc/sys/fs/binfmt_misc/ binfmt_misc 0x0", 1);
	try_io("allow_mount none /proc/sys/fs/binfmt_misc/ binfmt_misc 0x0 0x1", 0);
	try_io("allow_mount none /proc/sys/fs/binfmt_misc/ tmpfs binfmt_misc 0x0", 0);
	try_io("allow_mount /proc/bus/usb /proc/bus/usb/ usbfs 0x0", 1);
	close(fd);
	policy_file = proc_policy_exception_policy;
	fd = open(policy_file, O_WRONLY);
	try_io("", 0);
	// keep_domain
	// no_keep_domain
	// initialize_domain
	// no_initialize_domain
	// keep_domain from
	// no_keep_domain from
	// initialize_domain from
	// no_initialize_domain from
	// file_pattern
	// path_group
	// address_group
	// allow_read
	// allow_env
	// deny_rewrite
	// alias
	// aggregator
	close(fd);
}

int main(int argc, char *argv[]) {
	Init();
	StagePolicyIOTest();
	return 0;
}
