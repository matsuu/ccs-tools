/*
 * tomoyo_accept_test.c
 *
 * Testing program for fs/tomoyo_file.c
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0-pre   2007/08/13
 *
 */
#include "include.h"

static FILE *fp_domain = NULL, *fp_exception = NULL, *fp_level = NULL;

static void SetLevel(const int i) {
	fprintf(fp_level, "255-MAC_FOR_FILE=%d\n", i); fflush(fp_level);
}

int main(int argc, char *argv[]) {
	static char self_domain[4096];
	static char buffer[1024];
	static const int rw_flags[4] = { 0, O_RDONLY, O_WRONLY, O_RDWR };
	static const int create_flags[3] = { 0, O_CREAT /* nonexistent*/ , O_CREAT /* existent */ };
	static const int truncate_flags[2] = { 0, O_TRUNC };
	static const int append_flags[2] = { 0, O_APPEND };
	memset(buffer, 0, sizeof(buffer));
	PreInit();
	if ((fp_level = fopen(proc_policy_profile, "w")) == NULL) {
		fprintf(stderr, "Can't open %s\n", proc_policy_profile);
		exit(1);
	}
	fprintf(fp_level, "255-COMMENT=Test\n255-TOMOYO_VERBOSE=0\n255-MAC_FOR_FILE=0\n255-MAX_ACCEPT_ENTRY=2048\n");
	fflush(fp_level);
	if ((fp_domain = fopen(proc_policy_domain_policy, "w")) == NULL) {
		fprintf(stderr, "Can't open %s\n", proc_policy_domain_policy);
		exit(1);
	}
	if ((fp_exception = fopen(proc_policy_exception_policy, "w")) == NULL) {
		fprintf(stderr, "Can't open %s\n", proc_policy_exception_policy);
		exit(1);
	}
	{
		FILE *fp = fopen(proc_policy_self_domain, "r");
		if (!fp) {
			fprintf(stderr, "Can't open %s\n", proc_policy_self_domain);
			exit(1);
		}
		memset(self_domain, 0, sizeof(self_domain));
		fgets(self_domain, sizeof(self_domain) - 1, fp);
		fclose(fp);
	}
	{
		FILE *fp = fopen(proc_policy_domain_status, "w");
		if (!fp) {
			fprintf(stderr, "Can't open %s\n", proc_policy_domain_status);
			exit(1);
		}
		fprintf(fp, "255 %s\n", self_domain);
		fclose(fp);
	}
	fprintf(fp_domain, "%s\n", self_domain);
	
	{
		int append_loop;
		for (append_loop = 0; append_loop < 2; append_loop++) {
			int truncate_loop;
			for (truncate_loop = 0; truncate_loop < 2; truncate_loop++) {
				int create_loop;
				for (create_loop = 0; create_loop < 3; create_loop++) {
					int rw_loop;
					for (rw_loop = 0; rw_loop < 4; rw_loop++) {
						int level, flags, i, fd;
						snprintf(buffer, sizeof(buffer) - 1, "/tmp/file:a=%d:t=%d:c=%d:m=%d", append_loop, truncate_loop, create_loop, rw_loop);
						fprintf(fp_exception, "deny_rewrite %s\n", buffer); fflush(fp_exception);
						flags = rw_flags[rw_loop] | truncate_flags[truncate_loop] | append_flags[append_loop] | create_flags[create_loop];
						for (i = 1; i < 8; i++) fprintf(fp_domain, "delete %d %s\n", i, buffer);
						fflush(fp_domain);
						for (level = 0; level < 4; level++) {
							SetLevel(0);
							if (create_loop == 1) {
								unlink(buffer);
							} else {
								close(open(buffer, O_CREAT, 0666));
							}
							SetLevel(level);
							if ((fd = open(buffer, flags, 0666)) != EOF) close(fd);
							else fprintf(stderr, "%d: open(%04o) failed\n", level, flags);
							//if ((fd = open(buffer, flags, 0666)) != EOF) close(fd);
							//else fprintf(stderr, "%d: open(%04o) failed\n", level, flags);
							//if ((fd = open(buffer, flags, 0666)) != EOF) close(fd);
							//else fprintf(stderr, "%d: open(%04o) failed\n", level, flags);
						}
						for (i = 1; i < 8; i++) fprintf(fp_domain, "delete %d %s\n", i, buffer);
						fprintf(fp_domain, "delete allow_truncate %s\n", buffer);
						fprintf(fp_domain, "delete allow_create %s\n", buffer);
						fprintf(fp_domain, "delete allow_rewrite %s\n", buffer);
						fflush(fp_domain);
						if ((fd = open(buffer, flags, 0666)) != EOF) {
							close(fd);
							fprintf(stderr, "%d: open(%04o) didn't fail\n", 3, flags);
						}
					}
				}
			}
		}
	}
	fprintf(fp_level, "255-MAC_FOR_FILE=0\n");
	fflush(fp_level);
	printf("Done\n");
	return 0;
}
