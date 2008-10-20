/*
 * tomoyo_new_capability_test.c
 *
 * Testing program for fs/tomoyo_capability.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5-pre   2008/10/20
 *
 */
#include "include.h"

static int child(void *arg)
{
	errno = 0;
	pivot_root("/proc", proc_policy_dir);
	return errno;
}

static int domain_fd = EOF;
static const char *capability = "";
static char self_domain[4096] = "";

static int write_policy(void)
{
	FILE *fp = fopen(proc_policy_domain_policy, "r");
	char buffer[8192];
	char *cp;
	int domain_found = 0;
	int policy_found = 0;
	memset(buffer, 0, sizeof(buffer));
	cp = "allow_capability ";
	write(domain_fd, cp, strlen(cp));
	write(domain_fd, capability, strlen(capability));
	write(domain_fd, "\n", 1);
	if (!fp) {
		printf("allow_capability %s : BUG: capability read failed\n",
		       capability);
		return 0;
	}
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		cp = strchr(buffer, '\n');
		if (cp)
			*cp = '\0';
		if (!strncmp(buffer, "<kernel>", 8))
			domain_found = !strcmp(self_domain, buffer);
		if (domain_found) {
			if (!strncmp(buffer, "allow_capability ", 17) &&
			    !strcmp(buffer + 17, capability)) {
				policy_found = 1;
				break;
			}
		}
	}
	fclose(fp);
	if (!policy_found) {
		printf("allow_capability %s : BUG: capability write failed\n",
		       capability);
		return 0;
	}
	errno = 0;
	return 1;
}

static void delete_policy(void)
{
	const char *cp = "allow_capability ";
	write(domain_fd, "delete ", 7);
	write(domain_fd, cp, strlen(cp));
	write(domain_fd, capability, strlen(capability));
	write(domain_fd, "\n", 1);
}

static void show_result(int result, char should_success)
{
	printf("allow_capability %s : ", capability);
	if (should_success) {
		if (result != EOF)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
	} else {
		if (result == EOF) {
			if (errno == EPERM)
				printf("OK: Permission denied.\n");
			else
				printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG\n");
		}
	}
}

static void SetCapability(void)
{
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1,
		 "MAC_FOR_CAPABILITY::%s=enforcing\n", capability);
	WriteStatus(buffer);
}

static void UnsetCapability(void)
{
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1,
		 "MAC_FOR_CAPABILITY::%s=disabled\n", capability);
	WriteStatus(buffer);
}

static void StageCapabilityTest(void)
{
	char tmp1[128];
	char tmp2[128];
	memset(tmp1, 0, sizeof(tmp1));
	memset(tmp2, 0, sizeof(tmp2));

	capability = "inet_tcp_create";
	SetCapability();
	if (write_policy()) {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		show_result(fd, 1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		fd = socket(AF_INET, SOCK_STREAM, 0);
		show_result(fd, 0);
		if (fd != EOF)
			close(fd);
	}
	UnsetCapability();

	{
		int fd1 = socket(AF_INET, SOCK_STREAM, 0);
		int fd2 = socket(AF_INET, SOCK_STREAM, 0);
		int fd3 = socket(AF_INET, SOCK_STREAM, 0);
		int fd4 = socket(AF_INET, SOCK_STREAM, 0);
		struct sockaddr_in addr;
		socklen_t size = sizeof(addr);
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(0);
		bind(fd1, (struct sockaddr *) &addr, sizeof(addr));
		bind(fd2, (struct sockaddr *) &addr, sizeof(addr));
		bind(fd3, (struct sockaddr *) &addr, sizeof(addr));
		bind(fd4, (struct sockaddr *) &addr, sizeof(addr));
		getsockname(fd1, (struct sockaddr *) &addr, &size);

		capability = "inet_tcp_listen";
		SetCapability();
		if (write_policy()) {
			show_result(listen(fd1, 5), 1);
			delete_policy();
			show_result(listen(fd2, 5), 0);
		}
		UnsetCapability();

		capability = "inet_tcp_connect";
		SetCapability();
		if (write_policy()) {
			show_result(connect(fd3, (struct sockaddr *) &addr,
					    sizeof(addr)), 1);
			delete_policy();
			show_result(connect(fd4, (struct sockaddr *) &addr,
					    sizeof(addr)), 0);
		}
		UnsetCapability();

		if (fd1 != EOF)
			close(fd1);
		if (fd2 != EOF)
			close(fd2);
		if (fd3 != EOF)
			close(fd3);
		if (fd4 != EOF)
			close(fd4);
	}

	capability = "use_inet_udp";
	SetCapability();
	if (write_policy()) {
		int fd = socket(AF_INET, SOCK_DGRAM, 0);
		show_result(fd, 1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		show_result(fd, 0);
		if (fd != EOF)
			close(fd);
	}
	UnsetCapability();

	capability = "use_inet_ip";
	SetCapability();
	if (write_policy()) {
		int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		show_result(fd, 1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		show_result(fd, 0);
		if (fd != EOF)
			close(fd);
	}
	UnsetCapability();

	capability = "use_route";
	SetCapability();
	if (write_policy()) {
		int fd = socket(AF_ROUTE, SOCK_RAW, 0);
		show_result(fd, 1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		fd = socket(AF_ROUTE, SOCK_RAW, 0);
		show_result(fd, 0);
		if (fd != EOF)
			close(fd);
	}
	UnsetCapability();

	capability = "use_packet";
	SetCapability();
	if (write_policy()) {
		int fd = socket(AF_PACKET, SOCK_RAW, 0);
		show_result(fd, 1);
		if (fd != EOF)
			close(fd);
		delete_policy();
		fd = socket(AF_PACKET, SOCK_RAW, 0);
		show_result(fd, 0);
		if (fd != EOF)
			close(fd);
	}
	UnsetCapability();

	capability = "use_kernel_module";
	SetCapability();
	if (write_policy()) {
		if (!is_kernel26)
			show_result((int) create_module("", 0), 1);
		show_result(init_module("", NULL), 1);
		show_result(delete_module(""), 1);
		delete_policy();
		if (!is_kernel26)
			show_result((int) create_module("", 0), 0);
		show_result(init_module("", NULL), 0);
		show_result(delete_module(""), 0);
	}
	UnsetCapability();

	capability = "create_fifo";
	SetCapability();
	if (write_policy()) {
		strcpy(tmp1, "/tmp/XXXXXX");
		close(mkstemp(tmp1));
		unlink(tmp1);
		show_result(mknod(tmp1, S_IFIFO, 0), 1);
		unlink(tmp1);
		delete_policy();
		show_result(mknod(tmp1, S_IFIFO, 0), 0);
		unlink(tmp1);
	}
	UnsetCapability();

	capability = "create_block_dev";
	SetCapability();
	if (write_policy()) {
		strcpy(tmp1, "/tmp/XXXXXX");
		close(mkstemp(tmp1));
		unlink(tmp1);
		show_result(mknod(tmp1, S_IFBLK, MKDEV(1, 0)), 1);
		unlink(tmp1);
		delete_policy();
		show_result(mknod(tmp1, S_IFBLK, MKDEV(1, 0)), 0);
		unlink(tmp1);
	}
	UnsetCapability();

	capability = "create_char_dev";
	SetCapability();
	if (write_policy()) {
		strcpy(tmp1, "/tmp/XXXXXX");
		close(mkstemp(tmp1));
		unlink(tmp1);
		show_result(mknod(tmp1, S_IFCHR, MKDEV(1, 3)), 1);
		unlink(tmp1);
		delete_policy();
		show_result(mknod(tmp1, S_IFCHR, MKDEV(1, 3)), 0);
		unlink(tmp1);
	}
	UnsetCapability();

	capability = "create_unix_socket";
	SetCapability();
	if (write_policy()) {
		strcpy(tmp1, "/tmp/XXXXXX");
		close(mkstemp(tmp1));
		unlink(tmp1);
		show_result(mknod(tmp1, S_IFSOCK, 0), 1);
		unlink(tmp1);
		delete_policy();
		show_result(mknod(tmp1, S_IFSOCK, 0), 0);
		unlink(tmp1);
	}
	if (write_policy()) {
		struct sockaddr_un addr;
		int fd1 = socket(AF_UNIX, SOCK_STREAM, 0);
		int fd2 = socket(AF_UNIX, SOCK_STREAM, 0);
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(tmp1, "/tmp/XXXXXX");
		strncpy(addr.sun_path, tmp1, sizeof(addr.sun_path) - 1);
		show_result(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)),
			    1);
		unlink(tmp1);
		delete_policy();
		show_result(bind(fd2, (struct sockaddr *) &addr, sizeof(addr)),
			    0);
		unlink(tmp1);
		if (fd1 != EOF)
			close(fd1);
		if (fd2 != EOF)
			close(fd2);
	}
	UnsetCapability();

	capability = "SYS_MOUNT";
	SetCapability();
	if (write_policy()) {
		show_result(mount("/", "/", "nonexistent", 0, NULL), 1);
		delete_policy();
		show_result(mount("/", "/", "nonexistent", 0, NULL), 0);
	}
	UnsetCapability();

	capability = "SYS_UMOUNT";
	SetCapability();
	if (write_policy()) {
		show_result(umount("/"), 1);
		if (access("/", W_OK))
			mount("", "/", "", MS_REMOUNT, NULL);
		delete_policy();
		show_result(umount("/"), 0);
		if (access("/", W_OK))
			mount("", "/", "", MS_REMOUNT, NULL);
	}
	UnsetCapability();

	capability = "SYS_REBOOT";
	SetCapability();
	if (write_policy()) {
		show_result(reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
				   0x0000C0DE /* Use invalid value so that
						 the system won't reboot. */,
				   NULL), 1);
		delete_policy();
		show_result(reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
				   0x0000C0DE /* Use invalid value so that
						 the system won't reboot. */,
				   NULL), 0);
	}
	UnsetCapability();

	capability = "SYS_CHROOT";
	SetCapability();
	if (write_policy()) {
		show_result(chroot("/"), 1);
		delete_policy();
		show_result(chroot("/"), 0);
	}
	UnsetCapability();

	capability = "SYS_PIVOT_ROOT";
	SetCapability();
	if (write_policy()) {
		int error;
		char *stack = malloc(8192);
		pid_t pid = clone(child, stack + (8192 / 2), CLONE_NEWNS, NULL);
		while (waitpid(pid, &error, __WALL) == EOF && errno == EINTR)
			error += 0; /* Dummy. */
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		show_result(errno ? EOF : 0, 1);
		delete_policy();
		pid = clone(child, stack + (8192 / 2), CLONE_NEWNS, NULL);
		while (waitpid(pid, &error, __WALL) == EOF && errno == EINTR)
			error += 0; /* Dummy. */
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		show_result(errno ? EOF : 0, 0);
		free(stack);
	}
	UnsetCapability();

	signal(SIGINT, SIG_IGN);
	capability = "SYS_KILL";
	SetCapability();
	if (write_policy()) {
		show_result(kill(pid, SIGINT), 1);
		show_result(tkill(gettid(), SIGINT), 1);
#ifdef __NR_tgkill
		if (is_kernel26)
			show_result(tgkill(pid, gettid(), SIGINT), 1);
#endif
		delete_policy();
		show_result(kill(pid, SIGINT), 0);
		show_result(tkill(gettid(), SIGINT), 0);
#ifdef __NR_tgkill
		if (is_kernel26)
			show_result(tgkill(pid, gettid(), SIGINT), 0);
#endif
	}
	UnsetCapability();
	signal(SIGINT, SIG_DFL);

	capability = "SYS_KEXEC_LOAD";
	SetCapability();
	if (write_policy()) {
#ifdef __NR_sys_kexec_load
		if (is_kernel26)
			show_result(sys_kexec_load(0, 0, NULL, 0), 1);
#endif
		delete_policy();
#ifdef __NR_sys_kexec_load
		if (is_kernel26)
			show_result(sys_kexec_load(0, 0, NULL, 0), 0);
#endif
	}
	UnsetCapability();

	capability = "SYS_VHANGUP";
	SetCapability();
	if (write_policy()) {
		int pty_fd = EOF, status = 0;
		int pipe_fd[2] = { EOF, EOF };
		pipe(pipe_fd);
		switch (forkpty(&pty_fd, NULL, NULL, NULL)) {
		case 0:
			errno = 0;
			vhangup();
			/* Unreachable if vhangup() succeeded. */
			status = errno;
			write(pipe_fd[1], &status, sizeof(status));
			_exit(0);
		case -1:
			fprintf(stderr, "forkpty() failed.\n");
			break;
		default:
			close(pipe_fd[1]);
			read(pipe_fd[0], &status, sizeof(status));
			wait(NULL);
			close(pipe_fd[0]);
			close(pty_fd);
			errno = status;
			show_result(status ? EOF : 0, 1);
		}
		delete_policy();
		status = 0;
		pipe(pipe_fd);
		switch (forkpty(&pty_fd, NULL, NULL, NULL)) {
		case 0:
			errno = 0;
			vhangup();
			/* Unreachable if vhangup() succeeded. */
			status = errno;
			write(pipe_fd[1], &status, sizeof(status));
			_exit(0);
		case -1:
			fprintf(stderr, "forkpty() failed.\n");
			break;
		default:
			close(pipe_fd[1]);
			read(pipe_fd[0], &status, sizeof(status));
			wait(NULL);
			close(pipe_fd[0]);
			close(pty_fd);
			errno = status;
			show_result(status ? EOF : 0, 0);
		}
	}
	UnsetCapability();

	capability = "SYS_TIME";
	SetCapability();
	if (write_policy()) {
		struct timeval tv;
		struct timezone tz;
		struct timex buf;
		time_t now = time(NULL);
		show_result(stime(&now), 1);
		gettimeofday(&tv, &tz);
		show_result(settimeofday(&tv, &tz), 1);
		memset(&buf, 0, sizeof(buf));
		buf.modes = 0x100; /* Use invalid value so that the clock
				      won't change. */
		show_result(adjtimex(&buf), 1);
		delete_policy();
		now = time(NULL);
		show_result(stime(&now), 0);
		gettimeofday(&tv, &tz);
		show_result(settimeofday(&tv, &tz), 0);
		memset(&buf, 0, sizeof(buf));
		buf.modes = 0x100; /* Use invalid value so that the clock
				      won't change. */
		show_result(adjtimex(&buf), 0);
	}
	UnsetCapability();

	capability = "SYS_NICE";
	SetCapability();
	if (write_policy()) {
		show_result(nice(0), 1);
		show_result(setpriority(PRIO_PROCESS, pid,
					getpriority(PRIO_PROCESS, pid)), 1);
		delete_policy();
		show_result(nice(0), 0);
		show_result(setpriority(PRIO_PROCESS, pid,
					getpriority(PRIO_PROCESS, pid)), 0);
	}
	UnsetCapability();

	capability = "SYS_SETHOSTNAME";
	SetCapability();
	if (write_policy()) {
		char buffer[4096];
		memset(buffer, 0, sizeof(buffer));
		gethostname(buffer, sizeof(buffer) - 1);
		show_result(sethostname(buffer, strlen(buffer)), 1);
		getdomainname(buffer, sizeof(buffer) - 1);
		show_result(setdomainname(buffer, strlen(buffer)), 1);
		delete_policy();
		gethostname(buffer, sizeof(buffer) - 1);
		show_result(sethostname(buffer, strlen(buffer)), 0);
		getdomainname(buffer, sizeof(buffer) - 1);
		show_result(setdomainname(buffer, strlen(buffer)), 0);
	}
	UnsetCapability();

	capability = "SYS_LINK";
	SetCapability();
	if (write_policy()) {
		strcpy(tmp1, "/tmp/link_source_XXXXXX");
		close(mkstemp(tmp1));
		strcpy(tmp2, "/tmp/link_target_XXXXXX");
		show_result(link(tmp1, tmp2), 1);
		unlink(tmp2);
		unlink(tmp1);
		delete_policy();
		strcpy(tmp1, "/tmp/link_source_XXXXXX");
		close(mkstemp(tmp1));
		strcpy(tmp2, "/tmp/link_target_XXXXXX");
		show_result(link(tmp1, tmp2), 0);
		unlink(tmp2);
		unlink(tmp1);
	}
	UnsetCapability();

	capability = "SYS_SYMLINK";
	SetCapability();
	if (write_policy()) {
		strcpy(tmp1, "/tmp/symlink_target_XXXXXX");
		close(mkstemp(tmp1));
		strcpy(tmp2, "/tmp/symlink_source_XXXXXX");
		show_result(symlink(tmp1, tmp2), 1);
		unlink(tmp2);
		unlink(tmp1);
		delete_policy();
		strcpy(tmp1, "/tmp/symlink_target_XXXXXX");
		close(mkstemp(tmp1));
		strcpy(tmp2, "/tmp/symlink_source_XXXXXX");
		show_result(symlink(tmp1, tmp2), 0);
		unlink(tmp2);
		unlink(tmp1);
	}
	UnsetCapability();

	capability = "SYS_RENAME";
	SetCapability();
	if (write_policy()) {
		strcpy(tmp1, "/tmp/rename_old_XXXXXX");
		close(mkstemp(tmp1));
		strcpy(tmp2, "/tmp/rename_new_XXXXXX");
		show_result(rename(tmp1, tmp2), 1);
		unlink(tmp2);
		unlink(tmp1);
		delete_policy();
		strcpy(tmp1, "/tmp/rename_old_XXXXXX");
		close(mkstemp(tmp1));
		strcpy(tmp2, "/tmp/rename_new_XXXXXX");
		show_result(rename(tmp1, tmp2), 0);
		unlink(tmp2);
		unlink(tmp1);
	}
	UnsetCapability();

	capability = "SYS_UNLINK";
	SetCapability();
	if (write_policy()) {
		strcpy(tmp1, "/tmp/unlinkXXXXXX");
		close(mkstemp(tmp1));
		show_result(unlink(tmp1), 1);
		delete_policy();
		strcpy(tmp1, "/tmp/unlinkXXXXXX");
		close(mkstemp(tmp1));
		show_result(unlink(tmp1), 0);
	}
	UnsetCapability();
	unlink(tmp1);

	capability = "SYS_CHMOD";
	SetCapability();
	if (write_policy()) {
		show_result(chmod("/dev/null", 0222), 1);
		delete_policy();
		show_result(chmod("/dev/null", 0444), 0);
	}
	UnsetCapability();
	chmod("/dev/null", 0666);

	capability = "SYS_CHOWN";
	SetCapability();
	if (write_policy()) {
		show_result(chown("/dev/null", 1, 1), 1);
		delete_policy();
		show_result(chown("/dev/null", 2, 2), 0);
	}
	UnsetCapability();
	chown("/dev/null", 0, 0);

	capability = "SYS_IOCTL";
	SetCapability();
	if (write_policy()) {
		int fd = open("/dev/null", O_RDONLY);
		show_result(ioctl(fd, 0 /* Use invalid value so that nothing
					   happen. */), 1);
		delete_policy();
		show_result(ioctl(fd, 0 /* Use invalid value so that nothing
					   happen. */), 0);
		close(fd);
	}
	UnsetCapability();

	capability = "SYS_PTRACE";
	SetCapability();
	if (write_policy()) {
		int status = 0;
		int pipe_fd[2] = { EOF, EOF };
		pipe(pipe_fd);
		switch (fork()) {
		case 0:
			errno = 0;
			ptrace(PTRACE_TRACEME, 0, NULL, NULL);
			status = errno;
			write(pipe_fd[1], &status, sizeof(status));
			_exit(0);
		case -1:
			fprintf(stderr, "fork() failed.\n");
			break;
		default:
			close(pipe_fd[1]);
			read(pipe_fd[0], &status, sizeof(status));
			wait(NULL);
			close(pipe_fd[0]);
			errno = status;
			show_result(status ? EOF : 0, 1);
		}
		delete_policy();
		status = 0;
		pipe(pipe_fd);
		switch (fork()) {
		case 0:
			errno = 0;
			ptrace(PTRACE_TRACEME, 0, NULL, NULL);
			status = errno;
			write(pipe_fd[1], &status, sizeof(status));
			_exit(0);
		case -1:
			fprintf(stderr, "fork() failed.\n");
			break;
		default:
			close(pipe_fd[1]);
			read(pipe_fd[0], &status, sizeof(status));
			wait(NULL);
			close(pipe_fd[0]);
			errno = status;
			show_result(status ? EOF : 0, 0);
		}
	}
	UnsetCapability();
}

int main(int argc, char *argv[])
{
	Init();
	domain_fd = open(proc_policy_domain_policy, O_WRONLY);
	{
		int self_fd = open(proc_policy_self_domain, O_RDONLY);
		memset(self_domain, 0, sizeof(self_domain));
		read(self_fd, self_domain, sizeof(self_domain) - 1);
		close(self_fd);
		write(domain_fd, self_domain, strlen(self_domain));
		write(domain_fd, "\n", 1);
	}
	StageCapabilityTest();
	close(domain_fd);
	ClearStatus();
	return 0;
}
