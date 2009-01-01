/*
 * ccstree.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.6-pre   2008/12/16
 *
 */
#include "ccstools.h"

static pid_t get_ppid(const pid_t pid)
{
	char buffer[1024];
	FILE *fp;
	pid_t ppid = 1;
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/status", pid);
	fp = fopen(buffer, "r");
	if (fp) {
		while (memset(buffer, 0, sizeof(buffer)),
		       fgets(buffer, sizeof(buffer) - 1, fp)) {
			if (sscanf(buffer, "PPid: %u", &ppid) == 1)
				break;
		}
		fclose(fp);
	}
	return ppid;
}

static char *get_name(const pid_t pid)
{
	char buffer[1024];
	FILE *fp;
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/status", pid);
	fp = fopen(buffer, "r");
	if (fp) {
		while (memset(buffer, 0, sizeof(buffer)),
		       fgets(buffer, sizeof(buffer) - 1, fp)) {
			if (!strncmp(buffer, "Name:", 5)) {
				char *cp = buffer + 5;
				while (*cp == ' ' || *cp == '\t')
					cp++;
				memmove(buffer, cp, strlen(cp) + 1);
				cp = strchr(buffer, '\n');
				if (cp)
					*cp = '\0';
				break;
			}
		}
		fclose(fp);
		if (buffer[0])
			return strdup(buffer);
	}
	return NULL;
}

static int status_fd = EOF;

static const char *read_info(const pid_t pid, int *profile)
{
	char *cp; /* caller must use get()/put(). */
	shprintf("%d\n", pid);
	write(status_fd, shared_buffer, strlen(shared_buffer));
	memset(shared_buffer, 0, sizeof(shared_buffer));
	read(status_fd, shared_buffer, sizeof(shared_buffer) - 1);
	cp = strchr(shared_buffer, ' ');
	if (cp) {
		*profile = atoi(cp + 1);
		cp = strchr(cp + 1, ' ');
		if (cp)
			return cp + 1;
	}
	*profile = -1;
	return "<UNKNOWN>";
}

static struct task_entry *task_list = NULL;
static int task_list_len = 0;

static void dump(const pid_t pid, const int depth)
{
	int i;
	for (i = 0; i < task_list_len; i++) {
		const char *info;
		char *name;
		int j;
		int profile;
		if (pid != task_list[i].pid)
			continue;
		name = get_name(pid);
		get();
		info = read_info(pid, &profile);
		printf("%3d", profile);
		for (j = 0; j < depth - 1; j++)
			printf("    ");
		for (; j < depth; j++)
			printf("  +-");
		printf(" %s (%u) %s\n", name, pid, info);
		put();
		free(name);
		task_list[i].done = true;
	}
	for (i = 0; i < task_list_len; i++) {
		if (pid != task_list[i].ppid)
			continue;
		dump(task_list[i].pid, depth + 1);
	}
}

static void dump_unprocessed(void)
{
	int i;
	for (i = 0; i < task_list_len; i++) {
		const char *info;
		char *name;
		int profile;
		const pid_t pid = task_list[i].pid;
		if (task_list[i].done)
			continue;
		name = get_name(task_list[i].pid);
		get();
		info = read_info(pid, &profile);
		printf("%3d %s (%u) %s\n", profile, name, pid, info);
		put();
		free(name);
		task_list[i].done = true;
	}
}

int ccstree_main(int argc, char *argv[])
{
	const char *policy_file = proc_policy_process_status;
	static _Bool show_all = false;
	if (access(proc_policy_dir, F_OK)) {
		fprintf(stderr, "You can't use this command "
			"for this kernel.\n");
		return 1;
	}
	if (argc > 1) {
		if (!strcmp(argv[1], "-a")) {
			show_all = true;
		} else {
			fprintf(stderr, "Usage: %s [-a]\n", argv[0]);
			return 0;
		}
	}
	status_fd = open(policy_file, O_RDWR);
	if (status_fd == EOF) {
		fprintf(stderr, "Can't open %s\n", policy_file);
		return 1;
	}
	{
		struct dirent **namelist;
		int i;
		int n = scandir("/proc/", &namelist, 0, 0);
		for (i = 0; i < n; i++) {
			pid_t pid;
			char buffer[128];
			char test[16];
			if (sscanf(namelist[i]->d_name, "%u", &pid) != 1)
				goto skip;
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/exe",
				 pid);
			if (show_all ||
			    readlink(buffer, test, sizeof(test)) > 0) {
				task_list = realloc(task_list,
						    (task_list_len + 1) *
						    sizeof(struct task_entry));
				if (!task_list)
					out_of_memory();
				task_list[task_list_len].pid = pid;
				task_list[task_list_len].ppid = get_ppid(pid);
				task_list[task_list_len].done = false;
				task_list_len++;
			}
skip:
			free((void *) namelist[i]);
		}
		if (n >= 0)
			free((void *) namelist);
	}
	dump(1, 0);
	dump_unprocessed();
	close(status_fd);
	return 0;
}
