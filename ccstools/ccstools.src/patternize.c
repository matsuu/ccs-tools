/*
 * patternize.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-rc   2009/03/04
 *
 */
#include "ccstools.h"

/*
 * Check whether the given filename is patterened.
 * Returns nonzero if patterned, zero otherwise.
 */
static _Bool path_contains_pattern(const char *filename)
{
	if (filename) {
		char c;
		char d;
		char e;
		while (true) {
			c = *filename++;
			if (!c)
				break;
			if (c != '\\')
				continue;
			c = *filename++;
			switch (c) {
			case '\\':  /* "\\" */
				continue;
			case '0':   /* "\ooo" */
			case '1':
			case '2':
			case '3':
				d = *filename++;
				if (d < '0' || d > '7')
					break;
				e = *filename++;
				if (e < '0' || e > '7')
					break;
				if (c != '0' || d != '0' || e != '0')
					continue; /* pattern is not \000 */
			}
			return true;
		}
	}
	return false;
}

static const char *patternize(const char *cp, int argc, char *argv[],
			      struct path_info *pattern_list)
{
	int i;
	struct path_info cp2;
	cp2.name = cp;
	fill_path_info(&cp2);
	for (i = 1; i < argc; i++) {
		if (path_matches_pattern(&cp2, &pattern_list[i]))
			return argv[i];
	}
	return cp;
}

int patternize_main(int argc, char *argv[])
{
	int i;
	struct path_info *pattern_list
		= malloc(argc * sizeof(struct path_info));
	if (!pattern_list)
		out_of_memory();
	for (i = 0; i < argc; i++) {
		pattern_list[i].name = argv[i];
		fill_path_info(&pattern_list[i]);
	}
	get();
	while (freadline(stdin)) {
		char *sp = shared_buffer;
		const char *cp;
		_Bool first = true;
		u8 count = 0;
		while (true) {
			cp = strsep(&sp, " ");
			if (!cp)
				break;
			if (first) {
				if (!strcmp(cp, "allow_read") ||
				    !strcmp(cp, "allow_write") ||
				    !strcmp(cp, "allow_read/write") ||
				    !strcmp(cp, "allow_create") ||
				    !strcmp(cp, "allow_unlink") ||
				    !strcmp(cp, "allow_mkdir") ||
				    !strcmp(cp, "allow_rmdir") ||
				    !strcmp(cp, "allow_mkfifo") ||
				    !strcmp(cp, "allow_mksock") ||
				    !strcmp(cp, "allow_mkblock") ||
				    !strcmp(cp, "allow_mkchar") ||
				    !strcmp(cp, "allow_truncate") ||
				    !strcmp(cp, "allow_symlink") ||
				    !strcmp(cp, "allow_rewrite") ||
				    !strcmp(cp, "2") || !strcmp(cp, "4") ||
				    !strcmp(cp, "6"))
					count = 1;
				else if (!strcmp(cp, "allow_link") ||
					 !strcmp(cp, "allow_rename"))
					count = 2;
				else
					count = 0;
			} else if (count && count-- && *cp != '@' &&
				   !path_contains_pattern(cp)) {
				cp = patternize(cp, argc, argv, pattern_list);
			}
			if (!first)
				putchar(' ');
			first = false;
			printf("%s", cp);
		}
		putchar('\n');
	}
	put();
	free(pattern_list);
	return 0;
}
