/*
 * sakura_bind_test.c
 *
 * Testing program for fs/sakura_bind.c
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.3.1   2006/12/08
 *
 */
#include "include.h"

static int min_port = 0, max_port = 0;
static unsigned short int ipv4_listener_port = 0;
static unsigned short int ipv6_listener_port = 0; 

static void IPv4_TCP_Bind(void) {
	// Try to bind as many as possible.
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(0);
			if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); exit(1);
			} else {
				const int port = ntohs(addr.sin_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv4/TCP bind    port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static void IPv4_TCP_Connect(void) {
	// Try to connect as many as possible.
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			addr.sin_port = htons(ipv4_listener_port);
			if (connect(fd, (struct sockaddr *) &addr, sizeof(addr))) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv4/TCP connect port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static void IPv4_UDP_Bind(void) {
	// Try to bind as many as possible.
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET, SOCK_DGRAM, 0);
			struct sockaddr_in addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(0);
			if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv4/UDP bind    port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static void IPv4_UDP_Connect(void) {
	// Try to connect as many as possible.
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET, SOCK_DGRAM, 0);
			struct sockaddr_in addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			addr.sin_port = htons(ipv4_listener_port);
			if (connect(fd, (struct sockaddr *) &addr, sizeof(addr))) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv4/UDP connect port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static void IPv4_UDP_SendTo(void) {	
	// Try to send as many as possible.
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET, SOCK_DGRAM, 0);
			struct sockaddr_in addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			addr.sin_port = htons(ipv4_listener_port);
			if (sendto(fd, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr)) != 1) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv4/UDP sendto  port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static void IPv6_TCP_Bind(void) {
	// Try to bind as many as possible. 
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET6, SOCK_STREAM, 0);
			struct sockaddr_in6 addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			addr.sin6_addr = in6addr_any;
			addr.sin6_port = htons(0);
			if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) break;
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin6_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv6/TCP bind    port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static void IPv6_TCP_Connect(void) {	
	// Try to connect as many as possible.
	int status = 0;
	if (fork() == 0) {
		int i = 0;
		alarm(1);
		while (1) {
			const int fd = socket(AF_INET6, SOCK_STREAM, 0);
			struct sockaddr_in6 addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			addr.sin6_addr = in6addr_loopback;
			addr.sin6_port = htons(ipv6_listener_port);
			if (connect(fd, (struct sockaddr *) &addr, sizeof(addr))) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin6_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv6/TCP connect port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(&status);
	if (WIFSIGNALED(status)) printf("IPv6/TCP connect test timed out.\n"); fflush(stdout);
}

static void IPv6_UDP_Bind(void) {
	// Try to bind as many as possible.
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET6, SOCK_DGRAM, 0);
			struct sockaddr_in6 addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			addr.sin6_addr = in6addr_any;
			addr.sin6_port = htons(0);
			if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin6_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv6/UDP bind    port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static void IPv6_UDP_Connect(void) {
	// Try to connect as many as possible.
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET6, SOCK_DGRAM, 0);
			struct sockaddr_in6 addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			addr.sin6_addr = in6addr_loopback;
			addr.sin6_port = htons(ipv6_listener_port);
			if (connect(fd, (struct sockaddr *) &addr, sizeof(addr))) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin6_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv6/UDP connect port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static void IPv6_UDP_SendTo(void) {	
	// Try to send as many as possible.
	if (fork() == 0) {
		int i = 0;
		while (1) {
			const int fd = socket(AF_INET6, SOCK_DGRAM, 0);
			struct sockaddr_in6 addr;
			socklen_t size = sizeof(addr);
			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			addr.sin6_addr = in6addr_loopback;
			addr.sin6_port = htons(ipv6_listener_port);
			if (sendto(fd, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr)) != 1) break;
			size = sizeof(addr);
			if (getsockname(fd, (struct sockaddr *) &addr, &size)) {
				printf("getsockname() failed.\n"); fflush(stdout); _exit(1);
			} else {
				const int port = ntohs(addr.sin6_port);
				if (!port || (port >= min_port && port <= max_port)) {
					printf("BUG! Reserved port was assigned.\n"); fflush(stdout); _exit(1);
				}
				i++;
			}
		}
		printf("IPv6/UDP sendto  port exhausted at %d\n", i); fflush(stdout);
		_exit(0);
	}
	wait(NULL);
}

static int system_fd = EOF;

static void SetReservedRange(int low, int high) {
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "deny_autobind %d-%d\n", min_port, max_port);
	write(system_fd, buffer, strlen(buffer));
}

static void UnsetReservedRange(int low, int high) {
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "delete deny_autobind %d-%d\n", min_port, max_port);
	write(system_fd, buffer, strlen(buffer));
}

int main(int argc, char *argv[]) {
	int ipv4_listener_socket = EOF;
	int ipv6_listener_socket = EOF;
	pid_t ipv4_pid = 0;
	pid_t ipv6_pid = 0;
	Init();
	if ((system_fd = open("/proc/ccs/policy/system_policy", O_RDWR)) == EOF) {
		fprintf(stderr, "Can't open /proc/ccs/policy/system_policy .\n");
		return 1;
	}
	if (write(system_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to /proc/ccs/policy/manager to run this program.\n");
		return 1;
	}
	{
		FILE *fp = fopen("/proc/sys/net/ipv4/ip_local_port_range", "r");
		int original_range[2], narrow_range[2] = { 32768, 32768 + 100 };
		min_port = narrow_range[0] + 20;
		max_port = narrow_range[1] - 20;
		if (!fp || fscanf(fp, "%u %u", &original_range[0], &original_range[1]) != 2) {
			fprintf(stderr, "Can't open /proc/sys/net/ipv4/ip_local_port_range .\n");
			exit(1);
		}
		fclose(fp);
		if ((fp = fopen("/proc/sys/net/ipv4/ip_local_port_range", "w")) == NULL) {
			fprintf(stderr, "Can't open /proc/sys/net/ipv4/ip_local_port_range .\n");
			exit(1);
		}
		WriteStatus("RESTRICT_AUTOBIND=3\n");
		{
			struct sockaddr_in addr;
			socklen_t size = sizeof(addr);
			ipv4_listener_socket = socket(AF_INET, SOCK_STREAM, 0);
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(0);
			bind(ipv4_listener_socket, (struct sockaddr *) &addr, sizeof(addr));
			getsockname(ipv4_listener_socket, (struct sockaddr *) &addr, &size);
			ipv4_listener_port = ntohs(addr.sin_port);
			listen(ipv4_listener_socket, 512);
			ipv4_pid = fork();
			if (ipv4_pid == 0) {
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(ipv4_listener_socket, &fds);
				while (1) {
					fd_set temp_fds = fds;
					int fd, fd2;
					if (select(1024, &temp_fds, NULL, NULL, NULL) == EOF) {
						fprintf(stderr, "select=%s\n", strerror(errno));
						continue;
					}
					for (fd = 0; fd < 1024; fd++) {
						if (!FD_ISSET(fd, &temp_fds)) continue;
						if (fd == ipv4_listener_socket) {
							size = sizeof(addr);
							fd2 = accept(ipv4_listener_socket, (struct sockaddr *) &addr, &size);
							if (fd2 != EOF) FD_SET(fd2, &fds);
							else fprintf(stderr, "accept=%s\n", strerror(errno));
						} else {
							//fprintf(stderr, "closed %d\n", fd);
							close(fd); FD_CLR(fd, &fds);
						}
					}
				}
			}
		}
		{
			struct sockaddr_in6 addr;
			socklen_t size = sizeof(addr);
			ipv6_listener_socket = socket(AF_INET6, SOCK_STREAM, 0);
			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			addr.sin6_addr = in6addr_any;
			addr.sin6_port = htons(0);
			bind(ipv6_listener_socket, (struct sockaddr *) &addr, sizeof(addr));
			getsockname(ipv6_listener_socket, (struct sockaddr *) &addr, &size);
			ipv6_listener_port = ntohs(addr.sin6_port);
			listen(ipv6_listener_socket, 512);
			ipv6_pid = fork();
			if (ipv6_pid == 0) {
				fd_set fds;
				FD_ZERO(&fds);
				if (ipv6_listener_socket == EOF) while (1) sleep(1000);
				FD_SET(ipv6_listener_socket, &fds);
				while (1) {
					fd_set temp_fds = fds;
					int fd, fd2;
					if (select(1024, &temp_fds, NULL, NULL, NULL) == EOF) {
						fprintf(stderr, "select=%s\n", strerror(errno));
						continue;
					}
					for (fd = 0; fd < 1024; fd++) {
						if (!FD_ISSET(fd, &temp_fds)) continue;
						if (fd == ipv6_listener_socket) {
							size = sizeof(addr);
							fd2 = accept(ipv6_listener_socket, (struct sockaddr *) &addr, &size);
							if (fd2 != EOF) FD_SET(fd2, &fds);
							else fprintf(stderr, "accept=%s\n", strerror(errno));
						} else {
							//fprintf(stderr, "closed %d\n", fd);
							close(fd); FD_CLR(fd, &fds);
						}
					}
				}
			}
		}

		{
			int stage;
			for (stage = 0; stage < 10; stage++) {
				narrow_range[0] += 1024;
				narrow_range[1] += 1024;
				min_port += 1024;
				max_port += 1024;
				fprintf(fp, "%d %d\n", narrow_range[0], narrow_range[1]);
				fflush(fp);
				SetReservedRange(min_port, max_port);
				switch (stage) {
				case 0:
					IPv4_TCP_Bind();
					break;
				case 1:
					IPv4_TCP_Connect();
					break;
				case 2:
					IPv4_UDP_Bind();
					break;
				case 3:
					IPv4_UDP_Connect();
					break;
				case 4:
					IPv4_UDP_SendTo();
					break;
				case 5:
					IPv6_TCP_Bind();
					break;
				case 6:
					IPv6_TCP_Connect();
					break;
				case 7:
					IPv6_UDP_Bind();
					break;
				case 8:
					IPv6_UDP_Connect();
					break;
				case 9:
					IPv6_UDP_SendTo();
					break;
				}
				UnsetReservedRange(min_port, max_port);
			}
		}
		kill(ipv4_pid, SIGHUP);
		kill(ipv6_pid, SIGHUP);
		close(ipv4_listener_socket);
		close(ipv6_listener_socket);
		WriteStatus("RESTRICT_AUTOBIND=0\n");
		fprintf(fp, "%d %d\n", original_range[0], original_range[1]);
		fclose(fp);
	}
	printf("Done.\n");
	close(system_fd);
	ClearStatus();
	return 0;
}
