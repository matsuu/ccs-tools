/*
 * findtemp.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-pre   2009/02/24
 *
 */
#include "ccstools.h"

int findtemp_main(int argc, char *argv[])
{
	const char **pattern_list = NULL;
	int pattern_list_count = 0;
	int i;
	char buffer[16384];
	char buffer2[sizeof(buffer)];
	if (argc > 1) {
		if (strcmp(argv[1], "--all")) {
			printf("%s < domain_policy\n\n", argv[0]);
			return 0;
		}
	}
	while (memset(buffer, 0, sizeof(buffer)),
	       fscanf(stdin, "%16380s", buffer) == 1) {
		const char *cp;
		if (buffer[0] != '/')
			continue;
		{
			struct stat64 buf;
			if (!decode(buffer, buffer2))
				continue;
			if (!lstat64(buffer2, &buf))
				continue;
		}
		for (i = 0; i < pattern_list_count; i++) {
			if (!strcmp(pattern_list[i], buffer))
				break;
		}
		if (i < pattern_list_count)
			continue;
		pattern_list = realloc(pattern_list, sizeof(const char *) *
				       (pattern_list_count + 1));
		if (!pattern_list)
			out_of_memory();
		cp = strdup(buffer);
		if (!cp)
			out_of_memory();
		pattern_list[pattern_list_count++] = cp;
	}
	qsort(pattern_list, pattern_list_count, sizeof(const char *),
	      string_compare);
	for (i = 0; i < pattern_list_count; i++)
		printf("%s\n", pattern_list[i]);
	for (i = 0; i < pattern_list_count; i++)
		free((void *) pattern_list[i]);
	free(pattern_list);
	return 0;
}
