/*
 * ccstree.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7+   2009/04/28
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
		static const int offset = sizeof(buffer) / 6;
		while (memset(buffer, 0, sizeof(buffer)),
		       fgets(buffer, sizeof(buffer) - 1, fp)) {
			if (!strncmp(buffer, "Name:\t", 6)) {
				char *cp = buffer + 6;
				memmove(buffer, cp, strlen(cp) + 1);
				cp = strchr(buffer, '\n');
				if (cp)
					*cp = '\0';
				break;
			}
		}
		fclose(fp);
		if (buffer[0] && strlen(buffer) < offset - 1) {
			const char *src = buffer;
			char *dest = buffer + offset;
			while (1) {
				unsigned char c = *src++;
				if (!c) {
					*dest = '\0';
					break;
				}
				if (c == '\\') {
					c = *src++;
					if (c == '\\') {
						memmove(dest, "\\\\", 2);
						dest += 2;
					} else if (c == 'n') {
						memmove(dest, "\\012", 4);
						dest += 4;
					} else {
						break;
					}
				} else if (c > ' ' && c <= 126) {
					*dest++ = c;
				} else {
					*dest++ = '\\';
					*dest++ = (c >> 6) + '0';
					*dest++ = ((c >> 3) & 7) + '0';
					*dest++ = (c & 7) + '0';
				}
			}
			return strdup(buffer + offset);
		}
	}
	return NULL;
}

static struct task_entry *task_list = NULL;
static int task_list_len = 0;

static void dump(const pid_t pid, const int depth)
{
	int i;
	for (i = 0; i < task_list_len; i++) {
		int j;
		if (pid != task_list[i].pid)
			continue;
		printf("%3d", task_list[i].profile);
		for (j = 0; j < depth - 1; j++)
			printf("    ");
		for (; j < depth; j++)
			printf("  +-");
		printf(" %s (%u) %s\n", task_list[i].name,
		       task_list[i].pid, task_list[i].domain);
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
		if (task_list[i].done)
			continue;
		printf("%3d %s (%u) %s\n",
		       task_list[i].profile, task_list[i].name,
		       task_list[i].pid, task_list[i].domain);
		task_list[i].done = true;
	}
}

int ccstree_main(int argc, char *argv[])
{
	const char *policy_file = proc_policy_process_status;
	static _Bool show_all = false;
	if (argc > 1) {
		char *ptr = argv[1];
		char *cp = strchr(ptr, ':');
		if (cp) {
			*cp++ = '\0';
			network_ip = inet_addr(ptr);
			network_port = htons(atoi(cp));
			network_mode = true;
			if (!check_remote_host())
				return 1;
		} else if (!strcmp(ptr, "-a")) {
			show_all = true;
		} else {
			fprintf(stderr, "Usage: %s "
				"[{-a|remote_ip:remote_port}]}\n", argv[0]);
			return 0;
		}
	}
	if (network_mode) {
		FILE *fp = open_read("proc:process_status");
		if (!fp) {
			fprintf(stderr, "Can't connect.\n");
			return 1;
		}
		get();
		while (freadline(fp)) {
			unsigned int pid = 0;
			unsigned int ppid = 0;
			int profile = -1;
			char *name;
			char *domain;
			sscanf(shared_buffer, "PID=%u PPID=%u", &pid, &ppid);
			name = strstr(shared_buffer, "NAME=");
			if (name)
				name = strdup(name + 5);
			if (!name)
				name = "<UNKNOWN>";
			if (!freadline(fp))
				break;
			sscanf(shared_buffer, "%u %u", &pid, &profile);
			domain = strchr(shared_buffer, '<');
			if (domain)
				domain = strdup(domain);
			if (!domain)
				domain = "<UNKNOWN>";
			task_list = realloc(task_list,
					    (task_list_len + 1) *
					    sizeof(struct task_entry));
			if (!task_list)
				out_of_memory();
			task_list[task_list_len].pid = pid;
			task_list[task_list_len].ppid = ppid;
			task_list[task_list_len].profile = profile;
			task_list[task_list_len].name = name;
			task_list[task_list_len].domain = domain;
			task_list[task_list_len].done = false;
			task_list_len++;
		}
		put();
		fclose(fp);
	} else {
		struct dirent **namelist;
		int i;
		int n;
		int status_fd;
		if (access(proc_policy_dir, F_OK)) {
			fprintf(stderr, "You can't use this command "
				"for this kernel.\n");
			return 1;
		}
		status_fd = open(policy_file, O_RDWR);
		if (status_fd == EOF) {
			fprintf(stderr, "Can't open %s\n", policy_file);
			return 1;
		}
		n = scandir("/proc/", &namelist, 0, 0);
		for (i = 0; i < n; i++) {
			char *name;
			char *domain;
			int profile = -1;
			unsigned int pid = 0;
			char buffer[128];
			char test[16];
			if (sscanf(namelist[i]->d_name, "%u", &pid) != 1)
				goto skip;
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/exe",
				 pid);
			if (!show_all &&
			    readlink(buffer, test, sizeof(test)) <= 0)
				goto skip;
			name = get_name(pid);
			if (!name)
				name = "<UNKNOWN>";
			snprintf(buffer, sizeof(buffer) - 1, "%u\n", pid);
			write(status_fd, buffer, strlen(buffer));
			get();
			memset(shared_buffer, 0, sizeof(shared_buffer));
			read(status_fd, shared_buffer,
			     sizeof(shared_buffer) - 1);
			sscanf(shared_buffer, "%u %u", &pid, &profile);
			domain = strchr(shared_buffer, '<');
			if (domain)
				domain = strdup(domain);
			if (!domain)
				domain = "<UNKNOWN>";
			put();
			task_list = realloc(task_list, (task_list_len + 1) *
					    sizeof(struct task_entry));
			if (!task_list)
				out_of_memory();
			task_list[task_list_len].pid = pid;
			task_list[task_list_len].ppid = get_ppid(pid);
			task_list[task_list_len].profile = profile;
			task_list[task_list_len].name = name;
			task_list[task_list_len].domain = domain;
			task_list[task_list_len].done = false;
			task_list_len++;
skip:
			free((void *) namelist[i]);
		}
		if (n >= 0)
			free((void *) namelist);
		close(status_fd);
	}
	dump(1, 0);
	dump_unprocessed();
	return 0;
}
