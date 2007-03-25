/*
 * tomoyo_network_test.c
 *
 * Testing program for fs/tomoyo_network.c
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.4   2007/04/01
 *
 */
#include "include.h"

static int is_enforce = 0;

static void ShowPrompt(const char *str) {
	printf("Testing %50s: (%s) ", str, is_enforce ? "must fail" : "should success");
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

static void ShowResult2(int result) {
	if (is_enforce) {
		if (result == EOF) {
			if (errno == EAGAIN) printf("OK: Permission denied.\n");
			else printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG!\n");
		}
	} else {
		if (result != EOF) printf("OK\n");
		else printf("%s\n", strerror(errno));
	}
}

static void ShowResult3(int result) {
	if (is_enforce) {
		if (result == EOF) {
			if (errno == ECONNABORTED) printf("OK: Permission denied.\n");
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
	{
		struct sockaddr_in addr, addr2;
		socklen_t size = sizeof(addr);
		int fd1 = EOF, fd2 = EOF, fd3 = EOF;
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Server: Creating socket(PF_INET, SOCK_STREAM, 0)");
		ShowPrompt(buffer);
		fd1 = socket(PF_INET, SOCK_STREAM, 0);
		ShowResult(fd1);
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(0);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Binding TCP 127.0.0.1 0");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));

		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		getsockname(fd1, (struct sockaddr *) &addr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Listening TCP 127.0.0.1 %d", ntohs(addr.sin_port));
		ShowPrompt(buffer);
		ShowResult(listen(fd1, 5));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(listen(fd1, 5));
		
		snprintf(buffer, sizeof(buffer) - 1, "Client: Creating socket(PF_INET, SOCK_STREAM, 0)");
		ShowPrompt(buffer);
		fd2 = socket(PF_INET, SOCK_STREAM, 0);
		ShowResult(fd1);
		
		snprintf(buffer, sizeof(buffer) - 1, "Client: Connecting TCP 127.0.0.1 %d", ntohs(addr.sin_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		
		getsockname(fd2, (struct sockaddr *) &addr2, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Accepting TCP 127.0.0.1 %d", ntohs(addr2.sin_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult3(fd3 = accept(fd1, (struct sockaddr *) &addr2, &size));
		close(fd3); fd3 = EOF;
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		close(fd2); fd2 = socket(PF_INET, SOCK_STREAM, 0);
		connect(fd2, (struct sockaddr *) &addr, sizeof(addr));
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		getsockname(fd2, (struct sockaddr *) &addr2, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Accepting TCP 127.0.0.1 %d", ntohs(addr2.sin_port));
		ShowPrompt(buffer);
		ShowResult3(fd3 = accept(fd1, (struct sockaddr *) &addr2, &size));
		
		close(fd3); fd3 = EOF;
		close(fd2); fd2 = EOF;
		close(fd1); fd1 = EOF;
	}
	
	{
		struct sockaddr_in addr, addr2;
		socklen_t size = sizeof(addr);
		int fd1 = EOF, fd2 = EOF;
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Server: Creating socket(PF_INET, SOCK_DGRAM, 0)");
		ShowPrompt(buffer);
		fd1 = socket(PF_INET, SOCK_DGRAM, 0);
		ShowResult(fd1);

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(0);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Binding UDP 127.0.0.1 0");
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Client: Creating socket(PF_INET, SOCK_DGRAM, 0)");
		ShowPrompt(buffer);
		fd2 = socket(PF_INET, SOCK_DGRAM, 0);
		ShowResult(fd1);

		getsockname(fd1, (struct sockaddr *) &addr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Client: Connecting UDP 127.0.0.1 %d", ntohs(addr.sin_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		
		snprintf(buffer, sizeof(buffer) - 1, "Client: Sending UDP 127.0.0.1 %d", ntohs(addr.sin_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(sendto(fd2, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(sendto(fd2, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr)));
		
		getsockname(fd2, (struct sockaddr *) &addr2, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Receiving UDP 127.0.0.1 %d", ntohs(addr2.sin_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult2(recvfrom(fd1, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &addr2, &size));

		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		sendto(fd2, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr));

		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Server: Receiving UDP 127.0.0.1 %d", ntohs(addr2.sin_port));
		ShowPrompt(buffer);
		ShowResult2(recvfrom(fd1, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &addr, &size));

		if (fd2 != EOF) close(fd2);
		if (fd1 != EOF) close(fd1);

	}

	{
		static struct iphdr ip;
		struct sockaddr_in addr, addr2;
		socklen_t size = sizeof(addr);
		int fd1 = EOF, fd2 = EOF;
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Server: Creating socket(PF_INET, SOCK_RAW, IPPROTO_RAW)");
		ShowPrompt(buffer);
		fd1 = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
		ShowResult(fd1);

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(0);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Binding RAW 127.0.0.1 IPPROTO_RAW");
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Client: Creating socket(PF_INET, SOCK_RAW, IPPROTO_RAW)");
		ShowPrompt(buffer);
		fd2 = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
		ShowResult(fd1);

		getsockname(fd1, (struct sockaddr *) &addr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Client: Connecting RAW 127.0.0.1 %d", ntohs(addr.sin_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));

		memset(&ip, 0, sizeof(ip));
		ip.version = 4;
		ip.ihl = sizeof(struct iphdr) / 4;
		ip.protocol = IPPROTO_RAW;
		ip.saddr = ip.daddr = htonl(INADDR_LOOPBACK);

		snprintf(buffer, sizeof(buffer) - 1, "Client: Sending RAW 127.0.0.1 %d", ntohs(addr.sin_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(sendto(fd2, &ip, sizeof(ip), 0, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(sendto(fd2, &ip, sizeof(ip), 0, (struct sockaddr *) &addr, sizeof(addr)));
		
		getsockname(fd2, (struct sockaddr *) &addr2, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Receiving RAW 127.0.0.1 %d", ntohs(addr2.sin_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult2(recvfrom(fd1, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &addr2, &size));

		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		sendto(fd2, &ip, sizeof(ip), 0, (struct sockaddr *) &addr, sizeof(addr));

		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Server: Receiving RAW 127.0.0.1 %d", ntohs(addr2.sin_port));
		ShowPrompt(buffer);
		ShowResult2(recvfrom(fd1, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &addr, &size));

		if (fd2 != EOF) close(fd2);
		if (fd1 != EOF) close(fd1);

	}
	
	if ((i = socket(PF_INET6, SOCK_STREAM, 0)) == EOF) return;
	close(i);

	{
		struct sockaddr_in6 addr, addr2;
		socklen_t size = sizeof(addr);
		int fd1 = EOF, fd2 = EOF, fd3 = EOF;
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Server: Creating socket(PF_INET6, SOCK_STREAM, 0)");
		ShowPrompt(buffer);
		fd1 = socket(PF_INET6, SOCK_STREAM, 0);
		ShowResult(fd1);
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_addr = in6addr_loopback;
		addr.sin6_port = htons(0);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Binding TCP 0:0:0:0:0:0:0:1 0");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));

		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		getsockname(fd1, (struct sockaddr *) &addr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Listening TCP 0:0:0:0:0:0:0:1 %d", ntohs(addr.sin6_port));
		ShowPrompt(buffer);
		ShowResult(listen(fd1, 5));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(listen(fd1, 5));
		
		snprintf(buffer, sizeof(buffer) - 1, "Client: Creating socket(PF_INET6, SOCK_STREAM, 0)");
		ShowPrompt(buffer);
		fd2 = socket(PF_INET6, SOCK_STREAM, 0);
		ShowResult(fd1);
		
		snprintf(buffer, sizeof(buffer) - 1, "Client: Connecting TCP 0:0:0:0:0:0:0:1 %d", ntohs(addr.sin6_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		
		getsockname(fd2, (struct sockaddr *) &addr2, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Accepting TCP 0:0:0:0:0:0:0:1 %d", ntohs(addr2.sin6_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult3(fd3 = accept(fd1, (struct sockaddr *) &addr2, &size));
		close(fd3); fd3 = EOF;
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		close(fd2); fd2 = socket(PF_INET6, SOCK_STREAM, 0);
		connect(fd2, (struct sockaddr *) &addr, sizeof(addr));
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		getsockname(fd2, (struct sockaddr *) &addr2, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Accepting TCP 0:0:0:0:0:0:0:1 %d", ntohs(addr2.sin6_port));
		ShowPrompt(buffer);
		ShowResult3(fd3 = accept(fd1, (struct sockaddr *) &addr2, &size));
		
		close(fd3); fd3 = EOF;
		close(fd2); fd2 = EOF;
		close(fd1); fd1 = EOF;
	}
	
	{
		struct sockaddr_in6 addr, addr2;
		socklen_t size = sizeof(addr);
		int fd1 = EOF, fd2 = EOF;
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Server: Creating socket(PF_INET6, SOCK_DGRAM, 0)");
		ShowPrompt(buffer);
		fd1 = socket(PF_INET6, SOCK_DGRAM, 0);
		ShowResult(fd1);

		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_addr = in6addr_loopback;
		addr.sin6_port = htons(0);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Binding UDP 0:0:0:0:0:0:0:1 0");
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(bind(fd1, (struct sockaddr *) &addr, sizeof(addr)));
		
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Client: Creating socket(PF_INET6, SOCK_DGRAM, 0)");
		ShowPrompt(buffer);
		fd2 = socket(PF_INET6, SOCK_DGRAM, 0);
		ShowResult(fd1);

		getsockname(fd1, (struct sockaddr *) &addr, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Client: Connecting UDP 0:0:0:0:0:0:0:1 %d", ntohs(addr.sin6_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(connect(fd2, (struct sockaddr *) &addr, sizeof(addr)));
		
		snprintf(buffer, sizeof(buffer) - 1, "Client: Sending UDP 0:0:0:0:0:0:0:1 %d", ntohs(addr.sin6_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult(sendto(fd2, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr)));
		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		ShowPrompt(buffer);
		ShowResult(sendto(fd2, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr)));
		
		getsockname(fd2, (struct sockaddr *) &addr2, &size);
		snprintf(buffer, sizeof(buffer) - 1, "Server: Receiving UDP 0:0:0:0:0:0:0:1 %d", ntohs(addr2.sin6_port));
		is_enforce = 1; WriteStatus("MAC_FOR_NETWORK=3\n");
		ShowPrompt(buffer);
		ShowResult2(recvfrom(fd1, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &addr2, &size));

		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		sendto(fd2, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr));

		is_enforce = 0; WriteStatus("MAC_FOR_NETWORK=2\n");
		snprintf(buffer, sizeof(buffer) - 1, "Server: Receiving UDP 0:0:0:0:0:0:0:1 %d", ntohs(addr2.sin6_port));
		ShowPrompt(buffer);
		ShowResult2(recvfrom(fd1, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &addr, &size));

		if (fd2 != EOF) close(fd2);
		if (fd1 != EOF) close(fd1);

	}

	/* Where can I find an example program that uses IPv6 raw socket? */

}

int main(int argc, char *argv[]) {
	Init();
	StageNetworkTest();
	ClearStatus();
	return 0;
}
