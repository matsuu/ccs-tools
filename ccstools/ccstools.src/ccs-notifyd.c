/*
 * ccs-notifyd.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0-pre   2008/01/19
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

int main(int argc, char *argv[]) {
	int time_to_wait;
	const char *action_to_take;
	char buffer[16384];
	FILE *fp;
	int query_fd;
	unsetenv("SHELLOPTS"); /* Make sure popen() executes commands. */
	if (argc != 3) {
		printf("Usage: %s time-to-wait action-to-take\n\n", argv[0]);
		printf("This program is used for notifying policy violation in delayed enforcing mode to administrators.\n");
		printf("The time-to-wait parameter is grace time in second before terminating this program.\n");
		printf("The action-to-take parameter is action you want to use for notification.\n\n");
		printf("%s 180 'mail admin@example.com'\n", argv[0]);
		printf("   will wait for 180 seconds after sending mail to admin@example.com (if SMTP service is available).\n\n");
		printf("%s 30 'curl --data-binary @- https://your.server/path_to_cgi'\n", argv[0]);
		printf("   will wait for 30 seconds after executing curl (where path_to_cgi is a CGI that reads message from stdin).\n\n");
		printf("If you don't start ccs-queryd within time-to-wait seconds, the request that caused policy violation "
		       "in delayed enforcing mode will be rejected. In other words, if you set time-to-wait to 0, you can use this utility "
		       "for just notifying administrator the occurrence of first policy violation in enforcing mode.\n");
		printf("The action-to-take parameter is passed to system(), so escape appropriately as needed.\n");
		printf("To avoid deadlock, please be careful that policy violation won't occur while executing action-to-take.\n");
		return 0;
	}
	query_fd = open("/proc/ccs/query", O_RDONLY);
	if (query_fd == EOF) query_fd = open("/sys/kernel/security/tomoyo/query", O_RDONLY);
	if (query_fd == EOF) {
		fprintf(stderr, "You can't run this utility for this kernel.\n");
		return 1;
	}
	{ // Get exclusive lock.
		int fd = open("/proc/self/exe", O_RDONLY); if (flock(fd, LOCK_EX | LOCK_NB) == EOF) return 0;
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
	/*
	 * Set timeout to 60 seconds for starting action.
	 * This is a safeguard against deadlock of delayed enforcing mode
	 * because this process is a delayed enforcing mode handler but
	 * popen() may trigger policy violation in delayed enforcing mode.
	 * We close query_fd now if there is no need to give administrator
	 * grace period for starting ccs-queryd .
	 */
	if (!time_to_wait) close(query_fd); 
	alarm(60);
	fp = popen(action_to_take, "w");
	if (!fp) {
		fprintf(stderr, "Can't execute %s\n", action_to_take);
		return 1;
	}
	fprintf(fp, "%s\n", buffer);
	/* Set timeout to 180 seconds for terminating action. */
	alarm(180);
	pclose(fp);
	alarm(0);
	sleep(time_to_wait);
	syslog(LOG_WARNING, "Terminated.\n");
	closelog();
	return 0;
}
