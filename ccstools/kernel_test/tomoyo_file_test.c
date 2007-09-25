/*
 * tomoyo_file_test.c
 *
 * Testing program for fs/tomoyo_file.c
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0   2007/09/20
 *
 */
#include "include.h"

static int is_enforce = 0;

static void ShowPrompt(const char *str) {
	printf("Testing %35s: (%s) ", str, is_enforce ? "must fail" : "should success");
	errno = 0;
}

static void ShowResult(int result) {
	if (is_enforce) {
		if (result == EOF) {
			if (errno == EPERM) printf("OK: Permission denied.\n");
			else printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG!\n");
		}
	} else {
		if (result != EOF) printf("OK\n");
		else printf("%s\n", strerror(errno));
	}
}

static const char *dev_null_path       = "/dev/null";
static const char *truncate_path       = "/tmp/truncate_test";
static const char *ftruncate_path      = "/tmp/ftruncate_test";
static const char *open_creat_path     = "/tmp/open_test";
static const char *mknod_reg_path      = "/tmp/mknod_reg_test";
static const char *mknod_chr_path      = "/tmp/mknod_chr_test";
static const char *mknod_blk_path      = "/tmp/mknod_blk_test";
static const char *mknod_fifo_path     = "/tmp/mknod_fifo_test";
static const char *mknod_sock_path     = "/tmp/mknod_sock_test";
static const char *unlink_path         = "/tmp/unlink_test";
static const char *mkdir_path          = "/tmp/mkdir_test";
static const char *rmdir_path          = "/tmp/rmdir_test";
static const char *link_source_path    = "/tmp/link_source_test";
static const char *link_dest_path      = "/tmp/link_dest_test";
static const char *symlink_source_path = "/tmp/symlink_source_test";
static const char *symlink_dest_path   = "/tmp/symlink_dest_test";
static const char *rename_source_path  = "/tmp/rename_source_test";
static const char *rename_dest_path    = "/tmp/rename_dest_test";
static const char *socket_path         = "/tmp/socket_test";

static int ftruncate_fd = EOF;

static void StageFileTest(void) {
	int fd;
	{
		static int name[] = { CTL_NET, NET_IPV4, NET_IPV4_LOCAL_PORT_RANGE };
		int buffer[2] = { 32768, 61000 };
		int size = sizeof(buffer);
		ShowPrompt("sysctl(READ)");
		ShowResult(sysctl(name, 3, buffer, &size, 0, 0));
		ShowPrompt("sysctl(WRITE)");
		ShowResult(sysctl(name, 3, 0, 0, buffer, size));
	}

	ShowPrompt("uselib()");
	ShowResult(uselib("/bin/true"));

	{
		int pipe_fd[2] = { EOF, EOF };
		int err = 0;
		fflush(stdout); fflush(stderr);
		pipe(pipe_fd);
		if (fork() == 0) {
			execl("/bin/true", "/bin/true", NULL);
			err = errno;
			write(pipe_fd[1], &err, sizeof(err));
			_exit(0);
		}
		close(pipe_fd[1]);
		read(pipe_fd[0], &err, sizeof(err));
		ShowPrompt("execve()");
		errno = err;
		ShowResult(err ? EOF : 0);
	}

	ShowPrompt("open(O_RDONLY)");
	fd = open(dev_null_path, O_RDONLY);
	ShowResult(fd);
	if (fd != EOF) close(fd);
	
	ShowPrompt("open(O_WRONLY)");
	fd = open(dev_null_path, O_WRONLY);
	ShowResult(fd);
	if (fd != EOF) close(fd);

	ShowPrompt("open(O_RDWR)");
	fd = open(dev_null_path, O_RDWR);
	ShowResult(fd);
	if (fd != EOF) close(fd);

	ShowPrompt("open(O_CREAT | O_EXCL)");
	fd = open(open_creat_path, O_CREAT | O_EXCL, 0666);
	ShowResult(fd);
	if (fd != EOF) close(fd);

	ShowPrompt("open(O_TRUNC)");
	fd = open(truncate_path, O_TRUNC);
	ShowResult(fd);
	if (fd != EOF) close(fd);

	ShowPrompt("truncate()");
	ShowResult(truncate(truncate_path, 0));

	ShowPrompt("ftruncate()");
	ShowResult(ftruncate(ftruncate_fd, 0));
	
	ShowPrompt("mknod(S_IFREG)");
	ShowResult(mknod(mknod_reg_path, S_IFREG, 0));

	ShowPrompt("mknod(S_IFCHR)");
	ShowResult(mknod(mknod_chr_path, S_IFCHR, MKDEV(1, 3)));

	ShowPrompt("mknod(S_IFBLK)");
	ShowResult(mknod(mknod_blk_path, S_IFBLK, MKDEV(1, 0)));

	ShowPrompt("mknod(S_IFIFO)");
	ShowResult(mknod(mknod_fifo_path, S_IFIFO, 0));

	ShowPrompt("mknod(S_IFSOCK)");
	ShowResult(mknod(mknod_sock_path, S_IFSOCK, 0));

	ShowPrompt("mkdir()");
	ShowResult(mkdir(mkdir_path, 0600));

	ShowPrompt("rmdir()");
	ShowResult(rmdir(rmdir_path));

	ShowPrompt("unlink()");
	ShowResult(unlink(unlink_path));

	ShowPrompt("symlink()");
	ShowResult(symlink(symlink_dest_path, symlink_source_path));

	ShowPrompt("link()");
	ShowResult(link(link_source_path, link_dest_path));

	ShowPrompt("rename()");
	ShowResult(rename(rename_source_path, rename_dest_path));

	{
		struct sockaddr_un addr;
		int fd;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		ShowPrompt("unix_bind()");
		ShowResult(bind(fd, (struct sockaddr *) &addr, sizeof(addr)));
		if (fd != EOF) close(fd);
	}
	
	printf("\n\n");
}

static void CreateFiles(void) {
	mkdir(rmdir_path, 0700);
	close(creat(link_source_path, 0600));
	close(creat(rename_source_path, 0600));
	close(creat(truncate_path, 0600));
	close(creat(unlink_path, 0600));
	ftruncate_fd = open(ftruncate_path, O_WRONLY | O_CREAT, 0600);
}

static void CleanUpFiles(void) {
	if (ftruncate_fd != EOF) close(ftruncate_fd); ftruncate_fd = EOF;
	unlink(open_creat_path);
	unlink(mknod_reg_path);
	unlink(mknod_chr_path);
	unlink(mknod_blk_path);
	unlink(mknod_fifo_path);
	unlink(mknod_sock_path);
	rmdir(mkdir_path);
	unlink(symlink_source_path);
	unlink(symlink_dest_path);
	unlink(link_source_path);
	unlink(link_dest_path);
	unlink(rename_source_path);
	unlink(rename_dest_path);
	unlink(truncate_path);
	unlink(ftruncate_path);
	unlink(socket_path);
}

static void SetFileEnforce(int enforce) {
	if (enforce) {
		WriteStatus("MAC_FOR_FILE=3\n");
	} else {
		WriteStatus("MAC_FOR_FILE=2\n");
	}
}

int main(int argc, char *argv[]) {
	Init();

	printf("***** Testing file hooks in enforce mode. *****\n");
	CreateFiles();
	is_enforce = 1;
	SetFileEnforce(1);
	StageFileTest();
	SetFileEnforce(0);
	ClearStatus();
	CleanUpFiles();

	printf("***** Testing file hooks in permissive mode. *****\n");
	is_enforce = 0;
	CreateFiles();
	SetFileEnforce(0);
	StageFileTest();
	CleanUpFiles();

	ClearStatus();
	return 0;
}
