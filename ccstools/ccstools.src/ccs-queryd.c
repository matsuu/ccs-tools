/*
 * ccs-queryd.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.4+   2008/10/02
 *
 */
#include "ccstools.h"

static bool has_retry_counter = false;
static unsigned short int retries = 0;

static void do_check_update(FILE *fp_out)
{
	FILE *fp_in = fopen(proc_policy_exception_policy, "r");
	char **pathnames = NULL;
	int pathnames_len = 0;
	char buffer[16384];
	memset(buffer, 0, sizeof(buffer));
	if (!fp_in) {
		fprintf(stderr, "Can't open policy file.\n");
		return;
	}
	while (fgets(buffer, sizeof(buffer) - 1, fp_in)) {
		char *cp = strchr(buffer, '\n');
		if (!cp)
			break;
		*cp = '\0';
		if (!str_starts(buffer, KEYWORD_ALLOW_READ))
			continue;
		if (!decode(buffer, buffer))
			continue;
		pathnames = realloc(pathnames, sizeof(char *) *
				    (pathnames_len + 1));
		if (!pathnames)
			out_of_memory();
		pathnames[pathnames_len] = strdup(buffer);
		if (!pathnames[pathnames_len])
			out_of_memory();
		pathnames_len++;
	}
	fclose(fp_in);
	while (true) {
		char buffer[16384];
		struct stat64 buf;
		static time_t last_modified = 0;
		int i;
		fflush(fp_out);
		sleep(1);
		for (i = 0; i < pathnames_len; i++) {
			int j;
			if (!stat64(pathnames[i], &buf))
				continue;
			fprintf(fp_out, "-");
			fprintf_encoded(fp_out, pathnames[i]);
			fprintf(fp_out, "\n");
			free(pathnames[i]);
			pathnames_len--;
			for (j = i; j < pathnames_len; j++)
				pathnames[j] = pathnames[j + 1];
			i--;
		}
		if (stat64("/etc/ld.so.cache", &buf) ||
		    buf.st_mtime == last_modified)
			continue;
		fp_in = popen("/sbin/ldconfig -NXp", "r");
		if (!fp_in)
			continue;
		last_modified = buf.st_mtime;
		memset(buffer, 0, sizeof(buffer));
		while (fgets(buffer, sizeof(buffer) - 1, fp_in)) {
			char *cp = strchr(buffer, '\n');
			char *real_pathname;
			if (!cp)
				break;
			*cp = '\0';
			cp = strrchr(buffer, ' ');
			if (!cp || *++cp != '/')
				continue;
			if (stat64(cp, &buf))
				continue;
			real_pathname = realpath(cp, NULL);
			if (!real_pathname)
				continue;
			for (i = 0; i < pathnames_len; i++) {
				if (!strcmp(real_pathname, pathnames[i]))
					break;
			}
			if (i == pathnames_len) {
				char *cp;
				pathnames = realloc(pathnames, sizeof(char *) *
						    (pathnames_len + 1));
				if (!pathnames)
					out_of_memory();
				cp = strdup(real_pathname);
				if (!cp)
					out_of_memory();
				pathnames[pathnames_len++] = cp;
				fprintf(fp_out, "+");
				fprintf_encoded(fp_out, pathnames[i]);
				fprintf(fp_out, "\n");
			}
			free(real_pathname);
		}
		pclose(fp_in);
	}
}

#define GLOBALLY_READABLE_FILES_UPDATE_NONE 0
#define GLOBALLY_READABLE_FILES_UPDATE_ASK  1
#define GLOBALLY_READABLE_FILES_UPDATE_AUTO 2

static int check_update = GLOBALLY_READABLE_FILES_UPDATE_AUTO;

static void handle_update(const int fd)
{
	static FILE *fp = NULL;
	static char pathname[8192];
	static int pathname_len = 0;
	int c;
	if (!fp)
		fp = fopen(proc_policy_exception_policy, "w");
	if (!pathname_len)
		memset(pathname, 0, sizeof(pathname));
	while (read(fd, pathname + pathname_len, 1) == 1 &&
	       pathname[pathname_len] != '\n' &&
	       pathname_len < sizeof(pathname) - 1)
		pathname_len++;
	pathname[pathname_len] = '\0';
	pathname_len = 0;
	if (check_update == GLOBALLY_READABLE_FILES_UPDATE_AUTO) {
		if (pathname[0] == '-')
			fprintf(fp, KEYWORD_DELETE);
		fprintf(fp, KEYWORD_ALLOW_READ "%s\n", pathname + 1);
		fflush(fp);
		printw("The pathname %s was %s globally readable file.\n\n",
		       pathname + 1, (pathname[0] == '-') ?
		       "deleted. Deleted from" : "created. Appended to");
		refresh();
		return;
	}
	printw("The pathname %s was %s globally readable file? ('Y'es/'N'o):",
	       pathname + 1, (pathname[0] == '-') ?
	       "deleted. Delete from" : "created. Append to");
	refresh();
	while (true) {
		c = getch2();
		if (c == 'Y' || c == 'y' || c == 'N' || c == 'n')
			break;
		write(query_fd, "\n", 1);
	}
	printw("%c\n", c);
	refresh();
	if (c == 'Y' || c == 'y') {
		if (pathname[0] == '-')
			fprintf(fp, KEYWORD_DELETE);
		fprintf(fp, KEYWORD_ALLOW_READ "%s\n", pathname + 1);
		fflush(fp);
	}
	printw("\n");
	refresh();
}

static int domain_policy_fd = EOF;
static const int max_readline_history = 20;
static const char **readline_history = NULL;
static int readline_history_count = 0;
static const int buffer_len = 32768;
static char *buffer = NULL;

static bool handle_query_new_format(unsigned int serial)
{
	int c = 0;
	int y;
	int x;
	char *line = NULL;
	static unsigned int prev_pid = 0;
	unsigned int pid;
	time_t stamp;
	char *cp = strstr(buffer, " pid=");
	if (!cp || sscanf(cp + 5, "%u", &pid) != 1) {
		printw("ERROR: Unsupported query.\n");
		return false;
	}
	cp = strchr(buffer, '\0');
	if (*(cp - 1) != '\n') {
		printw("ERROR: Unsupported query.\n");
		return false;
	}
	*(cp - 1) = '\0';
	if (pid != prev_pid) {
		if (prev_pid)
			printw("----------------------------------------\n");
		prev_pid = pid;
	}
	if (sscanf(buffer, "#timestamp=%lu", &stamp) == 1) {
		cp = strchr(buffer, ' ');
		if (cp) {
			struct tm *tm = localtime(&stamp);
			printw("#%04d-%02d-%02d %02d:%02d:%02d#",
			       tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			       tm->tm_hour, tm->tm_min, tm->tm_sec);
			memmove(buffer, cp, strlen(cp) + 1);
		}
	}
	printw("%s\n", buffer);
	/* Is this domain query? */
	if (strstr(buffer, "\n#"))
		goto not_domain_query;
	printw("Allow? ('Y'es/Yes and 'A'ppend to policy/'N'o%s):",
	       has_retry_counter ? "/'R'etry" : "");
	refresh();
	while (true) {
		c = getch2();
		if (c == 'Y' || c == 'y' || c == 'N' || c == 'n' ||
		    c == 'A' || c == 'a' ||
		    (has_retry_counter && (c == 'R' || c == 'r')))
			break;
		write(query_fd, "\n", 1);
	}
	printw("%c\n", c);
	refresh();

	/* Append to domain policy. */
	if (c != 'A' && c != 'a')
		goto not_append;
	getyx(stdscr, y, x);
	cp = strrchr(buffer, '\n');
	if (!cp)
		return false;
	*cp++ = '\0';
	initial_readline_data = cp;
	readline_history_count = simple_add_history(cp, readline_history,
						    readline_history_count,
						    max_readline_history);
	line = simple_readline(y, 0, "Enter new entry> ", readline_history,
			       readline_history_count, 4000, 8);
	scrollok(stdscr, TRUE);
	printw("\n");
	refresh();
	if (!line || !*line) {
		printw("None added.\n");
		goto not_append;
	}
	readline_history_count = simple_add_history(line, readline_history,
						    readline_history_count,
						    max_readline_history);
	write(domain_policy_fd, buffer, strlen(buffer));
	write(domain_policy_fd, "\n", 1);
	write(domain_policy_fd, line, strlen(line));
	write(domain_policy_fd, "\n", 1);
	printw("Added '%s'.\n", line);
not_append:
	refresh();
	free(line);
write_answer:
	/* Write answer. */
	if (c == 'Y' || c == 'y' || c == 'A' || c == 'a')
		c = 1;
	else if (c == 'R' || c == 'r')
		c = 3;
	else
		c = 2;
	snprintf(buffer, buffer_len - 1, "A%u=%u\n", serial, c);
	write(query_fd, buffer, strlen(buffer));
	printw("\n");
	refresh();
	return true;
not_domain_query:
	printw("Allow? ('Y'es/'N'o%s):", has_retry_counter ? "/'R'etry" : "");
	refresh();
	while (true) {
		c = getch2();
		if (c == 'Y' || c == 'y' || c == 'N' || c == 'n' ||
		    (has_retry_counter && (c == 'R' || c == 'r')))
			break;
		write(query_fd, "\n", 1);
	}
	printw("%c\n", c);
	refresh();
	goto write_answer;
}

static bool handle_query_old_format(unsigned int serial)
{
	int c = 0;
	int y;
	int x;
	char *cp;
	char *line = NULL;
	static char *prev_buffer = NULL;
	if (!prev_buffer) {
		prev_buffer = malloc(buffer_len);
		if (!prev_buffer)
			return false;
		memset(prev_buffer, 0, buffer_len);
	}
	/* Is this domain query? */
	if (strncmp(buffer, "<kernel>", 8))
		goto not_domain_query;
	if (buffer[8] != '\0' && buffer[8] != ' ')
		goto not_domain_query;
	cp = strchr(buffer, '\n');
	if (!cp)
		goto not_domain_query;

	/* Check for same domain. */
	*cp++ = '\0';
	if (strcmp(buffer, prev_buffer)) {
		printw("----------------------------------------\n");
		memmove(prev_buffer, buffer, strlen(buffer) + 1);
	}
	printw("%s\n", buffer);
	printw("%s", cp);
	printw("Allow? ('Y'es/Yes and 'A'ppend to policy/'N'o):");
	refresh();
	while (true) {
		c = getch2();
		if (c == 'Y' || c == 'y' || c == 'N' || c == 'n' ||
		    c == 'A' || c == 'a')
			break;
		write(query_fd, "\n", 1);
	}
	printw("%c\n", c);
	refresh();

	/* Append to domain policy. */
	if (c != 'A' && c != 'a')
		goto not_append;

	getyx(stdscr, y, x);
	line = strchr(cp, '\n');
	if (!line)
		*line = '\0';
	initial_readline_data = cp;
	readline_history_count = simple_add_history(cp, readline_history,
						    readline_history_count,
						    max_readline_history);
	line = simple_readline(y, 0, "Enter new entry> ", readline_history,
			       readline_history_count, 4000, 8);
	scrollok(stdscr, TRUE);
	printw("\n");
	refresh();
	if (!line || !*line) {
		printw("None added.\n");
		goto not_append;
	}
	readline_history_count = simple_add_history(line, readline_history,
						    readline_history_count,
						    max_readline_history);
	write(domain_policy_fd, buffer, strlen(buffer));
	write(domain_policy_fd, "\n", 1);
	write(domain_policy_fd, line, strlen(line));
	write(domain_policy_fd, "\n", 1);
	printw("Added '%s'.\n", line);
not_append:
	free(line);
write_answer:
	refresh();
	printw("\n");
	/* Write answer. */
	if (c == 'Y' || c == 'y' || c == 'A' || c == 'a')
		c = 1;
	else
		c = 2;
	snprintf(buffer, buffer_len - 1, "A%u=%u\n", serial, c);
	write(query_fd, buffer, strlen(buffer));
	return true;
not_domain_query:
	printw("----------------------------------------\n");
	prev_buffer[0] = '\0';
	printw("%s", buffer);
	printw("Allow? ('Y'es/'N'o):");
	refresh();
	while (true) {
		c = getch2();
		if (c == 'Y' || c == 'y' || c == 'N' || c == 'n')
			break;
		write(query_fd, "\n", 1);
	}
	printw("%c\n", c);
	goto write_answer;
}

int ccsqueryd_main(int argc, char *argv[])
{
	domain_policy_fd = open(proc_policy_domain_policy, O_WRONLY);
	int pipe_fd[2] = { EOF, EOF };
	if (argc == 1)
		goto ok;
	if (!strcmp(argv[1], "--no-update")) {
		check_update = GLOBALLY_READABLE_FILES_UPDATE_NONE;
		goto ok;
	}
	if (!strcmp(argv[1], "--ask-update")) {
		check_update = GLOBALLY_READABLE_FILES_UPDATE_ASK;
		goto ok;
	}
	printf("Usage: %s [--no-update|--ask-update]\n\n", argv[0]);
	printf("This program is used for granting access requests manually.\n");
	printf("This program shows access requests that are about to rejected "
	       "by the kernel's decision.\n");
	printf("If you answer before the kernel's decision taken effect, your "
	       "decision will take effect.\n");
	printf("You can use this program to respond to accidental access "
	       "requests triggered by non-routine tasks (such as restarting "
	       "daemons after updating).\n");
	printf("To terminate this program, use 'Ctrl-C'.\n");
	return 0;
 ok:
	query_fd = open(proc_policy_query, O_RDWR);
	if (query_fd == EOF) {
		fprintf(stderr,
			"You can't run this utility for this kernel.\n");
		return 1;
	} else if (write(query_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to %s to "
			"run this program.\n", proc_policy_manager);
		return 1;
	}
	if (check_update != GLOBALLY_READABLE_FILES_UPDATE_NONE) {
		pipe(pipe_fd);
		switch (fork()) {
		case 0:
			close(domain_policy_fd);
			close(query_fd);
			close(pipe_fd[0]);
			do_check_update(fdopen(pipe_fd[1], "w"));
			_exit(0);
		case -1:
			fprintf(stderr, "Can't fork().\n");
			return 1;
		}
		close(pipe_fd[1]);
		pipe_fd[1] = EOF;
	}
	readline_history = malloc(max_readline_history * sizeof(const char *));
	if (!readline_history)
		out_of_memory();
	write(query_fd, "\n", 1);
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	clear();
	refresh();
	scrollok(stdscr, TRUE);
	while (true) {
		static bool first = true;
		static unsigned int prev_serial = 0;
		fd_set rfds;
		unsigned int serial;
		char *cp;
		if (!buffer) {
			buffer = malloc(buffer_len);
			if (!buffer)
				break;
		}
		/* Wait for query. */
		FD_ZERO(&rfds);
		FD_SET(query_fd, &rfds);
		if (pipe_fd[0] != EOF)
			FD_SET(pipe_fd[0], &rfds);
		select(query_fd > pipe_fd[0] ? query_fd + 1 : pipe_fd[0] + 1,
		       &rfds, NULL, NULL, NULL);
		if (pipe_fd[0] != EOF && FD_ISSET(pipe_fd[0], &rfds))
			handle_update(pipe_fd[0]);
		if (!FD_ISSET(query_fd, &rfds))
			continue;

		/* Read query. */
		memset(buffer, 0, buffer_len);
		if (read(query_fd, buffer, buffer_len - 1) <= 0)
			continue;
		cp = strchr(buffer, '\n');
		if (!cp)
			continue;
		*cp = '\0';

		/* Get query number. */
		switch (sscanf(buffer, "Q%u-%hu", &serial, &retries)) {
		case 2:
			has_retry_counter = true;
			break;
		case 1:
			break;
		default:
			continue;
		}
		memmove(buffer, cp + 1, strlen(cp + 1) + 1);

		if (!first && prev_serial == serial && !has_retry_counter) {
			sleep(1);
			write(query_fd, "\n", 1);
			continue;
		}
		first = false;
		prev_serial = serial;
		timeout(1000);
		if (!strncmp(buffer, "#timestamp=", 11)) {
			if (handle_query_new_format(serial))
				continue;
		} else {
			if (handle_query_old_format(serial))
				continue;
		}
		break;
	}
	endwin();
	return 0;
}
