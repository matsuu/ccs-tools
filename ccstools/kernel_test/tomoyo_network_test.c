/*
 * tomoyo_network_test.c
 *
 * Testing program for fs/tomoyo_network.c
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5-pre   2008/10/20
 *
 */
#include "include.h"

static int is_enforce = 0;

static void ShowPrompt(const char *str)
{
	printf("Testing %50s: (%s) ", str,
	       is_enforce ? "must fail" : "should success");
	errno = 0;
}

static void ShowResult(int result)
{
	if (is_enforce) {
		if (result == EOF) {
			if (errno == EPERM)
				printf("OK: Permission denied.\n");
			else
				printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG!(%d)\n", result);
		}
	} else {
		if (result != EOF)
			printf("OK\n");
		else
			printf("%s\n", strerror(errno));
	}
}

static void ShowResult2(int result)
{
	if (is_enforce) {
		if (result == EOF) {
			if (errno == EAGAIN)
				printf("OK: Permission denied.\n");
			else
				printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG!(%d)\n", result);
		}
	} else {
		if (result != EOF)
			printf("OK\n");
		else
			printf("%s\n", strerror(errno));
	}
}

static void ShowResult3(int result)
{
	if (is_enforce) {
		if (result == EOF) {
			if (errno == ECONNABORTED)
				printf("OK: Permission denied.\n");
			else
				printf("FAILED: %s\n", strerror(errno));
		} else {
			printf("BUG!(%d)\n", result);
		}
	} else {
		if (result != EOF)
			printf("OK\n");
		else
			printf("%s\n", strerror(errno));
	}
}

static void ShowResult4(int result)
{
	if (result == EOF) {
		if (errno == EDESTADDRREQ)
			printf("OK: Destination address required.\n");
		else
			printf("BUG!: %s\n", strerror(errno));
	} else {
		printf("BUG!(%d)\n", result);
	}
}

static void SetEnforce(int flag)
{
	is_enforce = flag;
	if (flag)
		WriteStatus("MAC_FOR_NETWORK=3\n");
	else
		WriteStatus("MAC_FOR_NETWORK=2\n");
}

static void StageNetworkTest(void)
{
	int i;
	static char buf[16];
	static char sbuffer[1024];
	static char cbuffer[1024];
	memset(buf, 0, sizeof(buf));
	memset(sbuffer, 0, sizeof(sbuffer));
	memset(cbuffer, 0, sizeof(cbuffer));

	{ /* IPv4 TCP */
		struct sockaddr_in saddr;
		struct sockaddr_in caddr;
		socklen_t size = sizeof(saddr);
		int fd1 = EOF;
		int fd2 = EOF;
		int fd3 = EOF;

		fd1 = socket(PF_INET, SOCK_STREAM, 0);

		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		saddr.sin_port = htons(0);
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Binding TCP 127.0.0.1 0");
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		getsockname(fd1, (struct sockaddr *) &saddr, &size);

		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Listening TCP 127.0.0.1 %d",
			 ntohs(saddr.sin_port));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult(listen(fd1, 5));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult(listen(fd1, 5));

		fd2 = socket(PF_INET, SOCK_STREAM, 0);

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Connecting TCP 127.0.0.1 %d",
			 ntohs(saddr.sin_port));
		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));
		getsockname(fd2, (struct sockaddr *) &caddr, &size);

		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Accepting TCP 127.0.0.1 %d",
			 ntohs(caddr.sin_port));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		fd3 = accept(fd1, (struct sockaddr *) &caddr, &size);
		ShowResult3(fd3);

		SetEnforce(0);
		close(fd2);
		fd2 = socket(PF_INET, SOCK_STREAM, 0);
		connect(fd2, (struct sockaddr *) &saddr, sizeof(saddr));
		getsockname(fd2, (struct sockaddr *) &caddr, &size);

		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Accepting TCP 127.0.0.1 %d",
			 ntohs(caddr.sin_port));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		fd3 = accept(fd1, (struct sockaddr *) &caddr, &size);
		ShowResult3(fd3);

		close(fd3);
		close(fd2);
		close(fd1);
	}

	{ /* IPv4 UDP */
		struct sockaddr_in saddr;
		struct sockaddr_in caddr;
		socklen_t size = sizeof(saddr);
		int fd1 = EOF;
		int fd2 = EOF;

		fd1 = socket(PF_INET, SOCK_DGRAM, 0);

		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		saddr.sin_port = htons(0);
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Binding UDP 127.0.0.1 0");
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		getsockname(fd1, (struct sockaddr *) &saddr, &size);

		fd2 = socket(PF_INET, SOCK_DGRAM, 0);

		/* send() -> recv() */

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Sending UDP 127.0.0.1 %d using send()",
			 ntohs(saddr.sin_port));
		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult4(send(fd2, "", 1, 0));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult4(send(fd2, "", 1, 0));

		/* write() -> read() */

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Sending UDP 127.0.0.1 %d using write()",
			 ntohs(saddr.sin_port));
		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult4(write(fd2, "", 1));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult4(write(fd2, "", 1));

		/* connect() */

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Connecting UDP 127.0.0.1 %d",
			 ntohs(saddr.sin_port));
		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));
		getsockname(fd2, (struct sockaddr *) &caddr, &size);

		/* sendto() -> recvfrom() */

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Sending UDP 127.0.0.1 %d using sendto()",
			 ntohs(saddr.sin_port));
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Receiving UDP 127.0.0.1 %d using recvfrom()",
			 ntohs(caddr.sin_port));

		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult(sendto(fd2, "", 1, 0, (struct sockaddr *) &saddr,
				  sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(sendto(fd2, "", 1, 0, (struct sockaddr *) &saddr,
				  sizeof(saddr)));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult2(recvfrom(fd1, buf, sizeof(buf) - 1, 0,
				     (struct sockaddr *) &caddr, &size));
		SetEnforce(0);
		sendto(fd2, "", 1, 0, (struct sockaddr *) &saddr,
		       sizeof(saddr));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult2(recvfrom(fd1, buf, sizeof(buf) - 1, 0,
				     (struct sockaddr *) &caddr, &size));

		/* send() -> recv() */

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Sending UDP 127.0.0.1 %d using send()",
			 ntohs(saddr.sin_port));
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Receiving UDP 127.0.0.1 %d using recv()",
			 ntohs(caddr.sin_port));
		if (0) {
			SetEnforce(1);
			ShowPrompt(cbuffer);
			ShowResult(send(fd2, "", 1, 0));
			/* This won't fail because dest address is given via
			   connect(). */
		}
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(send(fd2, "", 1, 0));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult2(recv(fd1, buf, sizeof(buf) - 1, 0));
		SetEnforce(0);
		send(fd2, "", 1, 0);
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult2(recv(fd1, buf, sizeof(buf) - 1, 0));

		/* write() -> read() */

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Sending UDP 127.0.0.1 %d using write()",
			 ntohs(saddr.sin_port));
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Receiving UDP 127.0.0.1 %d using read()",
			 ntohs(caddr.sin_port));
		if (0) {
			SetEnforce(1);
			ShowPrompt(cbuffer);
			ShowResult(write(fd2, "", 1));
			/* This won't fail because dest address is given via
			   connect(). */
		}
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(write(fd2, "", 1));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult2(read(fd1, buf, sizeof(buf) - 1));
		SetEnforce(0);
		write(fd2, "", 1);
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult2(read(fd1, buf, sizeof(buf) - 1));

		/* sendmsg() -> recvmsg() */
		{
			struct msghdr msg1;
			struct msghdr msg2;
			struct iovec iov1 = { "", 1 };
			struct iovec iov2 = { buf, sizeof(buf) - 1 };
			memset(&msg1, 0, sizeof(msg1));
			memset(&msg2, 0, sizeof(msg2));
			msg1.msg_iov = &iov1;
			msg1.msg_iovlen = 1;
			msg1.msg_name = &saddr;
			msg1.msg_namelen = sizeof(saddr);
			msg2.msg_iov = &iov2;
			msg2.msg_iovlen = 1;
			/*
			  msg2.msg_name = &caddr;
			  msg2.msg_namelen = sizeof(caddr);
			*/
			snprintf(cbuffer, sizeof(cbuffer) - 1,
				 "Client: Sending UDP 127.0.0.1 %d using "
				 "sendmsg()", ntohs(saddr.sin_port));
			snprintf(sbuffer, sizeof(sbuffer) - 1,
				 "Server: Receiving UDP 127.0.0.1 %d using "
				 "recvmsg()", ntohs(caddr.sin_port));
			SetEnforce(1);
			ShowPrompt(cbuffer);
			ShowResult(sendmsg(fd1, &msg1, 0));
			SetEnforce(0);
			ShowPrompt(cbuffer);
			ShowResult(sendmsg(fd1, &msg1, 0));
			SetEnforce(1);
			ShowPrompt(sbuffer);
			ShowResult2(recvmsg(fd1, &msg2, 0));
			SetEnforce(0);
			sendmsg(fd1, &msg1, 0);
			SetEnforce(0);
			ShowPrompt(sbuffer);
			ShowResult2(recvmsg(fd1, &msg2, 0));
		}

		close(fd2);
		close(fd1);

	}

	{ /* IPv4 RAW */
		static struct iphdr ip;
		struct sockaddr_in saddr;
		struct sockaddr_in caddr;
		socklen_t size = sizeof(saddr);
		int fd1 = EOF;
		int fd2 = EOF;

		fd1 = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);

		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		saddr.sin_port = htons(0);
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Binding RAW 127.0.0.1 IPPROTO_RAW");
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));

		fd2 = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);

		getsockname(fd1, (struct sockaddr *) &saddr, &size);
		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Connecting RAW 127.0.0.1 %d",
			 ntohs(saddr.sin_port));
		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));

		memset(&ip, 0, sizeof(ip));
		ip.version = 4;
		ip.ihl = sizeof(struct iphdr) / 4;
		ip.protocol = IPPROTO_RAW;
		ip.daddr = htonl(INADDR_LOOPBACK);
		ip.saddr = ip.daddr;

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Sending RAW 127.0.0.1 %d",
			 ntohs(saddr.sin_port));
		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult(sendto(fd2, &ip, sizeof(ip), 0,
				  (struct sockaddr *) &saddr, sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(sendto(fd2, &ip, sizeof(ip), 0,
				  (struct sockaddr *) &saddr, sizeof(saddr)));

		getsockname(fd2, (struct sockaddr *) &caddr, &size);
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Receiving RAW 127.0.0.1 %d",
			 ntohs(caddr.sin_port));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult2(recvfrom(fd1, buf, sizeof(buf) - 1, 0,
				     (struct sockaddr *) &caddr, &size));
		SetEnforce(0);
		sendto(fd2, &ip, sizeof(ip), 0, (struct sockaddr *) &saddr,
		       sizeof(saddr));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult2(recvfrom(fd1, buf, sizeof(buf) - 1, 0,
				     (struct sockaddr *) &caddr, &size));

		close(fd2);
		close(fd1);

	}

	i = socket(PF_INET6, SOCK_STREAM, 0);
	if (i == EOF)
		return;
	close(i);

	{
		struct sockaddr_in6 saddr;
		struct sockaddr_in6 caddr;
		socklen_t size = sizeof(saddr);
		int fd1 = EOF;
		int fd2 = EOF;
		int fd3 = EOF;

		fd1 = socket(PF_INET6, SOCK_STREAM, 0);

		memset(&saddr, 0, sizeof(saddr));
		saddr.sin6_family = AF_INET6;
		saddr.sin6_addr = in6addr_loopback;
		saddr.sin6_port = htons(0);
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Binding TCP 0:0:0:0:0:0:0:1 0");
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		getsockname(fd1, (struct sockaddr *) &saddr, &size);

		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Listening TCP 0:0:0:0:0:0:0:1 %d",
			 ntohs(saddr.sin6_port));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult(listen(fd1, 5));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult(listen(fd1, 5));

		fd2 = socket(PF_INET6, SOCK_STREAM, 0);

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Connecting TCP 0:0:0:0:0:0:0:1 %d",
			 ntohs(saddr.sin6_port));
		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));

		getsockname(fd2, (struct sockaddr *) &caddr, &size);
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Accepting TCP 0:0:0:0:0:0:0:1 %d",
			 ntohs(caddr.sin6_port));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		fd3 = accept(fd1, (struct sockaddr *) &caddr, &size);
		ShowResult3(fd3);

		SetEnforce(0);
		close(fd2);
		fd2 = socket(PF_INET6, SOCK_STREAM, 0);
		connect(fd2, (struct sockaddr *) &saddr, sizeof(saddr));
		getsockname(fd2, (struct sockaddr *) &caddr, &size);

		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Accepting TCP 0:0:0:0:0:0:0:1 %d",
			 ntohs(caddr.sin6_port));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		fd3 = accept(fd1, (struct sockaddr *) &caddr, &size);
		ShowResult3(fd3);

		close(fd3);
		close(fd2);
		close(fd1);
	}

	{
		struct sockaddr_in6 saddr;
		struct sockaddr_in6 caddr;
		socklen_t size = sizeof(saddr);
		int fd1 = EOF;
		int fd2 = EOF;

		fd1 = socket(PF_INET6, SOCK_DGRAM, 0);

		memset(&saddr, 0, sizeof(saddr));
		saddr.sin6_family = AF_INET6;
		saddr.sin6_addr = in6addr_loopback;
		saddr.sin6_port = htons(0);
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Binding UDP 0:0:0:0:0:0:0:1 0");
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult(bind(fd1, (struct sockaddr *) &saddr,
				sizeof(saddr)));
		getsockname(fd1, (struct sockaddr *) &saddr, &size);

		fd2 = socket(PF_INET6, SOCK_DGRAM, 0);

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Connecting UDP 0:0:0:0:0:0:0:1 %d",
			 ntohs(saddr.sin6_port));
		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(connect(fd2, (struct sockaddr *) &saddr,
				   sizeof(saddr)));
		getsockname(fd2, (struct sockaddr *) &caddr, &size);

		snprintf(cbuffer, sizeof(cbuffer) - 1,
			 "Client: Sending UDP 0:0:0:0:0:0:0:1 %d",
			 ntohs(saddr.sin6_port));
		snprintf(sbuffer, sizeof(sbuffer) - 1,
			 "Server: Receiving UDP 0:0:0:0:0:0:0:1 %d",
			 ntohs(caddr.sin6_port));

		SetEnforce(1);
		ShowPrompt(cbuffer);
		ShowResult(sendto(fd2, "", 1, 0, (struct sockaddr *) &saddr,
				  sizeof(saddr)));
		SetEnforce(0);
		ShowPrompt(cbuffer);
		ShowResult(sendto(fd2, "", 1, 0, (struct sockaddr *) &saddr,
				  sizeof(saddr)));
		SetEnforce(1);
		ShowPrompt(sbuffer);
		ShowResult2(recvfrom(fd1, buf, sizeof(buf) - 1, 0,
				     (struct sockaddr *) &caddr, &size));
		SetEnforce(0);
		sendto(fd2, "", 1, 0, (struct sockaddr *) &saddr,
		       sizeof(saddr));
		SetEnforce(0);
		ShowPrompt(sbuffer);
		ShowResult2(recvfrom(fd1, buf, sizeof(buf) - 1, 0,
				     (struct sockaddr *) &caddr, &size));

		close(fd2);
		close(fd1);

	}

	/* Where can I find an example program that uses IPv6 raw socket? */

}

int main(int argc, char *argv[])
{
	Init();
	StageNetworkTest();
	ClearStatus();
	return 0;
}
