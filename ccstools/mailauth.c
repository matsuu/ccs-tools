/*
 * mailauth.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.8   2009/05/28
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

static int str_starts(char *src, const char *find)
{
	const int len = strlen(find);
	if (strncmp(src, find, len))
		return 0;
	memmove(src, src + len, strlen(src + len) + 1);
	return 1;
}

static char *str_dup(const char *str)
{
	char *cp = strdup(str);
	if (!cp) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	return cp;
}

static void unescape_line(unsigned char *buffer)
{
	unsigned char *cp = buffer;
	unsigned char c;
	unsigned char d;
	unsigned char e;
	if (!cp)
		return;
	while (1) {
		c = *buffer++;
		if (!c)
			break;
		if (c != '\\') {
			*cp++ = c;
			continue;
		}
		c = *buffer++;
		if (c == '\\') {
			*cp++ = c;
			continue;
		}
		if (c < '0' || c > '3')
			break;
		d = *buffer++;
		if (d < '0' || d > '7')
			break;
		e = *buffer++;
		if (e < '0' || e > '7')
			break;
		*cp++ = ((c - '0') << 6) + ((d - '0') << 3) + (e - '0');
	}
	*cp = '\0';
}

static void normalize_line(unsigned char *buffer)
{
	unsigned char *sp = buffer;
	unsigned char *dp = buffer;
	int first = 1;
	while (*sp && (*sp <= 32 || 127 <= *sp))
		sp++;
	while (*sp) {
		if (!first)
			*dp++ = ' ';
		first = 0;
		while (32 < *sp && *sp < 127)
			*dp++ = *sp++;
		while (*sp && (*sp <= 32 || 127 <= *sp))
			sp++;
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

static char *password_prompt = "Enter\\040password";
static unsigned int max_trials = 3;
static unsigned int sleep_on_failure = 3;
static char *exec_on_success = "/bin/sh";
static char *mail_command = NULL;
static char *single_failure_message = "Incorrect\\040password.";
static char *complete_failure_message = "Authentication\\040failed.";

static void show_help(const char *argv0)
{
	char *self = canonicalize_file_name(argv0);
	fprintf(stderr,
		"This is an interpreter for one-time-password-using-mail "
		"authentication script. To perform "
		"one-time-password-using-mail authentication, create a program "
		"like the following example.\n\n");
	printf("#! %s\n"
	       PASSWORD_PROMPT          "%s\n"
	       MAX_TRIALS               "%d\n"
	       SLEEP_ON_FAILURE         "%d\n"
	       EXEC_ON_SUCCESS          "%s\n"
	       MAIL_COMMAND             "%s\n"
	       SINGLE_FAILURE_MESSAGE   "%s\n"
	       COMPLETE_FAILURE_MESSAGE "%s\n",
	       self,
	       password_prompt, max_trials, sleep_on_failure, exec_on_success,
	       "curl --data-binary @- https://your.server/path_to_cgi",
	       single_failure_message, complete_failure_message);
	fprintf(stderr,
		"\n"
		PASSWORD_PROMPT "is a string to display before user's input.\n"
		MAX_TRIALS "is the maximal count the user can try.\n"
		SLEEP_ON_FAILURE "is the delay in second for each failure.\n"
		EXEC_ON_SUCCESS "is a program to execute when the "
		"authentication succeeded.\n"
		MAIL_COMMAND "is the command line to notify password.\n"
		SINGLE_FAILURE_MESSAGE "is a string to display when "
		"the authentication failed for once.\n"
		COMPLETE_FAILURE_MESSAGE "is a string to display when "
		"the authentication failed for all trials.\n"
		"The above example sends password to "
		"https://your.server/path_to_cgi using curl.\n"
		"Any character with value <= 0x20 or 0x7F <= values "
		"are regarded as a delimiter.\n"
		"You have to follow the character representation rule shown "
		"below.\n"
		"  ASCII character (0x21 <= value <= 0x5B or 0x5D <= value "
		"<= 7E).\n"
		"  \\\\ for \\ character (value = 0x5C).\n"
		"  \\ooo style octal representation (value = any).\n"
		"The line "MAIL_COMMAND "is passed to system(), so "
		"escape appropriately as needed.\n");
}

static int parse_script(const char *argv1)
{
	char buffer[8192];
	FILE *fp = fopen(argv1, "r");
	if (!fp) {
		fprintf(stderr, "Can't open %s\n", argv1);
		return 1;
	}
	while (memset(buffer, 0, sizeof(buffer)),
	       fgets(buffer, sizeof(buffer) - 1, fp)) {
		char *cp = strchr(buffer, '\n');
		unsigned int v;
		if (*cp) {
			*cp = '\0';
		} else if (!feof(fp)) {
			fprintf(stderr, "Line too long.\n");
			fclose(fp);
			return 1;
		}
		normalize_line(buffer);
		if (str_starts(buffer, PASSWORD_PROMPT))
			password_prompt = str_dup(buffer);
		else if (sscanf(buffer, MAX_TRIALS "%u", &v) == 1)
			max_trials = v;
		else if (sscanf(buffer, SLEEP_ON_FAILURE "%u", &v) == 1)
			sleep_on_failure = v;
		else if (str_starts(buffer, EXEC_ON_SUCCESS))
			exec_on_success = str_dup(buffer);
		else if (str_starts(buffer, MAIL_COMMAND))
			mail_command = str_dup(buffer);
		else if (str_starts(buffer, SINGLE_FAILURE_MESSAGE))
			single_failure_message = str_dup(buffer);
		else if (str_starts(buffer, COMPLETE_FAILURE_MESSAGE))
			complete_failure_message = str_dup(buffer);
	}
	fclose(fp);
	if (!mail_command) {
		fprintf(stderr, "No mail command found.\n");
		return 1;
	}
	password_prompt = str_dup(password_prompt);
	unescape_line(password_prompt);
	exec_on_success = str_dup(exec_on_success);
	unescape_line(exec_on_success);
	single_failure_message = str_dup(single_failure_message);
	unescape_line(single_failure_message);
	complete_failure_message = str_dup(complete_failure_message);
	unescape_line(complete_failure_message);
	return 0;
}

static int do_auth(void)
{
	char password[17];
	char buffer[8192];
	{ /* Create password. */
		int i = 0;
		FILE *fp = fopen("/dev/urandom", "r");
		if (!fp) {
			fprintf(stderr, "Can't open /dev/urandom .\n");
			return 1;
		}
		memset(password, 0, sizeof(password));
		while (i < sizeof(password) - 1) {
			const unsigned int c = fgetc(fp);
			if (c < 10)
				password[i++] = c + '0';
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
	/* fprintf(stderr, "%s\n", password); */
	{ /* Check password. */
		int trial;
		for (trial = 0; trial < max_trials; trial++) {
			char *cp;
			printf("%s\n", password_prompt);
			memset(buffer, 0, sizeof(buffer));
			fgets(buffer, sizeof(buffer) - 1, stdin);
			cp = strchr(buffer, '\n');
			if (cp)
				*cp = '\0';
			if (!strcmp(buffer, password)) {
				execlp(exec_on_success, exec_on_success, NULL);
				fprintf(stderr, "Can't execute %s\n",
					exec_on_success);
				exit(1);
			}
			printf("%s\n", single_failure_message);
			sleep(sleep_on_failure);
		}
	}
	printf("%s\n", complete_failure_message);
	return 1;
}

int main(int argc, char *argv[])
{
	unsetenv("SHELLOPTS"); /* Make sure popen() executes MAIL_COMMAND */
	if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		show_help(argv[0]);
		return 1;
	}
	if (parse_script(argv[1]))
		return 1;
	return do_auth();
}
