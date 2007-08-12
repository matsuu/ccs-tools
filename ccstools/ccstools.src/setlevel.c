/*
 * setlevel.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0-pre   2007/08/12
 *
 */
#include "ccstools.h"

int setlevel_main(int argc, char *argv[]) {
	static const char *policy_file = "/proc/ccs/status";
	int i, fd;
	char c;
	if (access("/proc/ccs/", F_OK)) {
		fprintf(stderr, "You can't use this command for this kernel.\n");
		return 1;
	}
	if ((fd = open(policy_file, O_RDWR)) == EOF) {
		fprintf(stderr, "Can't open %s\n", policy_file);
		return 1;
	} else if (write(fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to /proc/ccs/manager to run this program.\n");
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
