/*
 * findtemp.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.2   2008/06/25
 *
 */
#include "ccstools.h"

int findtemp_main(int argc, char *argv[]) {
	const char **pattern_list = NULL;
	int pattern_list_count = 0;
	int i;
	char buffer[16384], buffer2[sizeof(buffer)];
	if (argc > 1) {
		if (strcmp(argv[1], "--all")) {
			printf("%s < domain_policy\n\n", argv[0]);
			return 0;
		}
	}
	while (memset(buffer, 0, sizeof(buffer)), fscanf(stdin, "%16380s", buffer) == 1) {
		if (buffer[0] != '/') continue;
		{
			struct stat64 buf;
			if (!decode(buffer, buffer2)) continue;
			if (lstat64(buffer2, &buf) == 0) continue;
		}
		for (i = 0; i < pattern_list_count; i++) {
			if (strcmp(pattern_list[i], buffer) == 0) break;
		}
		if (i < pattern_list_count) continue;
		if ((pattern_list = (const char **) realloc(pattern_list, sizeof(const char *) * (pattern_list_count + 1))) == NULL ||
			(pattern_list[pattern_list_count++] = strdup(buffer)) == NULL) {
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}
	}
	qsort(pattern_list, pattern_list_count, sizeof(char *), string_compare);
	for (i = 0; i < pattern_list_count; i++) printf("%s\n", pattern_list[i]);
	return 0;
}
