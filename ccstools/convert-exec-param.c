/*
 * convert-exec-param.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.2-pre   2008/06/08
 *
 */
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
	char buffer[2][16384];
	int line = 0;
	memset(buffer, 0, sizeof(buffer));
	while (1) {
		int i;
		char *cp1, *cp2;
		
		// Find header line.
		if (!fgets(buffer[0], sizeof(buffer[0]) - 1, stdin)) break;
		line++;
		if (!strchr(buffer[0], '\n')) goto out;
		if (buffer[0][0] != '#') continue;
		
		// Check for " argc=" part.
		cp1 = strstr(buffer[0], " argc=");
		if (!cp1) continue;
		
		// Get argc value.
		if (sscanf(cp1 + 1, "argc=%d", &argc) != 1) goto out;
		cp1 = strstr(buffer[0], " argv[]={ ");
		if (!cp1) goto out;
		cp2 = strstr(cp1 + 10, " } ");
		if (!cp2) goto out;
		
		// Check for " ... " part.
		if (!strncmp(cp2 - 4, " ... ", 5)) {
			fprintf(stderr, "%d: Too long header. Ignored.\n", line);
			continue;
		}
		*cp2 = '\0';
		memmove(buffer[0], cp1 + 10,  strlen(cp1 + 10) + 1);
		
		// Get domainname.
		line++;
		if (!fgets(buffer[1], sizeof(buffer[1]) - 1, stdin) || !strchr(buffer[1], '\n')) goto out;
		printf("select %s", buffer[1]);
		
		// Get "allow_execute " line.
		line++;
		if (!fgets(buffer[1], sizeof(buffer[1]) - 1, stdin) ||
		    strncmp(buffer[1], "allow_execute ", 14) ||
		    (cp1 = strchr(buffer[1], '\n')) == NULL) goto out;
		*cp1 = '\0';
		printf("%s if exec.argc=%d", buffer[1], argc);
		i = 0;
		cp1 = strtok(buffer[0], " ");
		while (cp1) {
			printf(" exec.argv[%d]=%s", i++, cp1);
			cp1 = strtok(NULL, " ");
		}
		printf("\ndelete %s\n\n", buffer[1]);
	}
	if (!line) {
		fprintf(stderr, "Usage: %s < /proc/ccs/grant_log or /proc/ccs/reject_log\n", argv[0]);
	}
	return 0;
 out:
	fprintf(stderr, "%d: Broken log entry. Aborted.\n", line);
	return 1;
}
