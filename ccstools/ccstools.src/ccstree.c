/*
 * ccstree.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.4.1   2007/06/05
 *
 */
#include "ccstools.h"

static pid_t GetPPID(const pid_t pid) {
	char buffer[1024];
	FILE *fp;
	pid_t ppid = 1;
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/status", pid);
	if ((fp = fopen(buffer, "r")) != NULL) {
		while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 1, fp)) {
			if (sscanf(buffer, "PPid: %u", &ppid) == 1) break;
		}
		fclose(fp);
	}
	return ppid;
}

static char *GetName(const pid_t pid) {
	char buffer[1024];
	FILE *fp;
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/status", pid);
	if ((fp = fopen(buffer, "r")) != NULL) {
		while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 1, fp)) {
			if (strncmp(buffer, "Name:", 5) == 0) {
				char *cp = buffer + 5;
				while (*cp == ' ' || *cp == '\t') cp++;
				memmove(buffer, cp, strlen(cp) + 1);
				if ((cp = strchr(buffer, '\n')) != NULL) *cp = '\0';
				break;
			}
		}
		fclose(fp);
		if (buffer[0]) return strdup(buffer);
	}
	return NULL;
}

static int status_fd = EOF;

static const char *ReadInfo(const pid_t pid, int *profile) {
	char *cp; /* caller must use get()/put(). */
	memset(shared_buffer, 0, shared_buffer_len);
	snprintf(shared_buffer, shared_buffer_len - 1, "%d\n", pid);
	write(status_fd, shared_buffer, strlen(shared_buffer));
	memset(shared_buffer, 0, shared_buffer_len);
	read(status_fd, shared_buffer, shared_buffer_len - 1);
	if ((cp = strchr(shared_buffer, ' ')) != NULL) {
		*profile = atoi(cp + 1);
		if ((cp = strchr(cp + 1, ' ')) != NULL) {
			return cp + 1;
		}
	}
	*profile = -1;
	return "<UNKNOWN>";
}

static TASK_ENTRY *task_list = NULL;
static int task_list_len = 0;

static void Dump(const pid_t pid, const int depth) {
	int i;
	for (i = 0; i < task_list_len; i++) {
		const char *info;
		char *name;
		int j, profile;
		if (pid != task_list[i].pid) continue;
		name = GetName(pid);
		get();
		info = ReadInfo(pid, &profile);
		printf("%3d", profile);
		for (j = 0; j < depth - 1; j++) printf("    ");
		for (; j < depth; j++) printf("  +-");
		printf(" %s (%u) %s\n", name, pid, info);
		put();
		free(name);
		task_list[i].done = 1;
	}
	for (i = 0; i < task_list_len; i++) {
		if (pid != task_list[i].ppid) continue;
		Dump(task_list[i].pid, depth + 1);
	}
}

static void DumpUnprocessed(void) {
	int i;
	for (i = 0; i < task_list_len; i++) {
		const char *info;
		char *name;
		int profile;
		const pid_t pid = task_list[i].pid;
		if (task_list[i].done) continue;
		name = GetName(task_list[i].pid);
		get();
		info = ReadInfo(pid, &profile);
		printf("%3d %s (%u) %s\n", profile, name, pid, info);
		put();
		free(name);
		task_list[i].done = 1;
	}
}

int ccstree_main(int argc, char *argv[]) {
	static const char *policy_file = "/proc/ccs/info/.process_status";
	static int show_all = 0;
	if (access("/proc/ccs/", F_OK)) {
		fprintf(stderr, "You can't use this command for this kernel.\n");
		return 1;
	}
	if (argc > 1) {
		if (strcmp(argv[1], "-a") == 0) {
			show_all = 1;
		} else {
			fprintf(stderr, "Usage: %s [-a]\n", argv[0]);
			return 0;
		}
	}
	if ((status_fd = open(policy_file, O_RDWR)) == EOF) {
		fprintf(stderr, "Can't open %s\n", policy_file);
		return 1;
	}
	{
		struct dirent **namelist;
		int i, n = scandir("/proc/", &namelist, 0, 0);
		for (i = 0; i < n; i++) {
			pid_t pid;
			if (sscanf(namelist[i]->d_name, "%u", &pid) == 1) {
				char buffer[128], test[16];
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/exe", pid);
				if (show_all || readlink(buffer, test, sizeof(test)) > 0) {
					task_list = (TASK_ENTRY *) realloc(task_list, (task_list_len + 1) * sizeof(TASK_ENTRY));
					task_list[task_list_len].pid = pid;
					task_list[task_list_len].ppid = GetPPID(pid);
					task_list[task_list_len].done = 0;
					task_list_len++;
				}
			}
			free((void *) namelist[i]);
		}
		if (n >= 0) free((void *) namelist);
	}
	Dump(1, 0);
	DumpUnprocessed();
	close(status_fd);
	return 0;
}
