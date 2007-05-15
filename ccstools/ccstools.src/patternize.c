#include "ccstools.h"

/*
 * Check whether the given filename is patterened.
 * Returns nonzero if patterned, zero otherwise.
 */
static int PathContainsPattern(const char *filename) {
	if (filename) {
		char c, d, e;
		while ((c = *filename++) != '\0') {
			if (c != '\\') continue;
			switch (c = *filename++) {
			case '\\':  /* "\\" */
				continue;
			case '0':   /* "\ooo" */
			case '1':
			case '2':
			case '3':
				if ((d = *filename++) >= '0' && d <= '7' && (e = *filename++) >= '0' && e <= '7'
					&& (c != '0' || d != '0' || e != '0')) continue; /* pattern is not \000 */
			}
			return 1;
		}
	}
	return 0;
}

static int PathMatchesToPattern(const struct path_info *pathname0, const struct path_info *pattern0) {
	//if (!pathname || !pattern) return 0;
	const char *pathname = pathname0->name, *pattern = pattern0->name;
	const int len = pattern0->const_len;
	if (!pattern0->is_patterned) return !pathcmp(pathname0, pattern0);
	if (pathname0->depth != pattern0->depth) return 0;
	if (strncmp(pathname, pattern, len)) return 0;
	pathname += len; pattern += len;
	while (*pathname && *pattern) {
		const char *pathname_delimiter = strchr(pathname, '/'), *pattern_delimiter = strchr(pattern, '/');
		if (!pathname_delimiter) pathname_delimiter = strchr(pathname, '\0');
		if (!pattern_delimiter) pattern_delimiter = strchr(pattern, '\0');
		if (!FileMatchesToPattern(pathname, pathname_delimiter, pattern, pattern_delimiter)) return 0;
		pathname = *pathname_delimiter ? pathname_delimiter + 1 : pathname_delimiter;
		pattern = *pattern_delimiter ? pattern_delimiter + 1 : pattern_delimiter;
	}
	while (*pattern == '\\' && (*(pattern + 1) == '*' || *(pattern + 1) == '@')) pattern += 2;
	return (!*pathname && !*pattern);
}

int patternize_main(int argc, char *argv[]) {
	struct path_info *pattern_list = malloc(argc * sizeof(struct path_info));
	if (!pattern_list) OutOfMemory();
	int i;
	for (i = 0; i < argc; i++) {
		pattern_list[i].name = argv[i];
		fill_path_info(&pattern_list[i]);
	}
	get();
	while (freadline(stdin)) {
		char *sp = shared_buffer, *cp;
		int first = 1;
		int check_second = 0;
		int disabled = 0;
		while ((cp = strsep(&sp, " ")) != NULL) {
		check:
			if (first) {
				unsigned int perm;
				if (sscanf(cp, "%u", &perm) == 1 && (perm & 1) == 1) {
					/* Is this entry for a program? */
					check_second = 1;
				} else if (strcmp(cp, "<kernel>") == 0 || strcmp(cp, "use_profile") == 0
						   || strcmp(cp, "allow_capability") == 0 || strcmp(cp, "allow_signal") == 0 ||
						   strcmp(cp, "allow_network") == 0) {
					/* This entry is not pathname related permission. I don't convert. */
					disabled = 1;
				}
			} else if (disabled) {
				// Nothing to do.
			} else if (check_second) {
				check_second = 0;
				if (*cp == '/' && * (strchr(cp, '\0') - 1) != '/') { /* Don't convert @path_group . */
					/* This entry is for a program. I don't convert. */
					disabled = 1;
				}
				goto check;
			} else if (strcmp(cp, "if") == 0) {
				/* Don't convert after condition part. */
				disabled = 1;
			} else if (!PathContainsPattern(cp)) {
				int i;
				struct path_info cp2;
				cp2.name = cp;
				fill_path_info(&cp2);
				for (i = 1; i < argc; i++) {
					if (PathMatchesToPattern(&cp2, &pattern_list[i])) {
						cp = argv[i]; break;
					}
				}
			}
			if (!first) putchar(' ');
			first = 0;
			printf("%s", cp);
		}
		putchar('\n');
	}
	put();
	free(pattern_list);
	return 0;
}
