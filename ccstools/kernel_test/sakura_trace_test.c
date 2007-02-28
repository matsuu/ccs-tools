/*
 * sakura_trace_test.c
 *
 * Testing program for fs/sakura_trace.c
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.3.1   2006/12/08
 *
 */
#include "include.h"

int main(int argc, char *argv[]) {
	system("dmesg -c > /dev/null");
	Init();
	if (mount("none", "/tmp", "tmpfs", MS_RDONLY, NULL)) {
		fprintf(stderr, "Can't mount tmpfs on /tmp/ .\n");
		return 1;
	}
	WriteStatus("TRACE_READONLY=1\n");
	{
		const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		snprintf(addr.sun_path, sizeof(addr.sun_path) - 1, "/tmp/bind_socket");
		bind(fd, (struct sockaddr *) &addr, sizeof(addr));
		close(fd);
	}
	mkdir("/tmp/mkdir", 0755);
	mknod("/tmp/mknod_file", 0600, 0);
	close(open("/tmp/open_create_file", O_WRONLY | O_CREAT, 0600));
	if (mount("none", "/tmp", "tmpfs", MS_REMOUNT, NULL)) {
		fprintf(stderr, "Can't remount.\n");
		goto out;
	}
	mkdir("/tmp/rmdir", 0755);
	close(open("/tmp/truncate_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/rename_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/source_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/open_write_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/utime_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/utimes_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/chmod_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/fchmod_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/chown_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/fchown_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/lchown_file", O_WRONLY | O_CREAT, 0600));
	close(open("/tmp/unlink_file", O_WRONLY | O_CREAT, 0600));
	if (mount("none", "/tmp", "tmpfs", MS_REMOUNT | MS_RDONLY, NULL)) {
		fprintf(stderr, "Can't remount.\n");
		goto out;
	}
	truncate("/tmp/truncate_file", 0);
	rename("/tmp/rename_file", "/tmp/renamed_file");
	link("/tmp/source_file", "/tmp/link_file");
	symlink("/tmp/source_file", "/tmp/symlink_file");
	rmdir("/tmp/rmdir");
	close(open("/tmp/open_write_file", O_WRONLY));
	{
		struct utimbuf buf;
		memset(&buf, 0, sizeof(buf));
		utime("/tmp/utime_file", &buf);
	}
	{
		struct timeval tv[2];
		memset(tv, 0, sizeof(tv));
		utimes("/tmp/utimes_file", tv);
	}
	chmod("/tmp/chmod_file", 0);
	{
		const int fd = open("/tmp/fchmod_file", O_RDONLY);
		fchmod(fd, 0);
		close(fd);
	}
	chown("/tmp/chown_file", 0, 0);
	{
		const int fd = open("/tmp/fchown_file", O_RDONLY);
		fchown(fd, 0, 0);
		close(fd);
	}
	lchown("/tmp/lchown_file", 0, 0);
	unlink("/tmp/unlink_file");
 out: ;
	umount("/tmp");
	WriteStatus("TRACE_READONLY=0\n");
	{
		FILE *fp = popen("dmesg -c", "r");
		static char buffer[4096];
		int i;
		struct {
			int checked;
			const char *word;
			const char *description;
		} checklist[] = {
			{ 0, "/tmp/bind_socket", "bind(AF_UNIX)" },
			{ 0, "/tmp/mkdir", "mkdir()" },
			{ 0, "/tmp/mknod_file", "mknod(FILE)" },
			{ 0, "/tmp/open_create_file", "open(CREATE)" },
			{ 0, "/tmp/truncate_file", "truncate()" },
			{ 0, "/tmp/rename_file", "rename()" },
			{ 0, "/tmp/link_file", "link()" },
			{ 0, "/tmp/symlink_file", "symlink()" },
			{ 0, "/tmp/rmdir", "rmdir()" },
			{ 0, "/tmp/open_write_file", "open(WRITE)" },
			{ 0, "/tmp/utime_file", "utime()" },
			{ 0, "/tmp/utimes_file", "utimes()" },
			{ 0, "/tmp/chmod_file", "chmod()" },
			{ 0, "/tmp/fchmod_file", "fchmod()" },
			{ 0, "/tmp/chown_file", "chown()" },
			{ 0, "/tmp/fchown_file", "fchown()" },
			{ 0, "/tmp/lchown_file", "lchown()" },
			{ 0, "/tmp/unlink_file", "unlink()" },
			{ 0, NULL, NULL }
		};
		while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 1, fp)) {
			printf("%s", buffer);
			if (strstr(buffer, "ReadOnly:") == NULL) continue;
			for (i = 0; checklist[i].word; i++) {
				if (!strstr(buffer, checklist[i].word)) continue;
				checklist[i].checked = 1;
				break;
			}
		}
		pclose(fp);
		for (i = 0; checklist[i].word; i++) {
			printf("%s %s\n", checklist[i].checked ? "OK" : "MISSING", checklist[i].description);
		}
	}
	return 0;
}
