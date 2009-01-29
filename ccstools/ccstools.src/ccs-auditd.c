/*
 * ccs-auditd.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-pre   2009/02/02
 *
 */
#include "ccstools.h"

int ccsauditd_main(int argc, char *argv[])
{
	const char *procfile_path[CCS_AUDITD_MAX_FILES] = {
		proc_policy_grant_log,
		proc_policy_reject_log
	};
	int i;
	int fd_in[CCS_AUDITD_MAX_FILES];
	int fd_out[CCS_AUDITD_MAX_FILES];
	const char *logfile_path[2] = { "/dev/null", "/dev/null" };
	if (access(proc_policy_dir, F_OK)) {
		fprintf(stderr, "You can't run this daemon for this kernel.\n");
		return 0;
	}
	if (argc < 3) {
		fprintf(stderr, "%s grant_log_file reject_log_file\n"
			"  These files may /dev/null, if needn't to be saved."
			"\n", argv[0]);
		return 0;
	}
	logfile_path[0] = argv[1];
	logfile_path[1] = argv[2];
	{ /* Get exclusive lock. */
		int fd = open("/proc/self/exe", O_RDONLY);
		if (flock(fd, LOCK_EX | LOCK_NB) == EOF)
			return 0;
	}
	umask(0);
	for (i = 0; i < CCS_AUDITD_MAX_FILES; i++) {
		fd_out[i] = open(logfile_path[i], O_WRONLY | O_CREAT |
				 O_APPEND, 0600);
		if (fd_out[i] == EOF) {
			fprintf(stderr, "Can't open %s for writing.\n",
				logfile_path[i]);
			return 1;
		}
	}
	switch (fork()) {
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
	switch (fork()) {
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
	close(0);
	close(1);
	close(2);
	openlog("ccs-auditd", 0,  LOG_USER);
	for (i = 0; i < CCS_AUDITD_MAX_FILES; i++) {
		fd_in[i] = open(procfile_path[i], O_RDONLY);
		if (fd_in[i] == EOF) {
			syslog(LOG_WARNING, "Can't open %s for reading.\n",
			       procfile_path[i]);
			return 1;
		}
	}
	syslog(LOG_WARNING, "Started.\n");
	while (true) {
		char buffer[16384];
		char timestamp[128];
		fd_set rfds;
		FD_ZERO(&rfds);
		for (i = 0; i < CCS_AUDITD_MAX_FILES; i++)
			FD_SET(fd_in[i], &rfds);
		/* Wait for data. */
		if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) == EOF)
			break;
		for (i = 0; i < CCS_AUDITD_MAX_FILES; i++) {
			time_t stamp;
			char *cp;
			int len;
			struct tm *tm;
			if (!FD_ISSET(fd_in[i], &rfds))
				continue;
			memset(buffer, 0, sizeof(buffer));
			if (read(fd_in[i], buffer, sizeof(buffer) - 1) < 0)
				continue;
			memset(timestamp, 0, sizeof(timestamp));
			if (sscanf(buffer, "#timestamp=%lu", &stamp) != 1)
				goto no_timestamp;
			cp = strchr(buffer, ' ');
			if (!cp)
				goto no_timestamp;
			tm = localtime(&stamp);
			snprintf(timestamp, sizeof(timestamp) - 1,
				 "#%04d-%02d-%02d %02d:%02d:%02d#",
				 tm->tm_year + 1900, tm->tm_mon + 1,
				 tm->tm_mday, tm->tm_hour, tm->tm_min,
				 tm->tm_sec);
			memmove(buffer, cp, strlen(cp) + 1);
no_timestamp:
			/* Open destination file. */
			if (access(logfile_path[i], F_OK)) {
				close(fd_out[i]);
				fd_out[i] = open(logfile_path[i],
						 O_WRONLY | O_CREAT | O_APPEND,
						 0600);
				if (fd_out[i] == EOF) {
					syslog(LOG_WARNING,
					       "Can't open %s for writing.\n",
					       logfile_path[i]);
					goto out;
				}
			}
			len = strlen(timestamp);
			write(fd_out[i], timestamp, len);
			len = strlen(buffer);
			write(fd_out[i], buffer, len);
			write(fd_out[i], "\n", 1);
			fsync(fd_out[i]);
		}
	}
out:
	syslog(LOG_WARNING, "Terminated.\n");
	closelog();
	return 0;
}
