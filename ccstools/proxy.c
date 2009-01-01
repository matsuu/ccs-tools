/*
 * proxy.c
 *
 * Binds to local port explicitly before forwarding TCP connections.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.5   2008/11/11
 *
 * This tool is intended to limit local port numbers that clients
 * will use when connecting to servers, so that servers can enforce
 * client's port number based access control.
 *
 * To compile this program run the following command
 *
 *   gcc -Wall -O3 -o path_to_output proxy.c
 *   chown 0:0 path_to_output
 *
 * where the path_to_output is the location you want to place the binary.
 *
 * To use this program for ssh, create a file named "useproxy"
 * containing a line
 *
 *   ProxyCommand path_to_output %h %p min_port max_port
 *
 * where the min_port and max_port are the local port range
 * the sshd will accept.
 *
 * Use -F option like
 *
 *   ssh -F useproxy example.com
 *
 * when you run ssh.
 *
 * You may append to ~/.ssh/config or /etc/ssh/ssh_config
 * if you want to use this tool by default.
 *
 * You need to turn SUID bit on (or give CAP_NET_BIND_SERVICE capability) like
 *
 *   chmod 4755 path_to_output
 *
 * if you want to allow non root user to use local port less than 1024.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>

static in_addr_t get_host_by_name_alias(const char *strHostName)
{
	in_addr_t IP;
	IP = inet_addr(strHostName);
	if (IP == INADDR_NONE) {
		struct hostent *hp = gethostbyname(strHostName);
		if (hp)
			IP = *(in_addr_t *) hp->h_addr_list[0];
	}
	return IP;
}

int main(int argc, char *argv[])
{
	const int remote = socket(PF_INET, SOCK_STREAM, 0);
	unsigned int port;
	struct sockaddr_in addr;
	in_addr_t forward_connect_ip = INADDR_NONE;
	unsigned short int forward_connect_port = 0;
	unsigned short int forward_bind_port_min = 0;
	unsigned short int forward_bind_port_max = 0;
	if (argc != 5) {
		fprintf(stderr, "Usage: %s forward_connect_host "
			"forward_connect_port forward_bind_port_min "
			"forward_bind_port_max\n", argv[0]);
		return 1;
	}
	forward_bind_port_min = atoi(argv[3]);
	forward_bind_port_max = atoi(argv[4]);
	for (port = forward_bind_port_min; port <= forward_bind_port_max;
	     port++) {
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);
		if (!bind(remote, (struct sockaddr *) &addr, sizeof(addr)))
			break;
	}
	if (port > forward_bind_port_max) {
		fprintf(stderr, "ERROR: No local ports available.\n");
		return 1;
	}
	{ /* Drop root privileges. */
		const gid_t gid = -1;
		setgroups(1, &gid);
		setgid(-1);
		setuid(-1);
	}
	forward_connect_ip = ntohl(get_host_by_name_alias(argv[1]));
	forward_connect_port = atoi(argv[2]);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(forward_connect_ip);
	addr.sin_port = htons(forward_connect_port);
	if (connect(remote, (struct sockaddr *) &addr, sizeof(addr)) ||
	    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK) ||
	    fcntl(1, F_SETFL, fcntl(1, F_GETFL) | O_NONBLOCK) ||
	    fcntl(remote, F_SETFL, fcntl(remote, F_GETFL) | O_NONBLOCK)) {
		fprintf(stderr, "ERROR: Connecting to %u.%u.%u.%u : %s\n",
			(unsigned char) (forward_connect_ip >> 24),
			(unsigned char) (forward_connect_ip >> 16),
			(unsigned char) (forward_connect_ip >> 8),
			(unsigned char) forward_connect_ip, strerror(errno));
		return 1;
	}
	while (1) {
		fd_set rfds;
		fd_set wfds;
		static char local_buf[4096];
		static char remote_buf[4096];
		static int local_len = 0;
		static int remote_len = 0;
		static int local_eof = 0;
		static int remote_eof = 0;
		int len;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		if (local_eof == 0 && local_len < sizeof(local_buf))
			FD_SET(0, &rfds);
		if (remote_eof == 0 && remote_len < sizeof(remote_buf))
			FD_SET(remote, &rfds);
		if (local_len)
			FD_SET(remote, &wfds);
		if (remote_len)
			FD_SET(1, &wfds);
		select(remote + 1, &rfds, &wfds, NULL, NULL);
		if (FD_ISSET(0, &rfds)) {
			len = read(0, local_buf + local_len,
				   sizeof(local_buf) - local_len);
			if (len > 0)
				local_len += len;
			else if (len == 0)
				local_eof = 1;
		}
		if (FD_ISSET(remote, &rfds)) {
			len = read(remote, remote_buf + remote_len,
				   sizeof(remote_buf) - remote_len);
			if (len > 0)
				remote_len += len;
			else if (len == 0)
				remote_eof = 1;
		}
		if (FD_ISSET(remote, &wfds)) {
			len = write(remote, local_buf, local_len);
			if (len > 0) {
				local_len -= len;
				memmove(local_buf, local_buf + len, local_len);
			}
		}
		if (FD_ISSET(1, &wfds)) {
			len = write(1, remote_buf, remote_len);
			if (len > 0) {
				remote_len -= len;
				memmove(remote_buf, remote_buf + len,
					remote_len);
			}
		}
		if (local_len == 0 && local_eof == 1) {
			shutdown(remote, SHUT_WR);
			local_eof = 2;
		}
		if (remote_len == 0 && remote_eof == 1) {
			shutdown(1, SHUT_WR);
			remote_eof = 2;
		}
		if (local_eof == 2 && remote_eof == 2)
			break;
	}
	return 0;
}
