/*
 * setlevel.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0   2007/09/20
 *
 */
#include "ccstools.h"

int setlevel_main(int argc, char *argv[]) {
	const char *policy_file = proc_policy_profile;
	int i, fd;
	char c;
	if (access(proc_policy_dir, F_OK)) {
		fprintf(stderr, "You can't use this command for this kernel.\n");
		return 1;
	}
	if ((fd = open(policy_file, O_RDWR)) == EOF) {
		fprintf(stderr, "Can't open %s\n", policy_file);
		return 1;
	} else if (write(fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to %s to run this program.\n", proc_policy_manager);
		return 1;
	}
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			write(fd, argv[i], strlen(argv[i])); write(fd, "\n", 1);
		}
	}
	printf("<<< Access Control Status >>>\n");
	while (read(fd, &c, 1) == 1) putchar(c);
	close(fd);
	return 0;
}
