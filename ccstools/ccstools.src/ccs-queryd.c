/*
 * ccs-queryd.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.2-pre   2008/06/23
 *
 */
#include "ccstools.h"

static void do_check_update(FILE *fp_out) {
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
		if (!cp) break;
		*cp = '\0';
		if (strncmp(buffer, KEYWORD_ALLOW_READ, KEYWORD_ALLOW_READ_LEN)) continue;
		if (!decode(buffer + KEYWORD_ALLOW_READ_LEN, buffer)) continue;
		pathnames = (char **) realloc(pathnames, sizeof(char *) * (pathnames_len + 1));
		if (!pathnames) return;
		pathnames[pathnames_len] = strdup(buffer);
		if (!pathnames[pathnames_len]) return;
		pathnames_len++;
	}
	fclose(fp_in);
	while (1) {
		char buffer[16384];
		struct stat64 buf;
		static time_t last_modified = 0;
		int i;
		fflush(fp_out);
		sleep(1);
		for (i = 0; i < pathnames_len; i++) {
			int j;
			if (stat64(pathnames[i], &buf) == 0) continue;
			fprintf(fp_out, "-");
			fprintf_encoded(fp_out, pathnames[i]);
			fprintf(fp_out, "\n");
			free(pathnames[i]);
			pathnames_len--;
			for (j = i; j < pathnames_len; j++) pathnames[j] = pathnames[j + 1];
			i--;
		}
		if (stat64("/etc/ld.so.cache", &buf) || buf.st_mtime == last_modified) continue;
		fp_in = popen("/sbin/ldconfig -NXp", "r");
		if (!fp_in) continue;
		last_modified = buf.st_mtime;
		memset(buffer, 0, sizeof(buffer));
		while (fgets(buffer, sizeof(buffer) - 1, fp_in)) {
			char *cp = strchr(buffer, '\n');
			char *real_pathname;
			if (!cp) break;
			*cp = '\0';
			cp = strrchr(buffer, ' ');
			if (!cp || *++cp != '/') continue;
			if (stat64(cp, &buf)) continue;
			if ((real_pathname = realpath(cp, NULL)) == NULL) continue;
			for (i = 0; i < pathnames_len; i++) {
				if (!strcmp(real_pathname, pathnames[i])) break;
			}
			if (i == pathnames_len) {
				pathnames = (char **) realloc(pathnames, sizeof(char *) * (pathnames_len + 1));
				if (!pathnames) return;
				pathnames[pathnames_len] = strdup(real_pathname);
				if (!pathnames[pathnames_len]) return;
				pathnames_len++;
				fprintf(fp_out, "+");
				fprintf_encoded(fp_out, pathnames[i]);
				fprintf(fp_out, "\n");
			}
			free(real_pathname);
		}
		pclose(fp_in);
	}
}

extern int query_fd;
extern char *initial_readline_data;

#define GLOBALLY_READABLE_FILES_UPDATE_NONE 0
#define GLOBALLY_READABLE_FILES_UPDATE_ASK  1
#define GLOBALLY_READABLE_FILES_UPDATE_AUTO 2

static int check_update = GLOBALLY_READABLE_FILES_UPDATE_AUTO;

static void handle_update(const int fd) {
	static FILE *fp = NULL;
	static char pathname[8192];
	static int pathname_len = 0;
	int c;
	if (!fp) fp = fopen(proc_policy_exception_policy, "w");
	if (!pathname_len) memset(pathname, 0, sizeof(pathname));
	while (read(fd, pathname + pathname_len, 1) == 1 && pathname[pathname_len] != '\n' && pathname_len < sizeof(pathname) - 1) pathname_len++;
	pathname[pathname_len] = '\0'; pathname_len = 0;
	if (check_update == GLOBALLY_READABLE_FILES_UPDATE_AUTO) {
		if (pathname[0] == '-') fprintf(fp, KEYWORD_DELETE);
		fprintf(fp, KEYWORD_ALLOW_READ "%s\n", pathname + 1);
		fflush(fp);
		printw("The pathname %s was %s globally readable file.\n\n", pathname + 1, (pathname[0] == '-') ? "deleted. Deleted from" : "created. Appended to");
		refresh();
		return;
	}
	printw("The pathname %s was %s globally readable file? ('Y'es/'N'o):", pathname + 1, (pathname[0] == '-') ? "deleted. Delete from" : "created. Append to");
	refresh();
	while (1) {
		c = getch2();
		if (c == 'Y' || c == 'y' || c == 'N' || c == 'n') break;
		write(query_fd, "\n", 1);
	}
	printw("%c\n", c); refresh();
	if (c == 'Y' || c == 'y') {
		if (pathname[0] == '-') fprintf(fp, KEYWORD_DELETE);
		fprintf(fp, KEYWORD_ALLOW_READ "%s\n", pathname + 1);
		fflush(fp);
	}
	printw("\n"); refresh();
}

int ccsqueryd_main(int argc, char *argv[]) {
	const int domain_policy_fd = open(proc_policy_domain_policy, O_WRONLY);
	static const int max_readline_history = 20;
	static const char **readline_history = NULL;
	static int readline_history_count = 0;
	int pipe_fd[2] = { EOF, EOF };
	if (argc == 1) goto ok;
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
	printf("This program shows access requests that are about to rejected by the kernel's decision.\n");
	printf("If you answer before the kernel's decision taken effect, your decision will take effect.\n");
	printf("You can use this program to respond to accidental access requests triggered by non-routine tasks (such as restarting daemons after updating).\n");
	printf("To terminate this program, use 'Ctrl-C'.\n");
	return 0;
 ok:
	query_fd = open(proc_policy_query, O_RDWR);
	if (query_fd == EOF) {
		fprintf(stderr, "You can't run this utility for this kernel.\n");
		return 1;
	} else if (write(query_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to %s to run this program.\n", proc_policy_manager);
		return 1;
	}
	if (check_update) {
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
		close(pipe_fd[1]); pipe_fd[1] = EOF;
	}
	readline_history = malloc(max_readline_history * sizeof(const char *));
	write(query_fd, "\n", 1);
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	clear(); refresh();
	scrollok(stdscr, 1);
	while (1) {
		static int first = 1;
		static unsigned int prev_serial = 0;
		static const int buffer_len = 32768;
		static char *buffer = NULL, *prev_buffer = NULL;
		fd_set rfds;
		unsigned int serial;
		char *cp;
		if (!buffer && (buffer = malloc(buffer_len)) == NULL) break;
		// Wait for query.
		FD_ZERO(&rfds);
		FD_SET(query_fd, &rfds);
		if (pipe_fd[0] != EOF) FD_SET(pipe_fd[0], &rfds);
		select(query_fd > pipe_fd[0] ? query_fd + 1 : pipe_fd[0] + 1, &rfds, NULL, NULL, NULL);
		if (pipe_fd[0] != EOF && FD_ISSET(pipe_fd[0], &rfds)) handle_update(pipe_fd[0]);
		if (!FD_ISSET(query_fd, &rfds)) continue;
		
		// Read query.
		memset(buffer, 0, buffer_len);
		if (read(query_fd, buffer, buffer_len - 1) <= 0) continue;
		if ((cp = strchr(buffer, '\n')) == NULL) continue;
		*cp = '\0';
		
		// Get query number.
		if (sscanf(buffer, "Q%u", &serial) != 1) continue;
		memmove(buffer, cp + 1, strlen(cp + 1) + 1);
		
		if (!first && prev_serial == serial) {
			sleep(1);
			write(query_fd, "\n", 1);
			continue;
		}
		first = 0;
		prev_serial = serial;
		timeout(1000);
		if (strncmp(buffer, "#timestamp=", 11) == 0) {
			/* New format. */
			static unsigned int prev_pid = 0;
			unsigned int pid;
			time_t stamp;
			char *cp = strstr(buffer, " pid=");
			if (!cp || sscanf(cp + 5, "%u", &pid) != 1) {
				printw("ERROR: Unsupported query.\n");
				break;
			}
			cp = strchr(buffer, '\0');
			if (*(cp - 1) != '\n') {
				printw("ERROR: Unsupported query.\n");
				break;
			}
			*(cp - 1) = '\0';
			if (pid != prev_pid) {
				if (prev_pid) printw("----------------------------------------\n");
				prev_pid = pid;
			}
			if (sscanf(buffer, "#timestamp=%lu", &stamp) == 1 && (cp = strchr(buffer, ' ')) != NULL) {
				struct tm *tm = localtime(&stamp);
				printw("#%04d-%02d-%02d %02d:%02d:%02d#", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
				memmove(buffer, cp, strlen(cp) + 1);
			}
			printw("%s\n", buffer);
			// Is this domain query?
			if (!strstr(buffer, "\n#")) {
				int c = 0;
				printw("Allow? ('Y'es/Yes and 'A'ppend to policy/'N'o):"); refresh();
				while (1) {
					c = getch2();
					if (c == 'Y' || c == 'y' || c == 'N' || c == 'n' || c == 'A' || c == 'a') break;
					write(query_fd, "\n", 1);
				}
				printw("%c\n", c); refresh();
				
				// Append to domain policy.
				if (c == 'A' || c == 'a') {
					int y, x;
					char *line;
					getyx(stdscr, y, x);
					cp = strrchr(buffer, '\n');
					if (!cp)
						break;
					*cp++ = '\0';
					initial_readline_data = cp;
					readline_history_count = simple_add_history(cp, readline_history, readline_history_count, max_readline_history);
					line = simple_readline(y, 0, "Enter new entry> ", readline_history, readline_history_count, 4000, 8);
					scrollok(stdscr, 1);
					printw("\n"); refresh();
					if (line && *line) {
						readline_history_count = simple_add_history(line, readline_history, readline_history_count, max_readline_history);
						write(domain_policy_fd, buffer, strlen(buffer));
						write(domain_policy_fd, "\n", 1);
						write(domain_policy_fd, line, strlen(line));
						write(domain_policy_fd, "\n", 1);
						printw("Added '%s'.\n", line);
					} else {
						printw("None added.\n", line);
					}
					refresh();
					free(line);
				}

				// Write answer.
				snprintf(buffer, buffer_len - 1, "A%u=%u\n", serial, c == 'Y' || c == 'y' || c == 'A' || c == 'a' ? 1 : 2);
				write(query_fd, buffer, strlen(buffer));
			} else {
				int c;
				printw("Allow? ('Y'es/'N'o):"); refresh();
				while (1) {
					c = getch2();
					if (c == 'Y' || c == 'y' || c == 'N' || c == 'n') break;
					write(query_fd, "\n", 1);
				}
				printw("%c\n", c); refresh();
				
				// Write answer.
				snprintf(buffer, buffer_len - 1, "A%u=%u\n", serial, c == 'Y' || c == 'y' ? 1 : 2);
				write(query_fd, buffer, strlen(buffer));
			}
			printw("\n"); refresh();
			continue;
		}
		
		if (!prev_buffer) {
			if ((prev_buffer = malloc(buffer_len)) == NULL) break;
			memset(prev_buffer, 0, buffer_len);
		}
		// Is this domain query?
		if (strncmp(buffer, "<kernel>", 8) == 0 && (buffer[8] == '\0' || buffer[8] == ' ') && (cp = strchr(buffer, '\n')) != NULL) {
			int c = 0;
			// Check for same domain.
			*cp++ = '\0';
			if (strcmp(buffer, prev_buffer)) {
				printw("----------------------------------------\n");
				memmove(prev_buffer, buffer, strlen(buffer) + 1);
			}
			printw("%s\n", buffer);
			printw("%s", cp);
			printw("Allow? ('Y'es/Yes and 'A'ppend to policy/'N'o):"); refresh();
			while (1) {
				c = getch2();
				if (c == 'Y' || c == 'y' || c == 'N' || c == 'n' || c == 'A' || c == 'a') break;
				write(query_fd, "\n", 1);
			}
			printw("%c\n", c); refresh();
			
			// Append to domain policy.
			if (c == 'A' || c == 'a') {
				int y, x;
				char *line;
				getyx(stdscr, y, x);
				if ((line = strchr(cp, '\n')) != NULL) *line = '\0';
				initial_readline_data = cp;
				readline_history_count = simple_add_history(cp, readline_history, readline_history_count, max_readline_history);
				line = simple_readline(y, 0, "Enter new entry> ", readline_history, readline_history_count, 4000, 8);
				scrollok(stdscr, 1);
				printw("\n"); refresh();
				if (line && *line) {
					readline_history_count = simple_add_history(line, readline_history, readline_history_count, max_readline_history);
					write(domain_policy_fd, buffer, strlen(buffer));
					write(domain_policy_fd, "\n", 1);
					write(domain_policy_fd, line, strlen(line));
					write(domain_policy_fd, "\n", 1);
					printw("Added '%s'.\n", line);
				} else {
					printw("None added.\n", line);
				}
				refresh();
				free(line);
			}

			// Write answer.
			snprintf(buffer, buffer_len - 1, "A%u=%u\n", serial, c == 'Y' || c == 'y' || c == 'A' || c == 'a' ? 1 : 2);
			write(query_fd, buffer, strlen(buffer));
		} else {
			int c;
			printw("----------------------------------------\n");
			prev_buffer[0] = '\0';
			printw("%s", buffer);
			printw("Allow? ('Y'es/'N'o):"); refresh();
			while (1) {
				c = getch2();
				if (c == 'Y' || c == 'y' || c == 'N' || c == 'n') break;
				write(query_fd, "\n", 1);
			}
			printw("%c\n", c); refresh();
			
			// Write answer.
			snprintf(buffer, buffer_len - 1, "A%u=%u\n", serial, c == 'Y' || c == 'y' ? 1 : 2);
			write(query_fd, buffer, strlen(buffer));
		}
		printw("\n"); refresh();
	}
	endwin();
	return 0;
}
