/*
 * ccs-queryd.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0   2008/04/01
 *
 */
#include "ccstools.h"

extern int query_fd;
extern char *initial_readline_data;

int ccsqueryd_main(int argc, char *argv[]) {
	const int domain_policy_fd = open(proc_policy_domain_policy, O_WRONLY);
	static const int max_readline_history = 20;
	static const char **readline_history = NULL;
	static int readline_history_count = 0;
	if (argc > 1) {
		printf("Usage: %s\n\n", argv[0]);
		printf("This program is used for granting access requests manually.\n");
		printf("This program shows access requests that are about to rejected by the kernel's decision.\n");
		printf("If you answer before the kernel's decision taken effect, your decision will take effect.\n");
		printf("You can use this program to respond to accidental access requests triggered by non-routine tasks (such as restarting daemons after updating).\n");
		printf("To terminate this program, use 'Ctrl-C'.\n");
		return 0;
	}
	query_fd = open(proc_policy_query, O_RDWR);
	if (query_fd == EOF) {
		fprintf(stderr, "You can't run this utility for this kernel.\n");
		return 1;
	} else if (write(query_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to %s to run this program.\n", proc_policy_manager);
		return 1;
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
		static const int buffer_len = 16384;
		static char *buffer = NULL, *prev_buffer = NULL;
		fd_set rfds;
		unsigned int serial;
		char *cp;
		if (!buffer && (buffer = malloc(buffer_len)) == NULL) break;
		if (!prev_buffer) {
			if ((prev_buffer = malloc(buffer_len)) == NULL) break;
			memset(prev_buffer, 0, buffer_len);
		}
		// Wait for query.
		FD_ZERO(&rfds);
		FD_SET(query_fd, &rfds);
		select(query_fd + 1, &rfds, NULL, NULL, NULL);
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
