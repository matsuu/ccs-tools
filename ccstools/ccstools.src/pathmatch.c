/*
 * pathmatch.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5   2008/11/11
 *
 */
#include "ccstools.h"

static bool print_path_needs_separator = false;

static void print_path(const char *dir, const char *file, const char *trailer)
{
	if (print_path_needs_separator)
		putchar(' ');
	print_path_needs_separator = true;
	fprintf_encoded(stdout, dir);
	fprintf_encoded(stdout, file);
	fprintf_encoded(stdout, trailer);
}

static char *scandir_filter_current_part = NULL;

static int scandir_filter(const struct dirent *buf)
{
	char buffer[1024];
	char c;
	char *dp = buffer;
	const char *cp = buf->d_name;
	if (buf->d_type == DT_DIR) {
		if (!strcmp(cp, ".") || !strcmp(cp, ".."))
			return 0;
	}
	/* Make sure strlen(cp) * 4 + 1 < sizeof(buffer). */
	if (strlen(cp) > 255)
		return 0;
	while (true) {
		c = *cp++;
		if (!c)
			break;
		if (c == '\\') {
			*dp++ = '\\';
			*dp++ = '\\';
		} else if (c > ' ' && c < 127) {
			*dp++ = c;
		} else {
			*dp++ = '\\';
			*dp++ = ((c >> 6) + '0');
			*dp++ = (((c >> 3) & 7) + '0');
			*dp++ = ((c & 7) + '0');
		}
	}
	*dp = '\0';
	/* printf("Compare: %s %s\n", buffer, scandir_filter_current_part); */
	if (file_matches_pattern(buffer, dp, scandir_filter_current_part,
				 strchr(scandir_filter_current_part, '\0')))
		return 1;
	return 0;
}

static bool scandir_target_is_dir = false;
static int scandir_target_depth = 0;
static char **scandir_target_part = NULL;

static void scan_dir(const char *path, int depth)
{
	struct dirent **namelist;
	int i;
	int n;
	scandir_filter_current_part = scandir_target_part[depth];
	/* printf("Scan: %d %s\n", depth, scandir_filter_current_part); */
	n = scandir(path, &namelist, scandir_filter, 0);
	if (n < 0)
		return;
	for (i = 0; i < n; i++) {
		const char *cp = namelist[i]->d_name;
		const unsigned char type = namelist[i]->d_type;
		if (depth < scandir_target_depth - 1) {
			if (type == DT_DIR) {
				const int len = strlen(path) + strlen(cp) + 4;
				char *child_path = malloc(len);
				if (!child_path)
					out_of_memory();
				snprintf(child_path, len - 1, "%s%s/",
					 path, cp);
				/* printf("Check: %s\n", child_path); */
				scan_dir(child_path, depth + 1);
				free(child_path);
			}
		} else if (scandir_target_is_dir) {
			if (type == DT_DIR)
				print_path(path, cp, "/");
		} else if (type != DT_DIR) {
			print_path(path, cp, "");
		}
		free((void *) namelist[i]);
	}
	free((void *) namelist);
}

static void do_pathmatch_main(char *target)
{
	if (!strcmp(target, "/")) {
		printf("/\n");
	} else if (target[0] != '/') {
		putchar('\n');
	} else {
		char *cp;
		int i;
		scandir_target_is_dir = (*(strchr(target, '\0') - 1) == '/');
		scandir_target_depth = 0;
		cp = target + 1;
		for (i = 1; ; i++) {
			char c = target[i];
			if (c != '/' && c != '\0')
				continue;
			target[i] = '\0';
			scandir_target_part = realloc(scandir_target_part,
						      (scandir_target_depth + 1)
						      * sizeof(char *));
			if (!scandir_target_part)
				out_of_memory();
			if (target + i != cp)
				scandir_target_part[scandir_target_depth++]
					= cp;
			cp = target + i + 1;
			if (!c)
				break;
		}
		/*
		for (i = 0; i < target_depth; i++)
			printf("%d %s\n", i, scandir_target_part[i]);
		*/
		print_path_needs_separator = false;
		scan_dir("/", 0);
		putchar('\n');
	}
}

int pathmatch_main(int argc, char *argv[])
{
	if (argc > 1) {
		int i;
		for (i = 1; i < argc; i++)
			do_pathmatch_main(argv[i]);
	} else {
		get();
		while (freadline(stdin))
			do_pathmatch_main(shared_buffer);
		put();
	}
	return 0;
}
