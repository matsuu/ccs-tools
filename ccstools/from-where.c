/*
 * from-where.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0-pre   2008/01/28
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	static char buffer[8192], buffer2[8192];
	pid_t pid;
	int i, j, k;
	unsigned int inode;
	const int debug = 0;
	struct address_entry {
		unsigned int inode;
		char *address;
	} *address_list = NULL;
	int address_list_len = 0;
	FILE *fp;
	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));
	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		printf("Usage: %s [exclude1 [exclude2 [...]]]\n\n", argv[0]);
		printf("This utility reports remote IP address and remote port number pair of TCP sockets "
		       "that are connected with this process.\n");
		printf("You can use exclude list for excluding actively opened TCP connections "
		       "because servers likely use constant address while clients unlikely do so.\n");
		printf("An exclude entry is in ip_address:port form (e.g. 127.0.0.1:22 ).\n");
		printf("You need to set SUID bit if you want to allow non-root users to use this utility.\n");
		return 0;
	}
	/* Get "inode" and "remotehost:remoteport" pairs. */
	fp = popen("netstat -nte", "r");
	if (!fp) {
		printf("Can't execute netstat\n");
		goto out;
	}
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		struct address_entry *tmp;
		if (sscanf(buffer, "tcp %*u %*u %*s %128s %*s %*u %u", buffer2, &inode) != 2) continue;
		tmp = (struct address_entry *) realloc(address_list, sizeof(struct address_entry) * (address_list_len + 1));
		if (!tmp) break;
		address_list = tmp;
		tmp[address_list_len].inode = inode;
		tmp[address_list_len].address = strdup(buffer2);
		if (!tmp[address_list_len].address) break;
		address_list_len++;
	}
	pclose(fp);
	/* Run process traversal starting from parent. */
	pid = getppid();
repeat:
	for (i = 0; i < 1024; i++) {
		snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/fd/%d", pid, i);
		if (readlink(buffer, buffer2, sizeof(buffer2) - 1) <= 0) continue;
		if (debug) printf("Reading %s\n", buffer);
		if (sscanf(buffer2, "socket:[%u]", &inode) != 1) continue;
		if (debug) printf("inode=%u\n", inode);
		for (j = 0; j < address_list_len; j++) {
			if (address_list[j].inode != inode) continue;
			/*
			 * Ignore some specific connections which are
			 * identified by constant "remotehost:remoteport" pair
			 *  (e.g. "127.0.0.1:5432" for connection to DMBS server).
			 */
			for (k = 1; k < argc; k++) {
				if (strcmp(address_list[j].address, argv[k]) == 0) break;
			}
			if (k == argc) printf("%s\n", address_list[j].address);
		}
	}
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/status", pid);
	/* Get parent process. */
	if ((fp = fopen(buffer, "r")) == NULL) goto out;
	if (debug) printf("Reading %s\n", buffer);
	while (fgets(buffer, sizeof(buffer) - 1, fp)) {
		if (sscanf(buffer, "PPid: %u", &pid) != 1) continue;
		fclose(fp);
		if (debug) printf("PPID=%u\n", pid);
		goto repeat;
	}
	fclose(fp);
out:
	/* No more parents exist or an error (e.g. ENOENT or EPERM) occurred. */
	if (debug) printf("Done\n");
	return 0;
}
