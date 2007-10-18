/*
 * mailauth.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.1   2007/10/19
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>

static void UnEscapeLine(unsigned char *buffer) {
	unsigned char *cp = buffer;
	unsigned char c, d, e;
	if (!cp) return;
	while ((c = *buffer++) != '\0') {
		if (c != '\\') {
			*cp++ = c;
			continue;
		}
		if ((c = *buffer++) == '\\') {
			*cp++ = c;
			continue;
		}
		if (c < '0' || c > '3' ||
			(d = *buffer++) < '0' || d > '7' ||
			(e = *buffer++) < '0' || e > '7') {
			break;
		}
		*cp++ = ((c - '0') << 6) + ((d - '0') << 3) + (e - '0');
	}
	*cp = '\0';
}

static void NormalizeLine(unsigned char *buffer) {
	unsigned char *sp = buffer, *dp = buffer;
	int first = 1;
	while (*sp && (*sp <= 32 || 127 <= *sp)) sp++;
	while (*sp) {
		if (!first) *dp++ = ' ';
		first = 0;
		while (32 < *sp && *sp < 127) *dp++ = *sp++;
		while (*sp && (*sp <= 32 || 127 <= *sp)) sp++;
	}
	*dp = '\0';
}

#define PASSWORD_PROMPT          "PASSWORD_PROMPT "
#define MAX_TRIALS               "MAX_TRIALS "
#define SLEEP_ON_FAILURE         "SLEEP_ON_FAILURE "
#define EXEC_ON_SUCCESS          "EXEC_ON_SUCCESS "
#define MAIL_COMMAND             "MAIL_COMMAND "
#define SINGLE_FAILURE_MESSAGE   "SINGLE_FAILURE_MESSAGE "
#define COMPLETE_FAILURE_MESSAGE "COMPLETE_FAILURE_MESSAGE "

#define PASSWORD_SIZE 16

int main(int argc, char *argv[]) {
	static char buffer[8192];
	static char password[PASSWORD_SIZE];
	unsigned int max_trials = 3;
	unsigned int sleep_on_failure = 3;
	char *password_prompt = "Enter\\040password";
	char *exec_on_success = "/bin/sh";
	char *single_failure_message = "Incorrect\\040password.";
	char *complete_failure_message = "Authentication\\040failed.";
	char *mail_command = NULL;
	unsetenv("SHELLOPTS"); /* Make sure popen() executes MAIL_COMMAND */
	if (argc != 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		char *self = canonicalize_file_name(argv[0]);
		fprintf(stderr, "This is an interpreter for one-time-password-using-mail authentication script. To perform one-time-password-using-mail authentication, create a program like the following example.\n\n");
		printf("#! %s\n", self);
		printf(PASSWORD_PROMPT          "%s\n", password_prompt);
		printf(MAX_TRIALS               "%d\n", max_trials);
		printf(SLEEP_ON_FAILURE         "%d\n", sleep_on_failure);
		printf(EXEC_ON_SUCCESS          "%s\n", exec_on_success);
		printf(MAIL_COMMAND             "%s\n", "curl --data-binary @- https://your.server/path_to_cgi");
		printf(SINGLE_FAILURE_MESSAGE   "%s\n", single_failure_message);
		printf(COMPLETE_FAILURE_MESSAGE "%s\n", complete_failure_message);
		fprintf(stderr, "\n");
		fprintf(stderr, PASSWORD_PROMPT "is a string to display before user's input.\n");
		fprintf(stderr, MAX_TRIALS "is the maximal count the user can try.\n");
		fprintf(stderr, SLEEP_ON_FAILURE "is the delay in second for each failure.\n");
		fprintf(stderr, EXEC_ON_SUCCESS "is a program to execute when the authentication succeeded.\n");
		fprintf(stderr, MAIL_COMMAND "is the command line to notify password.\n");
		fprintf(stderr, SINGLE_FAILURE_MESSAGE "is a string to display when the authentication failed for once.\n");
		fprintf(stderr, COMPLETE_FAILURE_MESSAGE "is a string to display when the authentication failed for all trials.\n");
		fprintf(stderr, "The above example sends password to https://your.server/path_to_cgi using curl.\n");
		fprintf(stderr, "Any character with value <= 0x20 or 0x7F <= values are regarded as a delimiter.\n"
				"You have to follow the character representation rule shown below.\n"
				"  ASCII character (0x21 <= value <= 0x5B or 0x5D <= value <= 7E).\n"
				"  \\\\ for \\ character (value = 0x5C).\n"
				"  \\ooo style octal representation (value = any).\n");
		fprintf(stderr, "The line "MAIL_COMMAND "is passed to system(), so escape appropriately as needed.\n");
		return 1;
	}
	{
		FILE *fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "Can't open %s\n", argv[1]);
			return 1;
		}
		while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 1, fp)) {
			char *cp = strchr(buffer, '\n');
			unsigned int v;
			if (*cp) {
				*cp = '\0';
			} else if (!feof(fp)) {
				fprintf(stderr, "Line too long.\n");
				fclose(fp);
				return 1;
			}
			NormalizeLine(buffer);
			if (strncmp(buffer, PASSWORD_PROMPT, strlen(PASSWORD_PROMPT)) == 0) {
				password_prompt = strdup(buffer + strlen(PASSWORD_PROMPT));
			} else if (sscanf(buffer, MAX_TRIALS "%u", &v) == 1) {
				max_trials = v;
			} else if (sscanf(buffer, SLEEP_ON_FAILURE "%u", &v) == 1) {
				sleep_on_failure = v;
			} else if (strncmp(buffer, EXEC_ON_SUCCESS, strlen(EXEC_ON_SUCCESS)) == 0) {
				exec_on_success = strdup(buffer + strlen(EXEC_ON_SUCCESS));
			} else if (strncmp(buffer, MAIL_COMMAND, strlen(MAIL_COMMAND)) == 0) {
				mail_command = strdup(buffer + strlen(MAIL_COMMAND));
			} else if (strncmp(buffer, SINGLE_FAILURE_MESSAGE, strlen(SINGLE_FAILURE_MESSAGE)) == 0) {
				single_failure_message = strdup(buffer + strlen(SINGLE_FAILURE_MESSAGE));
			} else if (strncmp(buffer, COMPLETE_FAILURE_MESSAGE, strlen(COMPLETE_FAILURE_MESSAGE)) == 0) {
				complete_failure_message = strdup(buffer + strlen(COMPLETE_FAILURE_MESSAGE));
			}
		}
		fclose(fp);
	}
	if (!mail_command) {
		fprintf(stderr, "No mail command found.\n");
		return 1;
	}
	password_prompt = strdup(password_prompt);
	UnEscapeLine(password_prompt);
	exec_on_success = strdup(exec_on_success);
	UnEscapeLine(exec_on_success);
	single_failure_message = strdup(single_failure_message);
	UnEscapeLine(single_failure_message);
	complete_failure_message = strdup(complete_failure_message);
	UnEscapeLine(complete_failure_message);
	
	{ /* Create password. */
		int i = 0;
		FILE *fp = fopen("/dev/urandom", "r");
		if (!fp) {
			fprintf(stderr, "Can't open /dev/urandom .\n");
			return 1;
		}
		memset(password, 0, sizeof(password));
		while (i < sizeof(password)) {
			const unsigned int c = fgetc(fp);
			if (c < 10) password[i++] = c + '0';
		}
		fclose(fp);
	}
	{ /* Send password. */
		FILE *fp = popen(mail_command, "w");
		if (!fp) {
			fprintf(stderr, "Can't send mail\n");
			return 1;
		}
		fprintf(fp, "%s\n", password);
		pclose(fp);
	} 
	//fprintf(stderr, "%s\n", password);
	{ /* Check password. */
		int trial;
		for (trial = 0; trial < max_trials; trial++) {
			char *cp;
			printf("%s\n", password_prompt);
			memset(buffer, 0, sizeof(buffer));
			fgets(buffer, sizeof(buffer) - 1, stdin);
			if ((cp = strchr(buffer, '\n')) != NULL) *cp = '\0';
			if (strcmp(buffer, password) == 0) {
				execlp(exec_on_success, exec_on_success, NULL);
				fprintf(stderr, "Can't execute %s\n", exec_on_success);
				exit(1);
			}
			printf("%s\n", single_failure_message);
			sleep(sleep_on_failure);
		}
	}
	printf("%s\n", complete_failure_message);
	return 0;
}
