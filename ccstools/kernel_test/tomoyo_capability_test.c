/*
 * tomoyo_capability_test.c
 *
 * Testing program for fs/tomoyo_capability.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0-rc   2008/03/28
 *
 */
#include "include.h"

static int child(void *arg) {
	errno = 0;
	pivot_root("/proc", proc_policy_dir);
	return errno;
}

static int domain_fd = EOF;
static int is_enforce = 0;

static void ShowPrompt(const char *str) {
	if (domain_fd != EOF) printf("Testing %34s: (%s) ", str, "should success");
	else printf("Testing %34s: (%s) ", str, is_enforce ? "must fail" : "should success");
	errno = 0;
}

static void ShowResult(int result) {
	if (domain_fd != EOF) {
		if (result != EOF) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
	} else if (is_enforce) {
		if (result == EOF) {
			if (errno == EPERM) printf("OK: Permission denied.\n");
			else printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG!\n");
		}
	} else {
		if (result != EOF) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
	}
}

static void SetCapability(const char *capability) {
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "MAC_FOR_CAPABILITY::%s=%d\n", capability, is_enforce ? 3 : 2);
	WriteStatus(buffer);
	if (domain_fd != EOF) {
		snprintf(buffer, sizeof(buffer) - 1, "allow_capability %s\n", capability);
		write(domain_fd, buffer, strlen(buffer));
	}
}

static void UnsetCapability(const char *capability) {
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "MAC_FOR_CAPABILITY::%s=%d\n", capability, 0);
	WriteStatus(buffer);
	if (domain_fd != EOF) {
		snprintf(buffer, sizeof(buffer) - 1, "delete allow_capability %s\n", capability);
		write(domain_fd, buffer, strlen(buffer));
	}
}

static void StageCapabilityTest(void) {
	int fd;
	char tmp1[128], tmp2[128];
	memset(tmp1, 0, sizeof(tmp1));
	memset(tmp2, 0, sizeof(tmp2));
	SetCapability("inet_tcp_create");
	ShowPrompt("inet_tcp_create");
	fd = socket(AF_INET, SOCK_STREAM, 0);
	ShowResult(fd);
	if (fd != EOF) close(fd);
	UnsetCapability("inet_tcp_create");

	{
		struct sockaddr_in addr;
		int fd1, fd2;
		socklen_t size = sizeof(addr);
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(0);
		
		fd1 = socket(AF_INET, SOCK_STREAM, 0);
		bind(fd1, (struct sockaddr *) &addr, sizeof(addr));
		getsockname(fd1, (struct sockaddr *) &addr, &size);
		SetCapability("inet_tcp_listen");
		ShowPrompt("inet_tcp_listen");
		ShowResult(listen(fd1, 5));
		UnsetCapability("inet_tcp_listen");
		
		fd2 = socket(AF_INET, SOCK_STREAM, 0);
		SetCapability("inet_tcp_connect");
		ShowPrompt("inet_tcp_connect");
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		UnsetCapability("inet_tcp_connect");
		
		if (fd2 != EOF) close(fd2);
		if (fd1 != EOF) close(fd1);
	}
	
	SetCapability("use_inet_udp");
	ShowPrompt("use_inet_udp");
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ShowResult(fd);
	if (fd != EOF) close(fd);
	UnsetCapability("use_inet_udp");
	
	SetCapability("use_inet_ip");
	ShowPrompt("use_inet_ip");
	fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	ShowResult(fd);
	if (fd != EOF) close(fd);
	UnsetCapability("use_inet_ip");

	SetCapability("use_route");
	ShowPrompt("use_route");
	fd = socket(AF_ROUTE, SOCK_RAW, 0);
	ShowResult(fd);
	if (fd != EOF) close(fd);
	UnsetCapability("use_route");
	
	SetCapability("use_packet");
	ShowPrompt("use_packet");
	fd = socket(AF_PACKET, SOCK_RAW, 0);
	ShowResult(fd);
	if (fd != EOF) close(fd);
	UnsetCapability("use_packet");

	SetCapability("use_kernel_module");
	if (!is_kernel26) {
		ShowPrompt("use_kernel_module(create_module())");
		ShowResult((int) create_module("", 0));
	}
	ShowPrompt("use_kernel_module(init_module())");
	ShowResult(init_module("", NULL));
	ShowPrompt("use_kernel_module(delete_module())");
	ShowResult(delete_module(""));
	UnsetCapability("use_kernel_module");

	SetCapability("create_fifo");
	ShowPrompt("create_fifo");
	strcpy(tmp1, "/tmp/XXXXXX");
	close(mkstemp(tmp1));
	unlink(tmp1);
	ShowResult(mknod(tmp1, S_IFIFO, 0));
	unlink(tmp1);
	UnsetCapability("create_fifo");

	SetCapability("create_block_dev");
	ShowPrompt("create_block_dev");
	strcpy(tmp1, "/tmp/XXXXXX");
	close(mkstemp(tmp1));
	unlink(tmp1);
	ShowResult(mknod(tmp1, S_IFBLK, MKDEV(1, 0)));
	unlink(tmp1);
	UnsetCapability("create_block_dev");

	SetCapability("create_char_dev");
	ShowPrompt("create_char_dev");
	strcpy(tmp1, "/tmp/XXXXXX");
	close(mkstemp(tmp1));
	unlink(tmp1);
	ShowResult(mknod(tmp1, S_IFCHR, MKDEV(1, 3)));
	unlink(tmp1);
	UnsetCapability("create_char_dev");

	SetCapability("create_unix_socket");
	ShowPrompt("create_unix_socket(mknod)");
	strcpy(tmp1, "/tmp/XXXXXX");
	close(mkstemp(tmp1));
	unlink(tmp1);
	ShowResult(mknod(tmp1, S_IFSOCK, 0));
	unlink(tmp1);
	{
		struct sockaddr_un addr;
		int fd;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(tmp1, "/tmp/XXXXXX");
		strncpy(addr.sun_path, tmp1, sizeof(addr.sun_path) - 1);
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		ShowPrompt("create_unix_socket(bind)");
		ShowResult(bind(fd, (struct sockaddr *) &addr, sizeof(addr)));
		unlink(tmp1);
		if (fd != EOF) close(fd);
	}
	UnsetCapability("create_unix_socket");

	SetCapability("SYS_MOUNT");
	ShowPrompt("SYS_MOUNT");
	ShowResult(mount("/", "/", "nonexistent", 0, NULL));
	UnsetCapability("SYS_MOUNT");
	
	SetCapability("SYS_UMOUNT");
	ShowPrompt("SYS_UMOUNT");
	ShowResult(umount("/"));
	UnsetCapability("SYS_UMOUNT");
	if (access("/", W_OK)) mount("", "/", "", MS_REMOUNT, NULL);
	
	SetCapability("SYS_REBOOT");
	ShowPrompt("SYS_REBOOT");
	ShowResult(reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, 
					  0x0000C0DE /* Use invalid value so that the system won't reboot. */, NULL));
	UnsetCapability("SYS_REBOOT");
	
	SetCapability("SYS_CHROOT");
	ShowPrompt("SYS_CHROOT");
	ShowResult(chroot("/"));
	UnsetCapability("SYS_CHROOT");

	SetCapability("SYS_PIVOT_ROOT");
	ShowPrompt("SYS_PIVOT_ROOT");

	{
		int error;
		char *stack = malloc(8192);
		const pid_t pid = clone(child, stack + (8192 / 2), CLONE_NEWNS, NULL);
		while (waitpid(pid, &error, __WALL) == EOF && errno == EINTR);
		free(stack);
		errno = WIFEXITED(error) ? WEXITSTATUS(error) : -1;
		ShowResult(errno ? EOF : 0);
		UnsetCapability("SYS_PIVOT_ROOT");
	}

	signal(SIGINT, SIG_IGN);
	SetCapability("SYS_KILL");
	ShowPrompt("SYS_KILL(sys_kill())");
	ShowResult(kill(pid, SIGINT));
	ShowPrompt("SYS_KILL(sys_tkill())");
	ShowResult(tkill(gettid(), SIGINT));
	if (is_kernel26) {
#ifdef __NR_tgkill
		ShowPrompt("SYS_KILL(sys_tgkill())");
		ShowResult(tgkill(pid, gettid(), SIGINT));
#endif
	}
	UnsetCapability("SYS_KILL");
	signal(SIGINT, SIG_DFL);

	SetCapability("SYS_KEXEC_LOAD");
	if (is_kernel26) {
#ifdef __NR_sys_kexec_load
		ShowPrompt("SYS_KEXEC_LOAD");
		ShowResult(sys_kexec_load(0, 0, NULL, 0));
#endif
	}
	UnsetCapability("SYS_KEXEC_LOAD");
	
	{
		int pty_fd = EOF, status = 0;
		int pipe_fd[2] = { EOF, EOF };
		pipe(pipe_fd);
		SetCapability("SYS_VHANGUP");
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
			ShowPrompt("SYS_VHANGUP");
			errno = status;
			ShowResult(status ? EOF : 0);
		}
		UnsetCapability("SYS_VHANGUP");
	}

	{
		struct timeval tv;
		struct timezone tz;
		struct timex buf;
		time_t now = time(NULL);
		SetCapability("SYS_TIME");
		ShowPrompt("SYS_TIME(stime())");
		ShowResult(stime(&now));
		gettimeofday(&tv, &tz);
		ShowPrompt("SYS_TIME(settimeofday())");
		ShowResult(settimeofday(&tv, &tz));
		memset(&buf, 0, sizeof(buf));
		buf.modes = 0x100; /* Use invalid value so that the clock won't change. */
		ShowPrompt("SYS_TIME(adjtimex())");
		ShowResult(adjtimex(&buf));
		UnsetCapability("SYS_TIME");
	}
	
	SetCapability("SYS_NICE");
	ShowPrompt("SYS_NICE(nice())");
	ShowResult(nice(0));
	ShowPrompt("SYS_NICE(setpriority())");
	ShowResult(setpriority(PRIO_PROCESS, pid, getpriority(PRIO_PROCESS, pid)));
	UnsetCapability("SYS_NICE");

	{
		char buffer[4096];
		memset(buffer, 0, sizeof(buffer));
		SetCapability("SYS_SETHOSTNAME");
		gethostname(buffer, sizeof(buffer) - 1);
		ShowPrompt("SYS_SETHOSTNAME(sethostname())");
		ShowResult(sethostname(buffer, strlen(buffer)));
		getdomainname(buffer, sizeof(buffer) - 1);
		ShowPrompt("SYS_SETHOSTNAME(setdomainname())");
		ShowResult(setdomainname(buffer, strlen(buffer)));
		UnsetCapability("SYS_SETHOSTNAME");
	}
	
	SetCapability("SYS_LINK");
	ShowPrompt("SYS_LINK");
	strcpy(tmp1, "/tmp/link_source_XXXXXX");
	close(mkstemp(tmp1));
	strcpy(tmp2, "/tmp/link_target_XXXXXX");
	ShowResult(link(tmp1, tmp2));
	unlink(tmp2);
	unlink(tmp1);
	UnsetCapability("SYS_LINK");
	
	SetCapability("SYS_SYMLINK");
	ShowPrompt("SYS_SYMLINK");
	strcpy(tmp1, "/tmp/symlink_target_XXXXXX");
	close(mkstemp(tmp1));
	strcpy(tmp2, "/tmp/symlink_source_XXXXXX");
	ShowResult(symlink(tmp1, tmp2));
	unlink(tmp2);
	unlink(tmp1);
	UnsetCapability("SYS_SYMLINK");
	
	SetCapability("SYS_RENAME");
	ShowPrompt("SYS_RENAME");
	strcpy(tmp1, "/tmp/rename_old_XXXXXX");
	close(mkstemp(tmp1));
	strcpy(tmp2, "/tmp/rename_new_XXXXXX");
	ShowResult(rename(tmp1, tmp2));
	unlink(tmp2);
	unlink(tmp1);
	UnsetCapability("SYS_RENAME");

	SetCapability("SYS_UNLINK");
	ShowPrompt("SYS_UNLINK");
	strcpy(tmp1, "/tmp/unlinkXXXXXX");
	close(mkstemp(tmp1));
	ShowResult(unlink(tmp1));
	UnsetCapability("SYS_UNLINK");
	unlink(tmp1);
	
	SetCapability("SYS_CHMOD");
	ShowPrompt("SYS_CHMOD");
	ShowResult(chmod("/dev/null", 0));
	chmod("/dev/null", 0666);
	UnsetCapability("SYS_CHMOD");

	SetCapability("SYS_CHOWN");
	ShowPrompt("SYS_CHOWN");
	ShowResult(chown("/dev/null", 1, 1));
	chown("/dev/null", 0, 0);
	UnsetCapability("SYS_CHOWN");

	SetCapability("SYS_IOCTL");
	{
		int fd = open("/dev/null", O_RDONLY);
		ShowPrompt("SYS_IOCTL");
		ShowResult(ioctl(fd, 0 /* Use invalid value so that nothing happen. */));
		close(fd);
	}
	UnsetCapability("SYS_IOCTL");

	{
		int status = 0;
		int pipe_fd[2] = { EOF, EOF };
		pipe(pipe_fd);
		SetCapability("SYS_PTRACE");
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
			ShowPrompt("SYS_PTRACE");
			errno = status;
			ShowResult(status ? EOF : 0);
		}
		UnsetCapability("SYS_PTRACE");
	}
}

int main(int argc, char *argv[]) {
	PreInit();
	Init();
	printf("***** Testing capability hooks in enforce mode. *****\n");
	is_enforce = 1;
	StageCapabilityTest();
	printf("\n\n");
	printf("***** Testing capability hooks in permissive mode. *****\n");
	is_enforce = 0;
	StageCapabilityTest();
	printf("\n\n");
	domain_fd = open(proc_policy_domain_policy, O_WRONLY);
	printf("***** Testing capability hooks in enforce mode with policy. *****\n");
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
	StageCapabilityTest();
	printf("\n\n");	
	close(domain_fd);
	ClearStatus();
	return 0;
}
