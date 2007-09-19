/*
 * ld-watch.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0-pre   2007/08/12
 *
 */
#include "ccstools.h"

int ldwatch_main(int argc, char *argv[]) {
	struct dll_pathname_entry *entry_list = NULL;
	int entry_list_count = 0;
	FILE *fp_policy;
	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		printf("Usage: %s file_to_exclude1 [file_to_exclude2 [...]]\n\n", argv[0]);
		printf("This program automatically registers files shown by 'ldconfig -NXp' as globally readable files.\n");
		printf("This program registers all files shown by 'ldconfig -NXp' by default, but you can specify files that you don't want to register by command line.\n");
		printf("For example, if you invoke\n");
		printf("  %s /lib/libcustom-1.0.0.so /lib/libcustom.so.1\n", argv[0]);
		printf("then, /lib/libcustom-1.0.0.so and /lib/libcustom.so.1 will be excluded from the result of 'ldconfig -NXp'.\n\n");
		printf("Start this program in one window, then update packages in another window.\n");
		printf("After you finished updating, wait for several seconds and terminate this program with 'Ctrl-C'.\n");
		return 0;
	}
	{
		const int fd = open(proc_policy_exception_policy, O_RDWR);
		if (fd == EOF) {
			fprintf(stderr, "You can't run this daemon for this kernel.\n");
			return 1;
		} else if (write(fd, "", 0) != 0) {
			fprintf(stderr, "You need to register this program to %s to run this program.\n", proc_policy_manager);
			return 1;
		}
		close(fd);
	}
	if ((fp_policy = fopen(proc_policy_exception_policy, "w")) == NULL) {
		fprintf(stderr, "Can't open policy file.\n");
		exit(1);
	}
	while (1) {
		struct stat64 buf;
		static time_t last_modified = 0;
		int i;
		if (stat64("/etc/ld.so.cache", &buf) == 0 && buf.st_mtime != last_modified) {
			FILE *fp_ldconfig;
			if ((fp_ldconfig = popen("/sbin/ldconfig -NXp", "r")) != NULL) {
				char buffer[16384];
				last_modified = buf.st_mtime;
				while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 1, fp_ldconfig)) {
					char *cp, *pathname, *real_pathname;
					if ((cp = strchr(buffer, '\n')) == NULL) continue;
					*cp = '\0';
					cp = strrchr(buffer, ' ');
					if (!cp || *++cp != '/') continue;
					// Check for duplicated entries.
					if ((real_pathname = realpath(cp, NULL)) == NULL) continue;
					for (i = 0; i < entry_list_count; i++) {
						if (strcmp(entry_list[i].real_pathname, real_pathname) == 0) break;
					}
					if (i < entry_list_count) {
						free(real_pathname);
						continue;
					}
					// Exclude if listed by command line.
					for (i = 1; i < argc; i++) {
						if (strcmp(argv[i], real_pathname) == 0 || strcmp(argv[i], cp) == 0) break;
					}
					if (i < argc) {
						printf("Skipped %s : %s\n", cp, real_pathname);
						free(real_pathname);
						continue;
					}
					// Add an entry.
					pathname = strdup(cp);
					entry_list = (struct dll_pathname_entry *) realloc(entry_list, (entry_list_count + 1) * sizeof(struct dll_pathname_entry));
					entry_list[entry_list_count].pathname = pathname;
					entry_list[entry_list_count++].real_pathname = real_pathname;
					printf("Added %s : %s\n", pathname, real_pathname);
					fprintf(fp_policy, KEYWORD_ALLOW_READ);
					fprintf_encoded(fp_policy, real_pathname);
					fprintf(fp_policy, "\n");
					fflush(fp_policy);
				}
				pclose(fp_ldconfig);
			}
			printf("Monitoring %d files.\n", entry_list_count);
		}
		// Check entries for update.
		for (i = 0; i < entry_list_count; i++) {
			struct dll_pathname_entry *ptr = &entry_list[i];
			char *real_pathname = realpath(ptr->pathname, NULL);
			if (real_pathname && strcmp(ptr->real_pathname, real_pathname)) {
				printf("Changed %s : %s -> %s\n", ptr->pathname, ptr->real_pathname, real_pathname);
				fprintf(fp_policy, KEYWORD_ALLOW_READ);
				fprintf_encoded(fp_policy, real_pathname);
				fprintf(fp_policy, "\n");
				fflush(fp_policy);
				free(ptr->real_pathname); ptr->real_pathname = real_pathname; real_pathname = NULL;
			}
			free(real_pathname);
		}
		sleep(1);
	}
	fclose(fp_policy);
	return 0;
}
