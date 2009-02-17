/*
 * setprofile.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-pre   2009/02/17
 *
 */
#include "ccstools.h"

int setprofile_main(int argc, char *argv[])
{
	FILE *fp_in;
	FILE *fp_out;
	unsigned int profile = 0;
	_Bool recursive = false;
	int i;
	int start = 2;
	if (argc > 1 && !strcmp(argv[1], "-r")) {
		recursive = true;
		start = 3;
	}
	if (argc <= start || sscanf(argv[start - 1], "%u", &profile) != 1) {
		fprintf(stderr, "%s [-r] profile domainname [domainname ...]\n",
			argv[0]);
		return 0;
	}
	for (i = start; i < argc; i++)
		normalize_line(argv[i]);
	{
		const int fd = open(proc_policy_domain_status, O_RDWR);
		if (fd == EOF) {
			fprintf(stderr, "You can't run this command for this "
				"kernel.\n");
			return 1;
		} else if (write(fd, "", 0) != 0) {
			fprintf(stderr, "You need to register this program to "
				"%s to run this program.\n",
				proc_policy_manager);
			return 1;
		}
		close(fd);
	}
	{
		_Bool profile_found = false;
		FILE *fp = fopen(proc_policy_profile, "r");
		if (!fp) {
			fprintf(stderr, "Can't open policy file.\n");
			exit(1);
		}
		get();
		while (freadline(fp)) {
			if (atoi(shared_buffer) != profile)
				continue;
			profile_found = true;
			break;
		}
		put();
		fclose(fp);
		if (!profile_found) {
			fprintf(stderr, "Profile %u not defined.\n", profile);
			exit(1);
		}
	}
	fp_in = fopen(proc_policy_domain_status, "r");
	fp_out = fopen(proc_policy_domain_status, "w");
	if (!fp_in || !fp_out) {
		fprintf(stderr, "Can't open policy file.\n");
		exit(1);
	}
	get();
	while (freadline(fp_in)) {
		char *cp = strchr(shared_buffer, ' ');
		if (!cp)
			break;
		*cp++ = '\0';
		for (i = start; i < argc; i++) {
			const int len = strlen(argv[i]);
			if (strncmp(cp, argv[i], len))
				continue;
			if (!recursive) {
				if (cp[len])
					continue;
			} else {
				if (cp[len] && cp[len] != ' ')
					continue;
			}
			fprintf(fp_out, "%u %s\n", profile, cp);
			printf("%u %s\n", profile, cp);
		}
	}
	put();
	fclose(fp_in);
	fclose(fp_out);
	return 0;
}
