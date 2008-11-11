/*
 * ccs_policy_io_test.c
 *
 * Testing program for policy parser.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5   2008/11/11
 *
 */
#include "include.h"

static int fd = EOF;
static const char *policy_file = "";

static void try_io(const char *policy, const char should_success)
{
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
		if (cp)
			*cp = '\0';
		if (!strcmp(buffer, policy)) {
			policy_found = 1;
			break;
		}
	}
	fclose(fp);
	if (should_success) {
		if (policy_found)
			printf("OK\n");
		else
			printf("BUG: policy write failed\n");
	} else {
		if (!policy_found)
			printf("OK : write rejected.\n");
		else
			printf("BUG: policy write not rejected.\n");
	}
	write(fd, "delete ", 7);
	write(fd, policy, strlen(policy));
	write(fd, "\n", 1);
}

static void stage_policy_io_test(void)
{
	int i;
	policy_file = proc_policy_system_policy;
	fd = open(policy_file, O_WRONLY);
	for (i = 0; i < 3; i++) {
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
		try_io("deny_autobind 65-100", 1);
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
		try_io("allow_mount none /tmp/ tmpfs 0x1", 1);
		try_io("allow_mount none /tmp/ tmpfs", 0);
		try_io("allow_mount none /tmp/ nonexistent 0x0", 1);
		try_io("allow_mount none /proc/ proc 0x0", 1);
		try_io("allow_mount none /selinux/ selinuxfs 0x0", 1);
		try_io("allow_mount /proc/bus/usb /proc/bus/usb/ usbfs 0x0", 1);
		try_io("allow_mount none /dev/pts/ devpts 0x0", 1);
		try_io("allow_mount any / --remount 0xC00", 1);
		try_io("allow_mount /dev/sda1 /boot/ ext3 0xC00", 1);
		try_io("allow_mount none /dev/shm/ tmpfs 0x0", 1);
		try_io("allow_mount none /proc/sys/fs/binfmt_misc/ binfmt_misc "
		       "0x0", 1);
		try_io("allow_mount none /proc/sys/fs/binfmt_misc/ binfmt_misc "
		       "0x0 0x1", 0);
		try_io("allow_mount none /proc/sys/fs/binfmt_misc/ tmpfs "
		       "binfmt_misc 0x0", 0);
		try_io("allow_mount /proc/bus/usb /proc/bus/usb/ usbfs 0x0", 1);
	}
	close(fd);
	policy_file = proc_policy_exception_policy;
	fd = open(policy_file, O_WRONLY);
	for (i = 0; i < 3; i++) {
		try_io("allow_read /tmp/abc", 1);
		try_io("allow_read /tmp/abc\\*", 1);
		try_io("allow_read abc", 0);
		try_io("allow_read /tmp/abc/", 0);
		try_io("allow_read", 0);
		try_io("allow_read *", 0);
		try_io("allow_env FOO", 1);
		try_io("allow_env FOO=", 0);
		try_io("allow_env FOO=BAR", 0);
		try_io("allow_env FOO BAR", 0);
		try_io("allow_env FOO\\040BAR", 1);
		try_io("allow_env FOO;BAR;BUZ", 1);
		try_io("file_pattern /\\*\\*\\*", 1);
		try_io("file_pattern /abc", 0);
		try_io("file_pattern /abc /def", 0);
		try_io("file_pattern abcdef", 0);
		try_io("alias /foo /bar", 1);
		try_io("alias /foo/ /bar", 0);
		try_io("alias /foo /bar/", 0);
		try_io("alias /f\\* /bar", 0);
		try_io("alias /foo /b\\*", 0);
		try_io("path_group TEST /", 1);
		try_io("path_group TEST /boo", 1);
		try_io("path_group TEST /bar", 1);
		try_io("path_group TEST /\\*", 1);
		try_io("path_group TEST / /", 0);
		try_io("path_group TEST /boo", 1);
		try_io("path_group TEST /bar", 1);
		try_io("path_group TEST boo", 1);
		try_io("path_group TEST boo/", 1);
		try_io("path_group TEST /bar", 1);
		try_io("path_group TEST3 /\\*", 1);
		try_io("path_group TEST3 / /", 0);
		try_io("path_group TEST3 /boo", 1);
		try_io("path_group TEST3 /bar", 1);
		try_io("path_group TEST3 boo", 1);
		try_io("path_group TEST3 boo/", 1);
		try_io("address_group TEST 0.0.0.0", 1);
		try_io("address_group TEST 0.0.0.0-1.1.1.1", 1);
		try_io("address_group TEST 0:0:0:0:0:0:0:ff", 1);
		try_io("address_group TEST "
		       "0:0:0:0:0:0:0:0-ff:ff:ff:ff:ff:ff:ff:ff", 1);
		try_io("address_group TEST "
		       "fff0:fff1:fff2:fff3:fff4:fff5:fff6:fff7-"
		       "fff8:fff9:fffa:fffb:fffc:fffd:fffe:ffff", 1);
		try_io("address_group TEST2 0:0:0:0:0:0:0:ff", 1);
		try_io("address_group TEST2 "
		       "0:0:0:0:0:0:0:0-ff:ff:ff:ff:ff:ff:ff:ff", 1);
		try_io("address_group TEST2 "
		       "fff0:fff1:fff2:fff3:fff4:fff5:fff6:fff7-"
		       "fff8:fff9:fffa:fffb:fffc:fffd:fffe:ffff", 1);
		try_io("deny_rewrite /", 1);
		try_io("deny_rewrite /foo", 1);
		try_io("deny_rewrite /\\*", 1);
		try_io("deny_rewrite /\\:", 0);
		try_io("deny_rewrite / /", 0);
		try_io("deny_rewrite @/TEST", 1);
		try_io("aggregator /boo/\\* /BOO", 1);
		try_io("aggregator /boo/\\* /BOO\\*", 0);
		try_io("aggregator /boo/\\*/ /BOO", 0);
		try_io("aggregator /boo/\\* /BOO/", 0);
		try_io("keep_domain <kernel>", 1);
		try_io("keep_domain <kernel> /sbin/init", 1);
		try_io("keep_domain <kernel> foo", 0);
		try_io("keep_domain <kernel> \\*", 0);
		try_io("keep_domain /ssh", 1);
		try_io("keep_domain /ssh /foo", 0);
		try_io("keep_domain /foo from <kernel>", 1);
		try_io("keep_domain /foo from <kernel> /sbin/init", 1);
		try_io("keep_domain from <kernel> /sbin/init", 0);
		try_io("keep_domain \\* from <kernel> /sbin/init", 0);
		try_io("no_keep_domain <kernel>", 1);
		try_io("no_keep_domain <kernel> /sbin/init", 1);
		try_io("no_keep_domain <kernel> foo", 0);
		try_io("no_keep_domain <kernel> \\*", 0);
		try_io("no_keep_domain /ssh", 1);
		try_io("no_keep_domain /ssh /foo", 0);
		try_io("no_keep_domain /foo from <kernel>", 1);
		try_io("no_keep_domain /foo from <kernel> /sbin/init", 1);
		try_io("no_keep_domain from <kernel> /sbin/init", 0);
		try_io("no_keep_domain \\* from <kernel> /sbin/init", 0);
		try_io("initialize_domain /foo", 1);
		try_io("initialize_domain /\\*", 0);
		try_io("initialize_domain /foo /bar", 0);
		try_io("initialize_domain /foo from /bar", 1);
		try_io("initialize_domain /foo from <kernel> /bar", 1);
		try_io("initialize_domain /\\* from <kernel>", 0);
		try_io("initialize_domain /foo from <kernel> \\*", 0);
		try_io("no_initialize_domain /foo", 1);
		try_io("no_initialize_domain /\\*", 0);
		try_io("no_initialize_domain /foo /bar", 0);
		try_io("no_initialize_domain /foo from /bar", 1);
		try_io("no_initialize_domain /foo from <kernel> /bar", 1);
		try_io("no_initialize_domain /\\* from <kernel>", 0);
		try_io("no_initialize_domain /foo from <kernel> \\*", 0);
	}
	close(fd);
}

int main(int argc, char *argv[])
{
	ccs_test_init();
	stage_policy_io_test();
	return 0;
}
