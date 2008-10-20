/*
 * mailauth.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5-pre   2008/10/20
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

static void UnEscapeLine(unsigned char *buffer)
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

static void NormalizeLine(unsigned char *buffer)
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

#define PASSWORD_SIZE 16

struct authinfo {
	char password[PASSWORD_SIZE];
	unsigned int max_trials;
	unsigned int sleep_on_failure;
	char *password_prompt;
	char *exec_on_success;
	char *single_failure_message;
	char *complete_failure_message;
	char *mail_command;
};

static void show_help(struct authinfo *ai, const char *argv0)
{
	char *self = canonicalize_file_name(argv0);
	fprintf(stderr, "This is an interpreter for "
		"one-time-password-using-mail authentication script. To "
		"perform one-time-password-using-mail authentication, create a "
		"program like the following example.\n\n");
	printf("#! %s\n", self);
	printf(PASSWORD_PROMPT          "%s\n", ai->password_prompt);
	printf(MAX_TRIALS               "%d\n", ai->max_trials);
	printf(SLEEP_ON_FAILURE         "%d\n", ai->sleep_on_failure);
	printf(EXEC_ON_SUCCESS          "%s\n", ai->exec_on_success);
	printf(MAIL_COMMAND             "%s\n", "curl --data-binary @- "
	       "https://your.server/path_to_cgi");
	printf(SINGLE_FAILURE_MESSAGE   "%s\n", ai->single_failure_message);
	printf(COMPLETE_FAILURE_MESSAGE "%s\n", ai->complete_failure_message);
	fprintf(stderr, "\n");
	fprintf(stderr, PASSWORD_PROMPT "is a string to display before user's "
		"input.\n");
	fprintf(stderr, MAX_TRIALS "is the maximal count the user can try.\n");
	fprintf(stderr, SLEEP_ON_FAILURE "is the delay in second for each "
		"failure.\n");
	fprintf(stderr, EXEC_ON_SUCCESS "is a program to execute when the "
		"authentication succeeded.\n");
	fprintf(stderr, MAIL_COMMAND "is the command line to notify "
		"password.\n");
	fprintf(stderr, SINGLE_FAILURE_MESSAGE "is a string to display when "
		"the authentication failed for once.\n");
	fprintf(stderr, COMPLETE_FAILURE_MESSAGE "is a string to display when "
		"the authentication failed for all trials.\n");
	fprintf(stderr, "The above example sends password to "
		"https://your.server/path_to_cgi using curl.\n");
	fprintf(stderr, "Any character with value <= 0x20 or 0x7F <= values "
		"are regarded as a delimiter.\n"
		"You have to follow the character representation rule shown "
		"below.\n"
		"  ASCII character (0x21 <= value <= 0x5B or 0x5D <= value "
		"<= 7E).\n"
		"  \\\\ for \\ character (value = 0x5C).\n"
		"  \\ooo style octal representation (value = any).\n");
	fprintf(stderr, "The line "MAIL_COMMAND "is passed to system(), so "
		"escape appropriately as needed.\n");
}

static int parse_script(struct authinfo *ai, const char *argv1)
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
		NormalizeLine(buffer);
		if (str_starts(buffer, PASSWORD_PROMPT))
			ai->password_prompt = str_dup(buffer);
		else if (sscanf(buffer, MAX_TRIALS "%u", &v) == 1)
			ai->max_trials = v;
		else if (sscanf(buffer, SLEEP_ON_FAILURE "%u", &v) == 1)
			ai->sleep_on_failure = v;
		else if (str_starts(buffer, EXEC_ON_SUCCESS))
			ai->exec_on_success = str_dup(buffer);
		else if (str_starts(buffer, MAIL_COMMAND))
			ai->mail_command = str_dup(buffer);
		else if (str_starts(buffer, SINGLE_FAILURE_MESSAGE))
			ai->single_failure_message = str_dup(buffer);
		else if (str_starts(buffer, COMPLETE_FAILURE_MESSAGE))
			ai->complete_failure_message = str_dup(buffer);
	}
	fclose(fp);
	if (!ai->mail_command) {
		fprintf(stderr, "No mail command found.\n");
		return 1;
	}
	ai->password_prompt = str_dup(ai->password_prompt);
	UnEscapeLine(ai->password_prompt);
	ai->exec_on_success = str_dup(ai->exec_on_success);
	UnEscapeLine(ai->exec_on_success);
	ai->single_failure_message = str_dup(ai->single_failure_message);
	UnEscapeLine(ai->single_failure_message);
	ai->complete_failure_message = str_dup(ai->complete_failure_message);
	UnEscapeLine(ai->complete_failure_message);
	return 0;
}

static int do_auth(struct authinfo *ai)
{
	const int max_trials = ai->max_trials;
	const char *password_prompt = ai->password_prompt;
	const char *exec_on_success = ai->exec_on_success;
	const char *single_failure_message = ai->single_failure_message;
	const int sleep_on_failure = ai->sleep_on_failure;
	const char *mail_command = ai->mail_command;
	char password[PASSWORD_SIZE];
	char buffer[8192];
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
	printf("%s\n", ai->complete_failure_message);
	return 1;
}

int main(int argc, char *argv[])
{
	struct authinfo ai;
	memset(&ai, 0, sizeof(ai));
	ai.max_trials = 3;
	ai.sleep_on_failure = 3;
	ai.password_prompt = "Enter\\040password";
	ai.exec_on_success = "/bin/sh";
	ai.single_failure_message = "Incorrect\\040password.";
	ai.complete_failure_message = "Authentication\\040failed.";
	unsetenv("SHELLOPTS"); /* Make sure popen() executes MAIL_COMMAND */
	if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		show_help(&ai, argv[0]);
		return 1;
	}
	if (parse_script(&ai, argv[1]))
		return 1;
	return do_auth(&ai);
}
