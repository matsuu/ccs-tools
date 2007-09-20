/*
 * setprofile.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0-rc   2007/09/20
 *
 */
#include "ccstools.h"

int setprofile_main(int argc, char *argv[]) {
	FILE *fp_in, *fp_out;
	int profile = 0;
	int recursive = 0;
	int i, start = 2;
	if (argc > 1 && strcmp(argv[1], "-r") == 0) {
		recursive = 1;
		start = 3;
	}
	if (argc <= start || sscanf(argv[start - 1], "%u", &profile) != 1) {
		fprintf(stderr, "%s [-r] profile domainname [domainname ...]\n", argv[0]);
		return 0;
	}
	for (i = start; i < argc; i++) NormalizeLine(argv[i]);
	{
		const int fd = open(proc_policy_domain_status, O_RDWR);
		if (fd == EOF) {
			fprintf(stderr, "You can't run this daemon for this kernel.\n");
			return 1;
		} else if (write(fd, "", 0) != 0) {
			fprintf(stderr, "You need to register this program to %s to run this program.\n", proc_policy_manager);
			return 1;
		}
		close(fd);
	}
	{
		int profile_found = 0;
		if ((fp_in = fopen(proc_policy_profile, "r")) == NULL) {
			fprintf(stderr, "Can't open policy file.\n");
			exit(1);
		}
		get();
		while (freadline(fp_in)) {
			if (atoi(shared_buffer) == profile) {
				profile_found = 1;
				break;
			}
		}
		put();
		fclose(fp_in);
		if (!profile_found) {
			fprintf(stderr, "Profile %u not defined.\n", profile);
			exit(1);
		}
	}
	if ((fp_in = fopen(proc_policy_domain_status, "r")) == NULL || (fp_out = fopen(proc_policy_domain_status, "w")) == NULL) {
		fprintf(stderr, "Can't open policy file.\n");
		exit(1);
	}
	get();
	while (freadline(fp_in)) {
		char *cp = strchr(shared_buffer, ' ');
		if (!cp) break;
		*cp++ = '\0';
		for (i = start; i < argc; i++) {
			const int len = strlen(argv[i]);
			if (strncmp(cp, argv[i], len)) continue;
			if (!recursive) {
				if (cp[len]) continue;
			} else {
				if (cp[len] && cp[len] != ' ') continue;
			}
			fprintf(fp_out, "%d %s\n", profile, cp);
			printf("%d %s\n", profile, cp);
		}
	}
	put();
	fclose(fp_in); fclose(fp_out);
	return 0;
}
