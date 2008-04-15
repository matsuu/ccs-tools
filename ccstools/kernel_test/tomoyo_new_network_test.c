/*
 * tomoyo_new_network_test.c
 *
 * Testing program for fs/tomoyo_network.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0+   2008/04/15
 *
 */
#include "include.h"

static int domain_fd = EOF;
static const char *policy = "";
static char self_domain[4096] = "";

static int write_policy(void) {
	FILE *fp = fopen(proc_policy_domain_policy, "r");
	char buffer[8192];
	char *cp;
	int domain_found = 0;
	int policy_found = 0;
	memset(buffer, 0, sizeof(buffer));
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
	if (!fp) {
		printf("%s : BUG: policy read failed\n", policy);
		return 0;
	}
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		cp = strchr(buffer, '\n');
		if (cp) *cp = '\0';
		if (!strncmp(buffer, "<kernel>", 8)) domain_found = !strcmp(self_domain, buffer);
		if (domain_found) {
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

static void delete_policy(void) {
	write(domain_fd, "delete ", 7);
	write(domain_fd, policy, strlen(policy));
	write(domain_fd, "\n", 1);
}

static void show_result(int result, char should_success) {
	printf("%s : ", policy);
	if (should_success) {
		if (result != EOF) printf("OK\n");
		else printf("FAILED: %s\n", strerror(errno));
	} else {
		if (result == EOF) {
			if (errno == EPERM) printf("OK: Permission denied.\n");
			else printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG\n");
		}
	}
}


static void StageNetworkTest(void) {
	int i;

	{ /* IPv4 TCP */
		char buffer[1024];
		struct sockaddr_in saddr, caddr;
		socklen_t size = sizeof(saddr);
		int fd1 = socket(PF_INET, SOCK_STREAM, 0);
		int fd2 = socket(PF_INET, SOCK_STREAM, 0);
		int fd3 = EOF;
		memset(buffer, 0, sizeof(buffer));
		policy = buffer;
		
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		saddr.sin_port = htons(0);
		
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP bind 127.0.0.1 0-1");
		errno = 0;
		show_result(bind(fd1, (struct sockaddr *) &saddr, sizeof(saddr)), 0);
		if (write_policy()) {
			show_result(bind(fd1, (struct sockaddr *) &saddr, sizeof(saddr)), 1);
			delete_policy();
		}
		getsockname(fd1, (struct sockaddr *) &saddr, &size);

		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP listen 127.0.0.0-127.255.255.255 %u-%u", ntohs(saddr.sin_port) - 1, ntohs(saddr.sin_port) + 1);
		errno = 0;
		show_result(listen(fd1, 5), 0);
		if (write_policy()) {
			show_result(listen(fd1, 5), 1);
			delete_policy();
		}
		
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP connect 127.0.0.1 %u-%u", ntohs(saddr.sin_port) - 1, ntohs(saddr.sin_port) + 1);
		errno = 0;
		show_result(connect(fd2, (struct sockaddr *) &saddr, sizeof(saddr)), 0);
		if (write_policy()) {
			show_result(connect(fd2, (struct sockaddr *) &saddr, sizeof(saddr)), 1);
			delete_policy();
		}
		getsockname(fd2, (struct sockaddr *) &caddr, &size);
		
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP accept 127.0.0.1 %u-%u", ntohs(caddr.sin_port) - 1, ntohs(caddr.sin_port) + 1);
		errno = 0;
		fd3 = accept(fd1, (struct sockaddr *) &caddr, &size);
		show_result(fd3, 0);
		if (fd3 != EOF) close(fd3);

		close(fd2);
		fd2 = socket(PF_INET, SOCK_STREAM, 0);
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP connect 127.0.0.0-127.255.255.255 %u-%u", ntohs(saddr.sin_port) - 1, ntohs(saddr.sin_port) + 1);
		if (write_policy()) {
			connect(fd2, (struct sockaddr *) &saddr, sizeof(saddr));
			delete_policy();
		}
		getsockname(fd2, (struct sockaddr *) &caddr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP accept 127.0.0.0-127.255.255.255 %u-%u", ntohs(caddr.sin_port) - 1, ntohs(caddr.sin_port) + 1);
		if (write_policy()) {
			fd3 = accept(fd1, (struct sockaddr *) &caddr, &size);
			show_result(fd3, 1);
			delete_policy();
			if (fd3 != EOF) close(fd3);
		}

		if (fd2 != EOF) close(fd2);
		if (fd1 != EOF) close(fd1);
	}
	
	if ((i = socket(PF_INET6, SOCK_STREAM, 0)) == EOF) return;
	close(i);

	{ /* IPv6 TCP */
		char buffer[1024];
		struct sockaddr_in6 saddr, caddr;
		socklen_t size = sizeof(saddr);
		int fd1 = socket(PF_INET6, SOCK_STREAM, 0);
		int fd2 = socket(PF_INET6, SOCK_STREAM, 0);
		int fd3 = EOF;
		memset(buffer, 0, sizeof(buffer));
		policy = buffer;
		
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin6_family = AF_INET6;
		saddr.sin6_addr = in6addr_loopback;
		saddr.sin6_port = htons(0);
		
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP bind 0:0:0:0:0:0:0:1 0-1");
		errno = 0;
		show_result(bind(fd1, (struct sockaddr *) &saddr, sizeof(saddr)), 0);
		if (write_policy()) {
			show_result(bind(fd1, (struct sockaddr *) &saddr, sizeof(saddr)), 1);
			delete_policy();
		}
		getsockname(fd1, (struct sockaddr *) &saddr, &size);

		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP listen 0:0:0:0:0:0:0:0-0:0:0:0:0:0:0:ff %u-%u", ntohs(saddr.sin6_port) - 1, ntohs(saddr.sin6_port) + 1);
		errno = 0;
		show_result(listen(fd1, 5), 0);
		if (write_policy()) {
			show_result(listen(fd1, 5), 1);
			delete_policy();
		}
		
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP connect 0:0:0:0:0:0:0:1 %u-%u", ntohs(saddr.sin6_port) - 1, ntohs(saddr.sin6_port) + 1);
		errno = 0;
		show_result(connect(fd2, (struct sockaddr *) &saddr, sizeof(saddr)), 0);
		if (write_policy()) {
			show_result(connect(fd2, (struct sockaddr *) &saddr, sizeof(saddr)), 1);
			delete_policy();
		}
		getsockname(fd2, (struct sockaddr *) &caddr, &size);
		
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP accept 0:0:0:0:0:0:0:1 %u-%u", ntohs(caddr.sin6_port) - 1, ntohs(caddr.sin6_port) + 1);
		errno = 0;
		fd3 = accept(fd1, (struct sockaddr *) &caddr, &size);
		show_result(fd3, 0);
		if (fd3 != EOF) close(fd3);

		close(fd2);
		fd2 = socket(PF_INET6, SOCK_STREAM, 0);
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP connect 0:0:0:0:0:0:0:0-0:0:0:0:0:0:0:ff %u-%u", ntohs(saddr.sin6_port) - 1, ntohs(saddr.sin6_port) + 1);
		if (write_policy()) {
			connect(fd2, (struct sockaddr *) &saddr, sizeof(saddr));
			delete_policy();
		}
		getsockname(fd2, (struct sockaddr *) &caddr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "allow_network TCP accept 0:0:0:0:0:0:0:0-0:0:0:0:0:0:0:ff %u-%u", ntohs(caddr.sin6_port) - 1, ntohs(caddr.sin6_port) + 1);
		if (write_policy()) {
			fd3 = accept(fd1, (struct sockaddr *) &caddr, &size);
			show_result(fd3, 1);
			delete_policy();
			if (fd3 != EOF) close(fd3);
		}

		if (fd2 != EOF) close(fd2);
		if (fd1 != EOF) close(fd1);
	}
}

int main(int argc, char *argv[]) {
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
	WriteStatus("MAC_FOR_NETWORK=enforcing\n");
	WriteStatus("MAX_REJECT_LOG=1024\n");
	StageNetworkTest();
	close(domain_fd);
	ClearStatus();
	return 0;
}
