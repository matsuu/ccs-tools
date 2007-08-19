/*
 * makesyaoranconf.c
 *
 * Generate policy template file for SYAORAN.
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.3.1   2006/12/08
 *
 * This program generates policy template file for SYAORAN
 * (Implementation of the Tamper-Proof Device Filesystem).
 *
 * You can pass the output of this program when you mount SYAORAN.
 * The proceedure for using SYAORAN is described in a documentation file
 * "Policy Specifications of SYAORAN filesystem".
 *
 */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/kdev_t.h>

static void DumpPath(const char *buffer) {
	unsigned char c;
	int width = 20;
	while ((c = * (const unsigned char *) buffer) != '\0') {
		buffer++;
		if (c <= 32 || c > 126) {
			printf("\\%c%c%c", (c >> 6) + '0', ((c >> 3) & 7) + '0', (c & 7) + '0');
			width -= 3;
		} else if (c == '\\') {
			putchar('\\'); putchar('\\');
			width -= 2;
		} else {
			putchar(c);
			width--;
		}
	}
	while (width-- > 0) putchar(' ');
}

#define PAGE_SIZE 4096

static int root_dir_len = 0;

static void FindFiles(const char *path) {
	struct dirent **namelist;
	int i, n = scandir(path, &namelist, 0, versionsort);
	char *filename;
	//fprintf(stderr, "scandir(%s)=%d\n", path, n);
	if (n < 0) return;
	if ((filename = malloc(PAGE_SIZE)) == NULL) {
		fprintf(stderr, "FATAL: Out of memory.\n");
		exit(1);
	}
	memset(filename, 0, PAGE_SIZE);
	for (i = 0; i < n; i++) {
		const char *file = namelist[i]->d_name;
		struct stat64 buf;
		if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0) {
			free((void *) namelist[i]);
			continue;
		}
		snprintf(filename, PAGE_SIZE - 2, "%s%s", path, file);
		free((void *) namelist[i]);
		file = NULL;
		if (lstat64(filename, &buf) == 0) {
			static char symlink_buffer[PAGE_SIZE];
			const uid_t user = buf.st_uid;
			const gid_t group = buf.st_gid;
			const mode_t mode = buf.st_mode;
			const mode_t perm = mode & 0777;
			const unsigned int major = MAJOR(buf.st_rdev);
			const unsigned int minor = MINOR(buf.st_rdev);
			const unsigned int flags = 0;
			if (!S_ISBLK(mode) && !S_ISCHR(mode) && !S_ISSOCK(mode) && !S_ISFIFO(mode) && !S_ISDIR(mode) && !S_ISLNK(mode) && !S_ISREG(mode)) continue;
			if (S_ISLNK(mode)) {
				memset(symlink_buffer, 0, sizeof(symlink_buffer));
				if (readlink(filename, symlink_buffer, sizeof(symlink_buffer) - 1) <= 0) continue;
			}
			if (strlen(filename) <= root_dir_len) continue;
			DumpPath(filename + root_dir_len);
			printf(" %3o %3d %3d %2u ", perm, user, group, flags);
			if (S_ISBLK(mode)) {
				printf("b %3u %3u\n", major, minor);
			} else if (S_ISCHR(mode)) {
				printf("c %3u %3u\n", major, minor);
			} else if (S_ISSOCK(mode)) {
				printf("s\n");
			} else if (S_ISFIFO(mode)) {
				printf("p\n");
			} else if (S_ISDIR(mode)) {
				printf("d\n");
				strcat(filename, "/");
				FindFiles(filename);
			} else if (S_ISLNK(mode)) {
				printf("l ");
				DumpPath(symlink_buffer);
				printf("\n");
			} else if (S_ISREG(mode)) {
				printf("f\n");
			}
		}
	}
	free((void *) namelist);
	free(filename);
}

int main(int argc, char *argv[]) {
	const char *root_dir = "/dev/";
	if (argc > 1) {
		if (chdir(argv[1])) {
			fprintf(stderr, "Can't chdir to %s\n", argv[1]);
			return 1;
		}
		root_dir = argv[1];
	}
	root_dir_len = strlen(root_dir);
	if (root_dir[root_dir_len - 1] != '/') {
		char *cp = malloc(root_dir_len + 16);
		if (!cp) return 1;
		snprintf(cp, root_dir_len + 16, "%s/", root_dir);
		root_dir = cp;
		root_dir_len = strlen(root_dir);
	}
	printf("#filename permission uid gid flags type [ symlink_data | major minor ]\n");
	FindFiles(root_dir);
	return 0;
}
