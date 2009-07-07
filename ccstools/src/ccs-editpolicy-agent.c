#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

static void show_tasklist(FILE *fp, const _Bool show_all)
{
	int status_fd = open(".process_status", O_RDWR);
	DIR *dir = opendir("/proc/");
	if (status_fd == EOF || !dir) {
		if (status_fd != EOF)
			close(status_fd);
		if (dir)
			closedir(dir);
		return;
	}
	fputc(0, fp);
	while (1) {
		FILE *status_fp;
		pid_t ppid = 1;
		char *name = NULL;
		char buffer[1024];
		char test[16];
		unsigned int pid;
		struct dirent *dent = readdir(dir);
		const char *cp;
		if (!dent)
			break;
		cp = dent->d_name;
		if (dent->d_type != DT_DIR || sscanf(cp, "%u", &pid) != 1)
			continue;
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/exe", pid);
		if (!show_all && readlink(buffer, test, sizeof(test)) <= 0)
			continue;
		snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/status", pid);
		status_fp = fopen(buffer, "r");
		if (status_fp) {
			while (memset(buffer, 0, sizeof(buffer)),
			       fgets(buffer, sizeof(buffer) - 1, status_fp)) {
				if (!strncmp(buffer, "Name:\t", 6)) {
					char *cp = buffer + 6;
					memmove(buffer, cp, strlen(cp) + 1);
					cp = strchr(buffer, '\n');
					if (cp)
						*cp = '\0';
					name = strdup(buffer);
				}
				if (sscanf(buffer, "PPid: %u", &ppid) == 1)
					break;
			}
			fclose(status_fp);
		}
		fprintf(fp, "PID=%u PPID=%u NAME=", pid, ppid);
		if (name) {
			cp = name;
			while (1) {
				unsigned char c = *cp++;
				if (!c)
					break;
				if (c == '\\') {
					c = *cp++;
					if (c == '\\')
						fprintf(fp, "\\\\");
					else if (c == 'n')
						fprintf(fp, "\\012");
					else
						break;
				} else if (c > ' ' && c <= 126) {
					fputc(c, fp);
				} else {
					fprintf(fp, "\\%c%c%c",
						(c >> 6) + '0',
						((c >> 3) & 7) + '0',
						(c & 7) + '0');
				}
			}
			free(name);
		} else {
			fprintf(fp, "<UNKNOWN>");
		}
		fputc('\n', fp);
		snprintf(buffer, sizeof(buffer) - 1, "%u\n", pid);
		write(status_fd, buffer, strlen(buffer));
		memset(buffer, 0, sizeof(buffer));
		while (1) {
			int len = read(status_fd, buffer, sizeof(buffer));
			if (len <= 0)
				break;
			fwrite(buffer, len, 1, fp);
		}
		fputc('\n', fp);
	}
	fputc(0, fp);
	closedir(dir);
	close(status_fd);
	fflush(fp);
}

static _Bool verbose = 0;

static void do_child(const int client)
{
	int i;
	int fd = EOF;
	char buffer[1024];
	/* Read filename. */
	for (i = 0; i < sizeof(buffer) - 1; i++) {
		if (read(client, buffer + i, 1) != 1)
			goto out;
		if (!buffer[i]) {
			char *cp = strrchr(buffer, '/');
			const _Bool ps = !strcmp(buffer,
						 "proc:process_status");
			const _Bool ps_all = !strcmp(buffer,
						     "proc:all_process_status");
			if (ps || ps_all) {
				FILE *fp = fdopen(client, "w");
				/* Open /proc/\$/ for reading. */
				if (fp) {
					show_tasklist(fp, ps_all);
					fclose(fp);
				}
				break;
			}
			if (!cp)
				cp = buffer;
			else
				cp++;
			/* Open for read/write. */
			fd = open(cp, O_RDWR);
			break;
		}
	}
	if (fd == EOF)
		goto out;
	/* Return \0 to indicate success. */
	if (write(client, "", 1) != 1)
		goto out;
	if (verbose) {
		write(2, "opened ", 7);
		write(2, buffer, strlen(buffer));
		write(2, "\n", 1);
	}
	while (1) {
		char c;
		/* Read a byte. */
		if (read(client, &c, 1) != 1)
			goto out;
		if (c) {
			/* Write that byte. */
			if (write(fd, &c, 1) != 1)
				goto out;
			if (verbose)
				write(1, &c, 1);
			continue;
		}
		/* Read until EOF. */
		while (1) {
			int len = read(fd, buffer, sizeof(buffer));
			if (len == 0)
				break;
			/* Don't send \0 because it is EOF marker. */
			if (len < 0 || memchr(buffer, '\0', len) ||
			    write(client, buffer, len) != len)
				goto out;
		}
		/* Return \0 to indicate EOF. */
		if (write(client, "", 1) != 1)
			goto out;
	}
 out:
	if (verbose)
		write(2, "disconnected\n", 13);
	close(fd);
	close(client);
}

int main(int argc, char *argv[])
{
	const int listener = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	socklen_t size = sizeof(addr);
	char *port;
	if (chdir("/proc/ccs/") && chdir("/sys/kernel/security/tomoyo/"))
		return 1;
	{
		int i;
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i], "--verbose"))
				continue;
			verbose = 1;
			argc--;
			for (; i < argc; i++)
				argv[i] = argv[i + 1];
			break;
		}
	}
	if (argc != 2) {
usage:
		fprintf(stderr, "%s listen_address:listen_port\n", argv[0]);
		return 1;
	}
	port = strchr(argv[1], ':');
	if (!port)
		goto usage;
	*port++ = '\0';
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_port = htons(atoi(port));
	if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) ||
	    listen(listener, 5) ||
	    getsockname(listener, (struct sockaddr *) &addr, &size)) {
		close(listener);
		return 1;
	}
	{
		const unsigned int ip = ntohl(addr.sin_addr.s_addr);
		printf("Listening at %u.%u.%u.%u:%u\n",
		       (unsigned char) (ip >> 24), (unsigned char) (ip >> 16),
		       (unsigned char) (ip >> 8), (unsigned char) ip,
		       ntohs(addr.sin_port));
		fflush(stdout);
	}
	close(0);
	if (!verbose) {
		close(1);
		close(2);
	}
	signal(SIGCHLD, SIG_IGN);
	while (1) {
		socklen_t size = sizeof(addr);
		const int client = accept(listener, (struct sockaddr *) &addr,
					  &size);
		if (client == EOF) {
			if (verbose)
				fprintf(stderr, "accept() failed\n");
			continue;
		}
		switch (fork()) {
		case 0:
			close(listener);
			do_child(client);
			_exit(0);
		case -1:
			if (verbose)
				fprintf(stderr, "fork() failed\n");
			close(client);
			break;
		default:
			close(client);
		}
	}
	close(listener);
	return 1;
}
