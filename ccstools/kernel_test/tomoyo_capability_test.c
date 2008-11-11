/*
 * tomoyo_capability_test.c
 *
 * Testing program for fs/tomoyo_capability.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5   2008/11/11
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
static int is_enforce = 0;

static void show_prompt(const char *str)
{
	if (domain_fd != EOF)
		printf("Testing %34s: (%s) ", str, "should success");
	else
		printf("Testing %34s: (%s) ", str,
		       is_enforce ? "must fail" : "should success");
	errno = 0;
}

static void show_result(int result)
{
	if (domain_fd != EOF) {
		if (result != EOF)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
	} else if (is_enforce) {
		if (result == EOF) {
			if (errno == EPERM)
				printf("OK: Permission denied.\n");
			else
				printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG!\n");
		}
	} else {
		if (result != EOF)
			printf("OK\n");
		else
			printf("FAILED: %s\n", strerror(errno));
	}
}

static void set_capability(const char *capability)
{
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "MAC_FOR_CAPABILITY::%s=%d\n",
		 capability, is_enforce ? 3 : 2);
	write_status(buffer);
	if (domain_fd != EOF) {
		snprintf(buffer, sizeof(buffer) - 1, "allow_capability %s\n",
			 capability);
		write(domain_fd, buffer, strlen(buffer));
	}
}

static void unset_capability(const char *capability)
{
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "MAC_FOR_CAPABILITY::%s=%d\n",
		 capability, 0);
	write_status(buffer);
	if (domain_fd != EOF) {
		snprintf(buffer, sizeof(buffer) - 1,
			 "delete allow_capability %s\n", capability);
		write(domain_fd, buffer, strlen(buffer));
	}
}

static void stage_capability_test(void)
{
	int fd;
	char tmp1[128];
	char tmp2[128];
	memset(tmp1, 0, sizeof(tmp1));
	memset(tmp2, 0, sizeof(tmp2));
	set_capability("inet_tcp_create");
	show_prompt("inet_tcp_create");
	fd = socket(AF_INET, SOCK_STREAM, 0);
	show_result(fd);
	if (fd != EOF)
		close(fd);
	unset_capability("inet_tcp_create");

	{
		struct sockaddr_in addr;
		int fd1;
		int fd2;
		socklen_t size = sizeof(addr);
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(0);

		fd1 = socket(AF_INET, SOCK_STREAM, 0);
		bind(fd1, (struct sockaddr *) &addr, sizeof(addr));
		getsockname(fd1, (struct sockaddr *) &addr, &size);
		set_capability("inet_tcp_listen");
		show_prompt("inet_tcp_listen");
		show_result(listen(fd1, 5));
		unset_capability("inet_tcp_listen");

		fd2 = socket(AF_INET, SOCK_STREAM, 0);
		set_capability("inet_tcp_connect");
		show_prompt("inet_tcp_connect");
		show_result(connect(fd2, (struct sockaddr *) &addr,
				   sizeof(addr)));
		unset_capability("inet_tcp_connect");

		if (fd2 != EOF)
			close(fd2);
		if (fd1 != EOF)
			close(fd1);
	}

	set_capability("use_inet_udp");
	show_prompt("use_inet_udp");
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	show_result(fd);
	if (fd != EOF)
		close(fd);
	unset_capability("use_inet_udp");

	set_capability("use_inet_ip");
	show_prompt("use_inet_ip");
	fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	show_result(fd);
	if (fd != EOF)
		close(fd);
	unset_capability("use_inet_ip");

	set_capability("use_route");
	show_prompt("use_route");
	fd = socket(AF_ROUTE, SOCK_RAW, 0);
	show_result(fd);
	if (fd != EOF)
		close(fd);
	unset_capability("use_route");

	set_capability("use_packet");
	show_prompt("use_packet");
	fd = socket(AF_PACKET, SOCK_RAW, 0);
	show_result(fd);
	if (fd != EOF)
		close(fd);
	unset_capability("use_packet");

	set_capability("use_kernel_module");
	if (!is_kernel26) {
		show_prompt("use_kernel_module(create_module())");
		show_result((int) create_module("", 0));
	}
	show_prompt("use_kernel_module(init_module())");
	show_result(init_module("", NULL));
	show_prompt("use_kernel_module(delete_module())");
	show_result(delete_module(""));
	unset_capability("use_kernel_module");

	set_capability("create_fifo");
	show_prompt("create_fifo");
	strcpy(tmp1, "/tmp/XXXXXX");
	close(mkstemp(tmp1));
	unlink(tmp1);
	show_result(mknod(tmp1, S_IFIFO, 0));
	unlink(tmp1);
	unset_capability("create_fifo");

	set_capability("create_block_dev");
	show_prompt("create_block_dev");
	strcpy(tmp1, "/tmp/XXXXXX");
	close(mkstemp(tmp1));
	unlink(tmp1);
	show_result(mknod(tmp1, S_IFBLK, MKDEV(1, 0)));
	unlink(tmp1);
	unset_capability("create_block_dev");

	set_capability("create_char_dev");
	show_prompt("create_char_dev");
	strcpy(tmp1, "/tmp/XXXXXX");
	close(mkstemp(tmp1));
	unlink(tmp1);
	show_result(mknod(tmp1, S_IFCHR, MKDEV(1, 3)));
	unlink(tmp1);
	unset_capability("create_char_dev");

	set_capability("create_unix_socket");
	show_prompt("create_unix_socket(mknod)");
	strcpy(tmp1, "/tmp/XXXXXX");
	close(mkstemp(tmp1));
	unlink(tmp1);
	show_result(mknod(tmp1, S_IFSOCK, 0));
	unlink(tmp1);
	{
		struct sockaddr_un addr;
		int fd;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(tmp1, "/tmp/XXXXXX");
		strncpy(addr.sun_path, tmp1, sizeof(addr.sun_path) - 1);
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		show_prompt("create_unix_socket(bind)");
		show_result(bind(fd, (struct sockaddr *) &addr, sizeof(addr)));
		unlink(tmp1);
		if (fd != EOF)
			close(fd);
	}
	unset_capability("create_unix_socket");

	set_capability("SYS_MOUNT");
	show_prompt("SYS_MOUNT");
	show_result(mount("/", "/", "nonexistent", 0, NULL));
	unset_capability("SYS_MOUNT");

	set_capability("SYS_UMOUNT");
	show_prompt("SYS_UMOUNT");
	show_result(umount("/"));
	unset_capability("SYS_UMOUNT");
	if (access("/", W_OK))
		mount("", "/", "", MS_REMOUNT, NULL);

	set_capability("SYS_REBOOT");
	show_prompt("SYS_REBOOT");
	show_result(reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
			  0x0000C0DE /* Use invalid value so that the system
					won't reboot. */, NULL));
	unset_capability("SYS_REBOOT");

	set_capability("SYS_CHROOT");
	show_prompt("SYS_CHROOT");
	show_result(chroot("/"));
	unset_capability("SYS_CHROOT");

	set_capability("SYS_PIVOT_ROOT");
	show_prompt("SYS_PIVOT_ROOT");

	{
		int error;
		char *stack = malloc(8192);
		const pid_t pid = clone(child, stack + (8192 / 2), CLONE_NEWNS,
					NULL);
		while (waitpid(pid, &error, __WALL) == EOF && errno == EINTR)
			error += 0; /* Dummy. */
		free(stack);
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		show_result(errno ? EOF : 0);
		unset_capability("SYS_PIVOT_ROOT");
	}

	signal(SIGINT, SIG_IGN);
	set_capability("SYS_KILL");
	show_prompt("SYS_KILL(sys_kill())");
	show_result(kill(pid, SIGINT));
	show_prompt("SYS_KILL(sys_tkill())");
	show_result(tkill(gettid(), SIGINT));
	if (is_kernel26) {
#ifdef __NR_tgkill
		show_prompt("SYS_KILL(sys_tgkill())");
		show_result(tgkill(pid, gettid(), SIGINT));
#endif
	}
	unset_capability("SYS_KILL");
	signal(SIGINT, SIG_DFL);

	set_capability("SYS_KEXEC_LOAD");
	if (is_kernel26) {
#ifdef __NR_sys_kexec_load
		show_prompt("SYS_KEXEC_LOAD");
		show_result(sys_kexec_load(0, 0, NULL, 0));
#endif
	}
	unset_capability("SYS_KEXEC_LOAD");

	{
		int pty_fd = EOF;
		int status = 0;
		int pipe_fd[2] = { EOF, EOF };
		pipe(pipe_fd);
		set_capability("SYS_VHANGUP");
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
			show_prompt("SYS_VHANGUP");
			errno = status;
			show_result(status ? EOF : 0);
		}
		unset_capability("SYS_VHANGUP");
	}

	{
		struct timeval tv;
		struct timezone tz;
		struct timex buf;
		time_t now = time(NULL);
		set_capability("SYS_TIME");
		show_prompt("SYS_TIME(stime())");
		show_result(stime(&now));
		gettimeofday(&tv, &tz);
		show_prompt("SYS_TIME(settimeofday())");
		show_result(settimeofday(&tv, &tz));
		memset(&buf, 0, sizeof(buf));
		buf.modes = 0x100; /* Use invalid value so that the clock won't
				      change. */
		show_prompt("SYS_TIME(adjtimex())");
		show_result(adjtimex(&buf));
		unset_capability("SYS_TIME");
	}

	set_capability("SYS_NICE");
	show_prompt("SYS_NICE(nice())");
	show_result(nice(0));
	show_prompt("SYS_NICE(setpriority())");
	show_result(setpriority(PRIO_PROCESS, pid,
			       getpriority(PRIO_PROCESS, pid)));
	unset_capability("SYS_NICE");

	{
		char buffer[4096];
		memset(buffer, 0, sizeof(buffer));
		set_capability("SYS_SETHOSTNAME");
		gethostname(buffer, sizeof(buffer) - 1);
		show_prompt("SYS_SETHOSTNAME(sethostname())");
		show_result(sethostname(buffer, strlen(buffer)));
		getdomainname(buffer, sizeof(buffer) - 1);
		show_prompt("SYS_SETHOSTNAME(setdomainname())");
		show_result(setdomainname(buffer, strlen(buffer)));
		unset_capability("SYS_SETHOSTNAME");
	}

	set_capability("SYS_LINK");
	show_prompt("SYS_LINK");
	strcpy(tmp1, "/tmp/link_source_XXXXXX");
	close(mkstemp(tmp1));
	strcpy(tmp2, "/tmp/link_target_XXXXXX");
	show_result(link(tmp1, tmp2));
	unlink(tmp2);
	unlink(tmp1);
	unset_capability("SYS_LINK");

	set_capability("SYS_SYMLINK");
	show_prompt("SYS_SYMLINK");
	strcpy(tmp1, "/tmp/symlink_target_XXXXXX");
	close(mkstemp(tmp1));
	strcpy(tmp2, "/tmp/symlink_source_XXXXXX");
	show_result(symlink(tmp1, tmp2));
	unlink(tmp2);
	unlink(tmp1);
	unset_capability("SYS_SYMLINK");

	set_capability("SYS_RENAME");
	show_prompt("SYS_RENAME");
	strcpy(tmp1, "/tmp/rename_old_XXXXXX");
	close(mkstemp(tmp1));
	strcpy(tmp2, "/tmp/rename_new_XXXXXX");
	show_result(rename(tmp1, tmp2));
	unlink(tmp2);
	unlink(tmp1);
	unset_capability("SYS_RENAME");

	set_capability("SYS_UNLINK");
	show_prompt("SYS_UNLINK");
	strcpy(tmp1, "/tmp/unlinkXXXXXX");
	close(mkstemp(tmp1));
	show_result(unlink(tmp1));
	unset_capability("SYS_UNLINK");
	unlink(tmp1);

	set_capability("SYS_CHMOD");
	show_prompt("SYS_CHMOD");
	show_result(chmod("/dev/null", 0));
	chmod("/dev/null", 0666);
	unset_capability("SYS_CHMOD");

	set_capability("SYS_CHOWN");
	show_prompt("SYS_CHOWN");
	show_result(chown("/dev/null", 1, 1));
	chown("/dev/null", 0, 0);
	unset_capability("SYS_CHOWN");

	set_capability("SYS_IOCTL");
	{
		int fd = open("/dev/null", O_RDONLY);
		show_prompt("SYS_IOCTL");
		show_result(ioctl(fd, 0 /* Use invalid value so that nothing
					  happen. */));
		close(fd);
	}
	unset_capability("SYS_IOCTL");

	{
		int status = 0;
		int pipe_fd[2] = { EOF, EOF };
		pipe(pipe_fd);
		set_capability("SYS_PTRACE");
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
			show_prompt("SYS_PTRACE");
			errno = status;
			show_result(status ? EOF : 0);
		}
		unset_capability("SYS_PTRACE");
	}
}

int main(int argc, char *argv[])
{
	ccs_test_pre_init();
	ccs_test_init();
	printf("***** Testing capability hooks in enforce mode. *****\n");
	is_enforce = 1;
	stage_capability_test();
	printf("\n\n");
	printf("***** Testing capability hooks in permissive mode. *****\n");
	is_enforce = 0;
	stage_capability_test();
	printf("\n\n");
	domain_fd = open(proc_policy_domain_policy, O_WRONLY);
	printf("***** Testing capability hooks in enforce mode with policy. "
	       "*****\n");
	is_enforce = 1;
	{
		char self_domain[4096];
		int self_fd = open(proc_policy_self_domain, O_RDONLY);
		memset(self_domain, 0, sizeof(self_domain));
		read(self_fd, self_domain, sizeof(self_domain) - 1);
		close(self_fd);
		write(domain_fd, self_domain, strlen(self_domain));
		write(domain_fd, "\n", 1);
	}
	stage_capability_test();
	printf("\n\n");
	close(domain_fd);
	clear_status();
	return 0;
}
