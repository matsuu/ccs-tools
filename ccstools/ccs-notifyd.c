/*
 * ccs-notifyd.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0   2008/04/01
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <syslog.h>
#include <signal.h>

int main(int argc, char *argv[]) {
	const char *proc_policy_query = "/proc/ccs/query";
	int time_to_wait;
	const char *action_to_take;
	char buffer[16384];
	FILE *fp;
	int query_fd;
	unsetenv("SHELLOPTS"); /* Make sure popen() executes commands. */
	if (argc != 3) {
		printf("Usage: %s time-to-wait action-to-take\n\n", argv[0]);
		printf("This program is used for notifying the first occurrence of policy violation in delayed enforcing mode.\n"
		       "The time-to-wait parameter is grace time in second before rejecting the request that caused policy violation "
		       "in delayed enforcing mode.\n"
		       "The action-to-take parameter is action you want to use for notification. "
		       "This parameter is passed to system(), so escape appropriately as needed.\n\n");
		printf("Examples:\n\n");
		printf("  %s 180 'mail admin@example.com'\n", argv[0]);
		printf("        Wait for 180 seconds before rejecting the request. The occurrence is notified by sending mail to admin@example.com (if SMTP service is available).\n\n");
		printf("  %s 0 'curl --data-binary @- https://your.server/path_to_cgi'\n", argv[0]);
		printf("        Reject the request immediately. The occurrence is notified by executing curl command.\n\n");
		return 0;
	}
	time_to_wait = atoi(argv[1]);
	action_to_take = argv[2];
	umask(0);
	switch(fork()) {
	case 0:
		break;
	case -1:
		fprintf(stderr, "Can't fork()\n");
		return 1;
	default:
		return 0;
	}
	if (setsid() == EOF) {
		fprintf(stderr, "Can't setsid()\n");
		return 1;
	}
	switch(fork()) {
	case 0:
		break;
	case -1:
		fprintf(stderr, "Can't fork()\n");
		return 1;
	default:
		return 0;
	}
	if (chdir("/")) {
		fprintf(stderr, "Can't chdir()\n");
		return 1;
	}
	{ // Get exclusive lock.
		int fd = open("/proc/self/exe", O_RDONLY); if (flock(fd, LOCK_EX | LOCK_NB) == EOF) return 0;
	}
	query_fd = open(proc_policy_query, O_RDWR);
	if (query_fd == EOF) {
		proc_policy_query = "/sys/kernel/security/tomoyo/query";
		query_fd = open(proc_policy_query, O_RDWR);
	}
	if (query_fd == EOF) {
		fprintf(stderr, "You can't run this utility for this kernel.\n");
		return 1;
	} else if (time_to_wait && write(query_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to %s to run this program.\n", proc_policy_query);
		return 1;
	}
	close(0); close(1); close(2);
	openlog("ccs-notifyd", 0,  LOG_USER);
	syslog(LOG_WARNING, "Started. (%d, %s)\n", time_to_wait, action_to_take);
	while (1) {
		fd_set rfds;
		sleep(1);
		// Wait for query.
		FD_ZERO(&rfds);
		FD_SET(query_fd, &rfds);
		select(query_fd + 1, &rfds, NULL, NULL, NULL);
		if (!FD_ISSET(query_fd, &rfds)) continue;
		// Read query.
		memset(buffer, 0, sizeof(buffer));
		if (read(query_fd, buffer, sizeof(buffer) - 1) <= 0) continue;
		break;
	}
	signal(SIGCLD, SIG_IGN);
	switch (fork()) {
	case 0:
		close(query_fd);
		fp = popen(action_to_take, "w");
		if (!fp) {
			syslog(LOG_WARNING, "Can't execute %s\n", action_to_take);
			closelog();
			_exit(1);
		}
		fprintf(fp, "%s\n", buffer);
		pclose(fp);
		_exit(0);
	case -1:
		syslog(LOG_WARNING, "Can't execute %s\n", action_to_take);
		break;
	default:
		while (time_to_wait-- > 0) {
			sleep(1);
			write(query_fd, "\n", 1);
		}
	}
	close(query_fd);
	syslog(LOG_WARNING, "Terminated.\n");
	closelog();
	return 0;
}
