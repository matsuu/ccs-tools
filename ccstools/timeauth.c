/*
 * timeauth.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.5-pre   2008/10/23
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

struct element {
	unsigned int key;
	unsigned int interval;
	unsigned int min_interval;
	unsigned int max_interval;
};

#define PASSWORD_PROMPT           "PASSWORD_PROMPT "
#define MAX_TRIALS                "MAX_TRIALS "
#define SLEEP_ON_FAILURE          "SLEEP_ON_FAILURE "
#define MAX_TIMING_ERRORS         "MAX_TIMING_ERRORS "
#define EXEC_ON_SUCCESS           "EXEC_ON_SUCCESS "
#define AUTHENTICATION_DATA       "AUTHENTICATION_DATA "
#define SINGLE_FAILURE_MESSAGE    "SINGLE_FAILURE_MESSAGE "
#define COMPLETE_FAILURE_MESSAGE  "COMPLETE_FAILURE_MESSAGE "

static char *password_prompt = "Enter\\040password";
static unsigned int max_trials = 3;
static unsigned int sleep_on_failure = 3;
static unsigned int max_timing_errors = 0;
static char *exec_on_success = "/bin/sh";
static char *authentication_data = NULL;
static char *single_failure_message = "Incorrect\\040password.";
static char *complete_failure_message = "Authentication\\040failed.";

static struct element *authdata_list = NULL;
static int authdata_list_len = 0;

static void show_help(const char *argv0)
{
	char *self = canonicalize_file_name(argv0);
	fprintf(stderr, "This is an interpreter for "
		"password-with-timing-information authentication "
		"script. To perform password-with-timing-information "
		"authentication, create a program like the following "
		"example.\n\n");
	printf("#! %s\n"
	       PASSWORD_PROMPT          "%s\n"
	       MAX_TRIALS               "%d\n"
	       SLEEP_ON_FAILURE         "%d\n"
	       MAX_TIMING_ERRORS        "%d\n"
	       EXEC_ON_SUCCESS          "%s\n"
	       AUTHENTICATION_DATA      "1000-5000 p 1000-5000 a "
	       "1000-5000 s 1000-5000 s 1000-5000 w 1000-5000 o "
	       "1000-5000 r 1000-5000 d 1000-5000 \\012\n"
	       SINGLE_FAILURE_MESSAGE   "%s\n"
	       COMPLETE_FAILURE_MESSAGE "%s\n",
	       self, password_prompt, max_trials, sleep_on_failure,
	       max_timing_errors, exec_on_success, single_failure_message,
	       complete_failure_message);
	fprintf(stderr,
		"\n"
		PASSWORD_PROMPT "is a string to display before user's input.\n"
		MAX_TRIALS "is the maximal count the user can try.\n"
		SLEEP_ON_FAILURE "is the delay in second for each failure.\n"
		MAX_TIMING_ERRORS "is the acceptable limit for timing errors.\n"
		EXEC_ON_SUCCESS "is a program to execute when the "
		"authentication succeeded.\n"
		AUTHENTICATION_DATA "is an authentication data that are the "
		"repetition of minimal-interval and maximal-interval in "
		"milliseconds and character.\n"
		SINGLE_FAILURE_MESSAGE "is a string to display when the "
		"authentication failed for once.\n"
		COMPLETE_FAILURE_MESSAGE "is a string to display when the "
		"authentication failed for all trials.\n"
		"The above example requires the user to press 'password' + "
		"Enter with 1 second to 5 seconds interval for each key input."
		"\n\n"
		"Any character with value <= 0x20 or 0x7F "
		"<= values are regarded as a delimiter.\n"
		"You have to follow the character representation rule "
		"shown below.\n"
		"  ASCII character (0x21 <= value <= 0x5B or 0x5D <= "
		"value <= 7E).\n"
		"  \\\\ for \\ character (value = 0x5C).\n"
		"  \\ooo style octal representation (value = any).\n"
		"You can obtain " AUTHENTICATION_DATA "interactively by "
		"executing '%s --make'\n", self);
}

static void make_mode(const char *argv0)
{
	int trial;
	int pos;
	struct termios tp;
	struct termios tp0;
	tcgetattr(0, &tp0);
	tp = tp0;
	tp.c_lflag &= ~(ICANON | ECHO);
	tp.c_cc[VTIME] = 0;
	tp.c_cc[VMIN] = 1;
	fprintf(stderr, "You have choosen make mode, so I will "
		"generate script template for you.\n\n"
		"Before you start, you need to decide the password and "
		"acceptable interval.\n\n"
		"You need to type same password for three times. "
		"For the first time, type the password with acceptable "
		"slowest interval. "
		"For the second time, type the same password with "
		"acceptable fastest interval. "
		"The third time is for your practice to confirm you "
		"can type with interval between the slowest and the "
		"fastest.\n"
		"If you typed the same password for three times with "
		"appropriate interval, the script will be printed to "
		"stdout.\n\n");
 retry3:
	fprintf(stderr, "Press Enter to start.");
	getchar();
	trial = 0;
 retry:
	switch (trial) {
	case 0:
		fprintf(stderr, "RECORD %d: Enter password with acceptable "
			"slowest interval.\n", trial);
		break;
	case 1:
		fprintf(stderr, "RECORD %d: Enter the same password with "
			"acceptable fastest interval.\n", trial);
		break;
	default:
		fprintf(stderr, "VERIFY: Enter the same password with between "
			"slowest and fastest.\n");
		break;
	}
	pos = 0;
	tcsetattr(0, TCSANOW, &tp);
	while (1) {
		int key;
		int interval;
		struct timeval tv0;
		struct timeval tv1;
		struct timezone tz;
		gettimeofday(&tv0, &tz);
		key = getc(stdin);
		gettimeofday(&tv1, &tz);
		if (key != '\n')
			fputc('*', stderr);
		else
			fputc('\n', stderr);
		interval = (tv1.tv_sec - tv0.tv_sec) * 1000
			+ (tv1.tv_usec - tv0.tv_usec) / 1000;
		if (trial == 0) {
			authdata_list_len = pos + 1;
			authdata_list = (struct element *)
				realloc(authdata_list, sizeof(struct element)
					* authdata_list_len);
			if (!authdata_list) {
				tcsetattr(0, TCSANOW, &tp0);
				fprintf(stderr, "Out of memory.\n");
				exit(1);
			}
			authdata_list[pos].key = key;
		} else {
			if (pos >= authdata_list_len ||
			    authdata_list[pos].key != key)
				goto out;
		}
		if (trial == 0)
			authdata_list[pos++].max_interval = interval;
		else if (trial == 1)
			authdata_list[pos++].min_interval = interval;
		else
			authdata_list[pos++].interval = interval;
		if (key != '\n')
			continue;
		if (pos != authdata_list_len)
			goto out;
		if (trial < 2)
			break;
		for (pos = 0; pos < authdata_list_len; pos++) {
			if (authdata_list[pos].interval >=
			    authdata_list[pos].min_interval ||
			    authdata_list[pos].interval <=
			    authdata_list[pos].max_interval)
				continue;
			tcsetattr(0, TCSANOW, &tp0);
			if (0) {
				fprintf(stderr, "Oops! You didn't hit within "
					"acceptable speed. (%d<=%d<=%d) "
					"Restarting from the begininng.\n",
					authdata_list[pos].min_interval,
					authdata_list[pos].interval,
					authdata_list[pos].max_interval);
				goto retry3;
			} else {
				fprintf(stderr, "Oops! You didn't hit within "
					"acceptable speed. (%d<=%d<=%d) "
					"Incremented " MAX_TIMING_ERRORS ".\n",
					authdata_list[pos].min_interval,
					authdata_list[pos].interval,
					authdata_list[pos].max_interval);
				max_timing_errors++;
			}
		}
		break;
	}
	tcsetattr(0, TCSANOW, &tp0);
	if (trial < 2) {
		trial++;
		goto retry;
	}
	{
		int i;
		char *self = canonicalize_file_name(argv0);
		exec_on_success = getenv("SHELL");
		if (!exec_on_success)
			exec_on_success = "/bin/sh";
		fprintf(stderr, "Printing the content of "
			"authentication script.\n");
		printf("#! %s\n", self);
		printf(PASSWORD_PROMPT   "%s\n", password_prompt);
		printf(MAX_TRIALS        "%d\n", max_trials);
		printf(SLEEP_ON_FAILURE  "%d\n", sleep_on_failure);
		printf(MAX_TIMING_ERRORS "%d\n", max_timing_errors);
		printf(EXEC_ON_SUCCESS   "%s\n", exec_on_success);
		printf(AUTHENTICATION_DATA);
		for (i = 0; i < authdata_list_len; i++) {
			unsigned char c = authdata_list[i].key;
			printf("%u-%u ", authdata_list[i].min_interval,
			       authdata_list[i].max_interval);
			if (c == '\\')
				printf("\\\\ ");
			else if (c > 32 && c < 127)
				printf("%c ", c);
			else
				printf("\\%03o ", c);
		}
		printf("\n");
		printf(SINGLE_FAILURE_MESSAGE   "%s\n", single_failure_message);
		printf(COMPLETE_FAILURE_MESSAGE "%s\n",
		       complete_failure_message);
	}
	return;
 out:
	tcsetattr(0, TCSANOW, &tp0);
	fprintf(stderr, "Oops! You didn't hit the same key. Restarting this "
		"trial.\n");
	goto retry;
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
		else if (sscanf(buffer, MAX_TIMING_ERRORS "%u", &v) == 1)
			max_timing_errors = v;
		else if (str_starts(buffer, EXEC_ON_SUCCESS))
			exec_on_success = str_dup(buffer);
		else if (str_starts(buffer, AUTHENTICATION_DATA))
			authentication_data = str_dup(buffer);
		else if (str_starts(buffer, SINGLE_FAILURE_MESSAGE))
			single_failure_message = str_dup(buffer);
		else if (str_starts(buffer, COMPLETE_FAILURE_MESSAGE))
			complete_failure_message = str_dup(buffer);
	}
	fclose(fp);
	if (!authentication_data) {
		fprintf(stderr, "No authentication data found.\n");
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
	{
		char *cp = strtok(authentication_data, " ");
		while (cp) {
			unsigned int a;
			unsigned int b;
			unsigned int c;
			unsigned int v;
			if (sscanf(cp, "%u-%u", &a, &b) != 2)
				break;
			cp = strtok(NULL, " ");
			if (!cp)
				break;
			if (!strcmp(cp, "\\\\")) {
				c = '\\';
			} else if (sscanf(cp, "\\%o", &v) == 1) {
				c = v;
			} else {
				v = *(unsigned char *) cp;
				if (v >= 0x21 && v <= 0x7E && v != 0x5c)
					c = v;
				else
					break;
			}
			authdata_list = (struct element *)
				realloc(authdata_list, sizeof(struct element)
					* (authdata_list_len + 1));
			if (!authdata_list)
				break;
			authdata_list[authdata_list_len].min_interval = a;
			authdata_list[authdata_list_len].max_interval = b;
			authdata_list[authdata_list_len++].key = c;
			cp = strtok(NULL, " ");
		}
		if (!authdata_list) {
			fprintf(stderr, "No authentication data found.\n");
			return 1;
		}
		free(authentication_data);
		authentication_data = NULL;
	}
	return 0;
}

static int do_auth(void)
{
	int trial;
	struct termios tp;
	struct termios tp0;
	tcgetattr(0, &tp0);
	tp = tp0;
	tp.c_lflag &= ~(ICANON | ECHO);
	tp.c_cc[VTIME] = 0;
	tp.c_cc[VMIN] = 1;
	for (trial = 0; trial < max_trials; trial++) {
		int errors = 0;
		int pos = 0;
		int failed = 0;
		printf("%s: ", password_prompt);
		tcsetattr(0, TCSANOW, &tp);
		while (1) {
			int key;
			int interval;
			struct timeval tv0;
			struct timeval tv1;
			struct timezone tz;
			gettimeofday(&tv0, &tz);
			key = getc(stdin);
			gettimeofday(&tv1, &tz);
			if (key != '\n')
				putchar('*');
			else
				putchar('\n');
			interval = (tv1.tv_sec - tv0.tv_sec) * 1000
				+ (tv1.tv_usec - tv0.tv_usec) / 1000;
			if (!failed && pos < authdata_list_len) {
				if (authdata_list[pos].key != key) {
					failed = 1;
				} else if (interval <
					   authdata_list[pos].min_interval ||
					   interval >
					   authdata_list[pos].max_interval) {
					if (++errors > max_timing_errors)
						failed = 1;
				}
			} else {
				failed = 1;
			}
			pos++;
			if (key == '\n')
				break;
		}
		tcsetattr(0, TCSANOW, &tp0);
		if (pos == authdata_list_len && !failed) {
			execlp(exec_on_success, exec_on_success, NULL);
			fprintf(stderr, "Can't execute %s\n",
				exec_on_success);
			exit(1);
		}
		printf("%s\n", single_failure_message);
		sleep(sleep_on_failure);
	}
	printf("%s\n", complete_failure_message);
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		show_help(argv[0]);
		return 1;
	}
	if (!strcmp(argv[1], "--make")) {
		make_mode(argv[0]);
		return 0;
	}
	if (parse_script(argv[1]))
		return 1;
	return do_auth();
}
