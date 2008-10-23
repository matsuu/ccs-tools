/*
 * tomoyo_file_test.c
 *
 * Testing program for fs/tomoyo_file.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5-pre   2008/10/20
 *
 */
#include "include.h"

static int domain_fd = EOF;
static int exception_fd = EOF;
static const char *policy = "";
static char self_domain[4096] = "";

static int write_policy(void)
{
	FILE *fp;
	char buffer[8192];
	char *cp;
	int domain_found = 0;
	int policy_found = 0;
	memset(buffer, 0, sizeof(buffer));
	cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	fp = fopen(proc_policy_domain_policy, "r");
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
	if (!fp) {
		printf("%s : BUG: policy read failed\n", policy);
		return 0;
	}
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		cp = strchr(buffer, '\n');
		if (cp)
			*cp = '\0';
		if (!strncmp(buffer, "<kernel>", 8))
			domain_found = !strcmp(self_domain, buffer);
		if (domain_found) {
			/* printf("<%s>\n", buffer); */
			if (!strcmp(buffer, policy)) {
				policy_found = 1;
				break;
			}
		}
	}
	fclose(fp);
	if (!policy_found) {
		printf("%s : BUG: policy write failed\n", policy);
		return 0;
	}
	errno = 0;
	return 1;
}

static void delete_policy(void)
{
	write(domain_fd, "delete ", 7);
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
}

static void show_result(int result, char should_success)
{
	int err = errno;
	printf("%s : ", policy);
	if (should_success) {
		if (result != EOF)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(err));
	} else {
		if (result == EOF) {
			if (err == EPERM)
				printf("OK: Permission denied.\n");
			else
				printf("FAILED: %s\n", strerror(err));
		} else {
			printf("BUG: didn't fail.\n");
		}
	}
}

static void create2(const char *pathname)
{
	const char *cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	close(creat(pathname, 0600));
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	errno = 0;
}

static void mkdir2(const char *pathname)
{
	const char *cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	mkdir(pathname, 0600);
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	errno = 0;
}

static void unlink2(const char *pathname)
{
	const char *cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	unlink(pathname);
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	errno = 0;
}

static void rmdir2(const char *pathname)
{
	const char *cp = "255-MAC_FOR_FILE=disabled\n";
	write(profile_fd, cp, strlen(cp));
	rmdir(pathname);
	cp = "255-MAC_FOR_FILE=enforcing\n";
	write(profile_fd, cp, strlen(cp));
	errno = 0;
}

static void stage_file_test(void)
{
	char *filename = "";
	policy = "allow_read /proc/sys/net/ipv4/ip_local_port_range "
		"if task.uid=0 task.gid=0";
	if (write_policy()) {
		static int name[] = { CTL_NET, NET_IPV4,
				      NET_IPV4_LOCAL_PORT_RANGE };
		int buffer[2] = { 32768, 61000 };
		size_t size = sizeof(buffer);
		show_result(sysctl(name, 3, buffer, &size, 0, 0), 1);
		delete_policy();
		show_result(sysctl(name, 3, buffer, &size, 0, 0), 0);
	}
	policy = "allow_write /proc/sys/net/ipv4/ip_local_port_range "
		"if task.euid=0 0=0 1-100=10-1000";
	if (write_policy()) {
		static int name[] = { CTL_NET, NET_IPV4,
				      NET_IPV4_LOCAL_PORT_RANGE };
		int buffer[2] = { 32768, 61000 };
		size_t size = sizeof(buffer);
		show_result(sysctl(name, 3, 0, 0, buffer, size), 1);
		delete_policy();
		show_result(sysctl(name, 3, 0, 0, buffer, size), 0);
	}
	policy = "allow_read/write /proc/sys/net/ipv4/ip_local_port_range "
		"if 1!=10-100";
	if (write_policy()) {
		static int name[] = { CTL_NET, NET_IPV4,
				      NET_IPV4_LOCAL_PORT_RANGE };
		int buffer[2] = { 32768, 61000 };
		size_t size = sizeof(buffer);
		show_result(sysctl(name, 3, buffer, &size, buffer, size), 1);
		delete_policy();
		show_result(sysctl(name, 3, buffer, &size, buffer, size), 0);
	}

	policy = "allow_read /bin/true "
		"if path1.uid=0 path1.parent.uid=0 10=10-100";
	if (write_policy()) {
		show_result(uselib("/bin/true"), 1);
		delete_policy();
		show_result(uselib("/bin/true"), 0);
	}

	policy = "allow_execute /bin/true if task.uid!=10 path1.parent.uid=0";
	if (write_policy()) {
		int pipe_fd[2] = { EOF, EOF };
		int err = 0;
		fflush(stdout);
		fflush(stderr);
		pipe(pipe_fd);
		if (fork() == 0) {
			execl("/bin/true", "/bin/true", NULL);
			err = errno;
			write(pipe_fd[1], &err, sizeof(err));
			_exit(0);
		}
		close(pipe_fd[1]);
		read(pipe_fd[0], &err, sizeof(err));
		close(pipe_fd[0]);
		wait(NULL);
		errno = err;
		show_result(err ? EOF : 0, 1);
		delete_policy();
		fflush(stdout);
		fflush(stderr);
		pipe(pipe_fd);
		if (fork() == 0) {
			execl("/bin/true", "/bin/true", NULL);
			err = errno;
			write(pipe_fd[1], &err, sizeof(err));
			_exit(0);
		}
		close(pipe_fd[1]);
		read(pipe_fd[0], &err, sizeof(err));
		close(pipe_fd[0]);
		wait(NULL);
		errno = err;
		show_result(err ? EOF : 0, 0);
	}

	policy = "allow_read /dev/null if path1.parent.ino=path1.parent.ino";
	if (write_policy()) {
		int fd = open("/dev/null", O_RDONLY);
		show_result(fd, 1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		fd = open("/dev/null", O_RDONLY);
		show_result(fd, 0);
		if (fd != EOF)
			close(fd);
	}

	policy = "allow_write /dev/null if path1.uid=path1.gid";
	if (write_policy()) {
		int fd = open("/dev/null", O_WRONLY);
		show_result(fd, 1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		fd = open("/dev/null", O_WRONLY);
		show_result(fd, 0);
		if (fd != EOF)
			close(fd);
	}

	policy = "allow_read/write /dev/null if task.uid=path1.parent.uid";
	if (write_policy()) {
		int fd = open("/dev/null", O_RDWR);
		show_result(fd, 1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		fd = open("/dev/null", O_RDWR);
		show_result(fd, 0);
		if (fd != EOF)
			close(fd);
	}

	policy = "allow_create /tmp/open_test if path1.parent.uid=task.uid";
	if (write_policy()) {
		policy = "allow_write /tmp/open_test if path1.parent.uid=0";
		if (write_policy()) {
			int fd = open("/tmp/open_test",
				      O_WRONLY | O_CREAT | O_EXCL, 0666);
			show_result(fd, 1);
			if (fd != EOF)
				close(fd);
			unlink2("/tmp/open_test");
			delete_policy();
			fd = open("/tmp/open_test",
				  O_WRONLY | O_CREAT | O_EXCL, 0666);
			show_result(fd, 0);
			if (fd != EOF)
				close(fd);
			unlink2("/tmp/open_test");
		}
		policy = "allow_create /tmp/open_test "
			"if path1.parent.uid=task.uid\n";
		delete_policy();
	}

	policy = "allow_write /tmp/open_test if task.uid=0 path1.ino!=0";
	if (write_policy()) {
		policy = "allow_create /tmp/open_test if 0=0";
		if (write_policy()) {
			int fd = open("/tmp/open_test",
				      O_WRONLY | O_CREAT | O_EXCL, 0666);
			show_result(fd, 1);
			if (fd != EOF)
				close(fd);
			unlink2("/tmp/open_test");
			delete_policy();
			fd = open("/tmp/open_test",
				  O_WRONLY | O_CREAT | O_EXCL, 0666);
			show_result(fd, 0);
			if (fd != EOF)
				close(fd);
			unlink2("/tmp/open_test");
		}
		policy = "allow_write /tmp/open_test "
			"if task.uid=0 path1.ino!=0\n";
		delete_policy();
	}

	filename = "/tmp/truncate_test";
	create2(filename);

	policy = "allow_truncate /tmp/truncate_test if task.uid=path1.uid";
	if (write_policy()) {
		policy = "allow_write /tmp/truncate_test if 1!=100-1000000";
		if (write_policy()) {
			int fd = open(filename, O_WRONLY | O_TRUNC);
			show_result(fd, 1);
			if (fd != EOF)
				close(fd);
			delete_policy();
			fd = open(filename, O_WRONLY | O_TRUNC);
			show_result(fd, 0);
			if (fd != EOF)
				close(fd);
		}
		policy = "allow_truncate /tmp/truncate_test "
			"if task.uid=path1.uid";
		delete_policy();
	}

	policy = "allow_write /tmp/truncate_test";
	if (write_policy()) {
		policy = "allow_truncate /tmp/truncate_test";
		if (write_policy()) {
			int fd = open(filename, O_WRONLY | O_TRUNC);
			show_result(fd, 1);
			if (fd != EOF)
				close(fd);
			delete_policy();
			fd = open(filename, O_WRONLY | O_TRUNC);
			show_result(fd, 0);
			if (fd != EOF)
				close(fd);
		}
		policy = "allow_write /tmp/truncate_test\n";
		delete_policy();
	}

	policy = "allow_truncate /tmp/truncate_test";
	if (write_policy()) {
		show_result(truncate(filename, 0), 1);
		delete_policy();
		show_result(truncate(filename, 0), 0);
	}

	policy = "allow_truncate /tmp/truncate_test";
	if (write_policy()) {
		int fd;
		const char *cp = "255-MAC_FOR_FILE=disabled\n";
		write(profile_fd, cp, strlen(cp));
		fd = open(filename, O_WRONLY);
		cp = "255-MAC_FOR_FILE=enforcing\n";
		write(profile_fd, cp, strlen(cp));
		show_result(ftruncate(fd, 0), 1);
		delete_policy();
		show_result(ftruncate(fd, 0), 0);
		if (fd != EOF)
			close(fd);
	}

	unlink2(filename);

	policy = "allow_create /tmp/mknod_reg_test";
	if (write_policy()) {
		filename = "/tmp/mknod_reg_test";
		show_result(mknod(filename, S_IFREG, 0), 1);
		delete_policy();
		unlink2(filename);
		show_result(mknod(filename, S_IFREG, 0), 0);
	}

	policy = "allow_mkchar /tmp/mknod_chr_test";
	if (write_policy()) {
		filename = "/tmp/mknod_chr_test";
		show_result(mknod(filename, S_IFCHR, MKDEV(1, 3)), 1);
		delete_policy();
		unlink2(filename);
		show_result(mknod(filename, S_IFCHR, MKDEV(1, 3)), 0);
	}

	policy = "allow_mkblock /tmp/mknod_blk_test";
	if (write_policy()) {
		filename = "/tmp/mknod_blk_test";
		show_result(mknod(filename, S_IFBLK, MKDEV(1, 0)), 1);
		delete_policy();
		unlink2(filename);
		show_result(mknod(filename, S_IFBLK, MKDEV(1, 0)), 0);
	}

	policy = "allow_mkfifo /tmp/mknod_fifo_test";
	if (write_policy()) {
		filename = "/tmp/mknod_fifo_test";
		show_result(mknod(filename, S_IFIFO, 0), 1);
		delete_policy();
		unlink2(filename);
		show_result(mknod(filename, S_IFIFO, 0), 0);
	}

	policy = "allow_mksock /tmp/mknod_sock_test";
	if (write_policy()) {
		filename = "/tmp/mknod_sock_test";
		show_result(mknod(filename, S_IFSOCK, 0), 1);
		delete_policy();
		unlink2(filename);
		show_result(mknod(filename, S_IFSOCK, 0), 0);
	}

	policy = "allow_mkdir /tmp/mkdir_test/";
	if (write_policy()) {
		filename = "/tmp/mkdir_test";
		show_result(mkdir(filename, 0600), 1);
		delete_policy();
		rmdir2(filename);
		show_result(mkdir(filename, 0600), 0);
	}

	policy = "allow_rmdir /tmp/rmdir_test/";
	if (write_policy()) {
		filename = "/tmp/rmdir_test";
		mkdir2(filename);
		show_result(rmdir(filename), 1);
		delete_policy();
		mkdir2(filename);
		show_result(rmdir(filename), 0);
		rmdir2(filename);
	}

	policy = "allow_unlink /tmp/unlink_test";
	if (write_policy()) {
		filename = "/tmp/unlink_test";
		create2(filename);
		show_result(unlink(filename), 1);
		delete_policy();
		create2(filename);
		show_result(unlink(filename), 0);
		unlink2(filename);
	}

	policy = "allow_symlink /tmp/symlink_source_test";
	if (write_policy()) {
		filename = "/tmp/symlink_source_test";
		show_result(symlink("/tmp/symlink_dest_test", filename), 1);
		delete_policy();
		unlink2(filename);
		show_result(symlink("/tmp/symlink_dest_test", filename), 0);
	}

	policy = "allow_link /tmp/link_source_test /tmp/link_dest_test";
	if (write_policy()) {
		filename = "/tmp/link_source_test";
		create2(filename);
		show_result(link(filename, "/tmp/link_dest_test"), 1);
		delete_policy();
		unlink2("/tmp/link_dest_test");
		show_result(link(filename, "/tmp/link_dest_test"), 0);
		unlink2(filename);
	}

	policy = "allow_rename /tmp/rename_source_test /tmp/rename_dest_test";
	if (write_policy()) {
		filename = "/tmp/rename_source_test";
		create2(filename);
		show_result(rename(filename, "/tmp/rename_dest_test"), 1);
		delete_policy();
		unlink2("/tmp/rename_dest_test");
		create2(filename);
		show_result(rename(filename, "/tmp/rename_dest_test"), 0);
		unlink2(filename);
	}

	policy = "allow_mksock /tmp/socket_test";
	if (write_policy()) {
		struct sockaddr_un addr;
		int fd;
		filename = "/tmp/socket_test";
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, filename, sizeof(addr.sun_path) - 1);
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		show_result(bind(fd, (struct sockaddr *) &addr, sizeof(addr)),
			    1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		unlink2(filename);
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		show_result(bind(fd, (struct sockaddr *) &addr, sizeof(addr)),
			    0);
		if (fd != EOF)
			close(fd);
	}

	filename = "/tmp/rewrite_test";
	create2(filename);
	policy = "allow_read/write /tmp/rewrite_test";
	if (write_policy()) {
		char *cp = "deny_rewrite /tmp/rewrite_test\n";
		write(exception_fd, cp, strlen(cp));
		policy = "allow_truncate /tmp/rewrite_test";
		if (write_policy()) {
			int fd;

			fd = open(filename, O_RDONLY);
			show_result(fd, 1);
			if (fd != EOF)
				close(fd);

			fd = open(filename, O_WRONLY | O_APPEND);
			show_result(fd, 1);
			if (fd != EOF)
				close(fd);

			fd = open(filename, O_WRONLY);
			show_result(fd, 0);
			if (fd != EOF)
				close(fd);

			fd = open(filename, O_WRONLY | O_TRUNC);
			show_result(fd, 0);
			if (fd != EOF)
				close(fd);

			fd = open(filename, O_WRONLY | O_TRUNC | O_APPEND);
			show_result(fd, 0);
			if (fd != EOF)
				close(fd);

			show_result(truncate(filename, 0), 0);

			cp = "255-MAC_FOR_FILE=disabled\n";
			write(profile_fd, cp, strlen(cp));
			fd = open(filename, O_WRONLY | O_APPEND);
			cp = "255-MAC_FOR_FILE=enforcing\n";
			write(profile_fd, cp, strlen(cp));
			show_result(ftruncate(fd, 0), 0);

			show_result(fcntl(fd, F_SETFL,
					  fcntl(fd, F_GETFL) & ~O_APPEND), 0);
			if (fd != EOF)
				close(fd);

			delete_policy();
		}
		policy = "allow_read/write /tmp/rewrite_test";
		delete_policy();
		cp = "delete deny_rewrite /tmp/rewrite_test\n";
		write(exception_fd, cp, strlen(cp));
	}
	unlink2(filename);
}

int main(int argc, char *argv[])
{
	char *cp;
	ccs_test_init();
	domain_fd = open(proc_policy_domain_policy, O_WRONLY);
	exception_fd = open(proc_policy_exception_policy, O_WRONLY);
	{
		int self_fd = open(proc_policy_self_domain, O_RDONLY);
		memset(self_domain, 0, sizeof(self_domain));
		read(self_fd, self_domain, sizeof(self_domain) - 1);
		close(self_fd);
		write(domain_fd, self_domain, strlen(self_domain));
		cp = " /bin/true\n";
		write(domain_fd, cp, strlen(cp));
		write(domain_fd, self_domain, strlen(self_domain));
		write(domain_fd, "\n", 1);
		cp = "use_profile 255\n";
		write(domain_fd, cp, strlen(cp));
	}
	cp = "255-MAX_REJECT_LOG=1024\n";
	write(profile_fd, cp, strlen(cp));
	stage_file_test();
	cp = "use_profile 0\n";
	write(domain_fd, cp, strlen(cp));
	clear_status();
	return 0;
}
