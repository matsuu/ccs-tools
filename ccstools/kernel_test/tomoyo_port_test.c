/*
 * tomoyo_port_test.c
 *
 * Testing program for fs/tomoyo_port.c
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.3.1   2006/12/08
 *
 */
#include "include.h"

static int is_enforce = 0;

static void ShowPrompt(const char *str) {
	printf("Testing %21s: (%s) ", str, is_enforce ? "must fail" : "should success");
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

static void StageNetworkTest(void) {
	int i;
	static char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	for (i = 0; i <= 2048; i += 512) {
		struct sockaddr_in addr;
		socklen_t size = sizeof(addr);
		const int fd1 = socket(AF_INET, SOCK_STREAM, 0);
		const int fd2 = socket(AF_INET, SOCK_STREAM, 0);
		const int fd3 = socket(AF_INET, SOCK_DGRAM, 0);
		const int fd4 = socket(AF_INET, SOCK_DGRAM, 0);
		const int fd5 = socket(AF_INET, SOCK_DGRAM, 0);

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(i);
		snprintf(buffer, sizeof(buffer) - 1, "IPv4-Bind TCP/%d", i);
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		getsockname(fd1, (struct sockaddr *) &addr, &size); listen(fd1, 5);
		snprintf(buffer, sizeof(buffer) - 1, "IPv4-Connect TCP/%d", i);
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		if (fd2 != EOF) close(fd2);
		if (fd1 != EOF) close(fd1);
		
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(i);
		snprintf(buffer, sizeof(buffer) - 1, "IPv4-Bind UDP/%d", i);
		ShowPrompt(buffer);
		ShowResult(bind(fd3, (struct sockaddr *) &addr, sizeof(addr)));
		getsockname(fd3, (struct sockaddr *) &addr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "IPv4-Connect UDP/%d", i);
		ShowPrompt(buffer);
		ShowResult(connect(fd4, (struct sockaddr *) &addr, sizeof(addr)));
		if (fd4 != EOF) close(fd4);
		if (fd3 != EOF) close(fd3);

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(i);
		snprintf(buffer, sizeof(buffer) - 1, "IPv4-SendTo UDP/%d", i);
		ShowPrompt(buffer);
		ShowResult(sendto(fd5, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr)));
		if (fd5 != EOF) close(fd5);
	}
	if ((i = socket(AF_INET6, SOCK_STREAM, 0)) == EOF) return;
	close(i);
	for (i = 0; i <= 2048; i += 512) {
		struct sockaddr_in6 addr;
		socklen_t size = sizeof(addr);
		const int fd1 = socket(AF_INET6, SOCK_STREAM, 0);
		const int fd2 = socket(AF_INET6, SOCK_STREAM, 0);
		const int fd3 = socket(AF_INET6, SOCK_DGRAM, 0);
		const int fd4 = socket(AF_INET6, SOCK_DGRAM, 0);
		const int fd5 = socket(AF_INET6, SOCK_DGRAM, 0);

		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_addr = in6addr_loopback;
		addr.sin6_port = htons(i);
		snprintf(buffer, sizeof(buffer) - 1, "IPv6-Bind TCP/%d", i);
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		getsockname(fd1, (struct sockaddr *) &addr, &size); listen(fd1, 5);
		snprintf(buffer, sizeof(buffer) - 1, "IPv6-Connect TCP/%d", i);
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		if (fd2 != EOF) close(fd2);
		if (fd1 != EOF) close(fd1);

		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_addr = in6addr_loopback;
		addr.sin6_port = htons(i);
		snprintf(buffer, sizeof(buffer) - 1, "IPv6-Bind UDP/%d", i);
		ShowPrompt(buffer);
		ShowResult(bind(fd3, (struct sockaddr *) &addr, sizeof(addr)));
		getsockname(fd3, (struct sockaddr *) &addr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "IPv6-Connect UDP/%d", i);
		ShowPrompt(buffer);
		ShowResult(connect(fd4, (struct sockaddr *) &addr, sizeof(addr)));
		if (fd4 != EOF) close(fd4);
		if (fd3 != EOF) close(fd3);

		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_addr = in6addr_loopback;
		addr.sin6_port = htons(i);
		snprintf(buffer, sizeof(buffer) - 1, "IPv6-SendTo UDP/%d", i);
		ShowPrompt(buffer);
		ShowResult(sendto(fd5, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr)));
		if (fd5 != EOF) close(fd5);
	}
}

static void SetNetworkEnforce(int enforce) {
	if (enforce) {
		WriteStatus("MAC_FOR_BINDPORT=3\n"); WriteStatus("MAC_FOR_CONNECTPORT=3\n");
	} else {
		WriteStatus("MAC_FOR_BINDPORT=2\n"); WriteStatus("MAC_FOR_CONNECTPORT=2\n");
	}
}

int main(int argc, char *argv[]) {
	Init();
	
	printf("***** Testing network port hooks in enforce mode. *****\n");
	is_enforce = 1;
	SetNetworkEnforce(1);
	StageNetworkTest();
	printf("\n\n");

	printf("***** Testing network port hooks in permissive mode. *****\n");
	is_enforce = 0;
	SetNetworkEnforce(0);
	StageNetworkTest();
	ClearStatus();
	return 0;
}
