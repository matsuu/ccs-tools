/*
 * patternize.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.4+   2008/09/08
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
			return 1;
		}
	}
	return 0;
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
		bool first = true;
		bool disabled = false;
		while (true) {
			cp = strsep(&sp, " ");
			if (!cp)
				break;
			if (first) {
				if (!strcmp(cp, "allow_execute") ||
				    !strcmp(cp, "1") || !strcmp(cp, "3") ||
				    !strcmp(cp, "5") || !strcmp(cp, "7")) {
					/* This entry is an execute permission.
					   I don't convert. */
					disabled = true;
				} else if (!strcmp(cp, "<kernel>") ||
					   !strcmp(cp, "use_profile") ||
					   !strcmp(cp, "allow_capability") ||
					   !strcmp(cp, "allow_signal") ||
					   !strcmp(cp, "allow_network")) {
					/* This entry is not pathname related
					   permission. I don't convert. */
					disabled = true;
				}
			} else if (disabled) {
				/* Nothing to do. */
			} else if (!strcmp(cp, "if")) {
				/* Don't convert after condition part. */
				disabled = true;
			} else if (!path_contains_pattern(cp)) {
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
