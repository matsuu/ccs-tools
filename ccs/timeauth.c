/*
 * timeauth.c
 *
 * An example program for CERBERUS.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.3   2006/11/11
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

#define PASSWORD_PROMPT           "PASSWORD_PROMPT "
#define MAX_TRIALS                "MAX_TRIALS "
#define SLEEP_ON_FAILURE          "SLEEP_ON_FAILURE "
#define MAX_TIMING_ERRORS         "MAX_TIMING_ERRORS "
#define EXEC_ON_SUCCESS           "EXEC_ON_SUCCESS "
#define AUTHENTICATION_DATA       "AUTHENTICATION_DATA "
#define SINGLE_FAILURE_MESSAGE    "SINGLE_FAILURE_MESSAGE "
#define COMPLETE_FAILURE_MESSAGE  "COMPLETE_FAILURE_MESSAGE "

typedef struct {
	unsigned int key;
	unsigned int interval;
	unsigned int min_interval;
	unsigned int max_interval;
} ELEMENT;

int main(int argc, char *argv[]) {
	static char buffer[8192];
	ELEMENT *authdata_list = NULL;
	int authdata_list_len = 0;
	unsigned int max_trials = 3;
	unsigned int sleep_on_failure = 3;
	unsigned int max_timing_errors = 0;
	char *password_prompt = "Enter\\040password";
	char *exec_on_success = "/bin/sh";
	char *single_failure_message = "Incorrect\\040password.";
	char *complete_failure_message = "Authentication\\040failed.";
	char *authentication_data = NULL;
	if (argc != 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		char *self = canonicalize_file_name(argv[0]);
		fprintf(stderr, "This is an interpreter for password-with-timing-information authentication script. To perform password-with-timing-information authentication, create a program like the following example.\n\n");
		printf("#! %s\n", self);
		printf(PASSWORD_PROMPT          "%s\n", password_prompt);
		printf(MAX_TRIALS               "%d\n", max_trials);
		printf(SLEEP_ON_FAILURE         "%d\n", sleep_on_failure);
		printf(MAX_TIMING_ERRORS        "%d\n", max_timing_errors);
		printf(EXEC_ON_SUCCESS          "%s\n", exec_on_success);
		printf(AUTHENTICATION_DATA      "1000-5000 p 1000-5000 a 1000-5000 s 1000-5000 s 1000-5000 w 1000-5000 o 1000-5000 r 1000-5000 d 1000-5000 \\012\n");
		printf(SINGLE_FAILURE_MESSAGE   "%s\n", single_failure_message);
		printf(COMPLETE_FAILURE_MESSAGE "%s\n", complete_failure_message);
		fprintf(stderr, "\n");
		fprintf(stderr, PASSWORD_PROMPT "is a string to display before user's input.\n");
		fprintf(stderr, MAX_TRIALS "is the maximal count the user can try.\n");
		fprintf(stderr, SLEEP_ON_FAILURE "is the delay in second for each failure.\n");
		fprintf(stderr, MAX_TIMING_ERRORS "is the acceptable limit for timing errors.\n");
		fprintf(stderr, EXEC_ON_SUCCESS "is a program to execute when the authentication succeeded.\n");
		fprintf(stderr, AUTHENTICATION_DATA "is an authentication data that are the repetition of minimal-interval "
				"and maximal-interval in milliseconds and character.\n");
		fprintf(stderr, SINGLE_FAILURE_MESSAGE "is a string to display when the authentication failed for once.\n");
		fprintf(stderr, COMPLETE_FAILURE_MESSAGE "is a string to display when the authentication failed for all trials.\n");
		fprintf(stderr, "The above example requires the user to press 'password' + Enter "
				"with 1 second to 5 seconds interval for each key input.\n\n");
		fprintf(stderr, "Any character with value <= 0x20 or 0x7F <= values are regarded as a delimiter.\n"
				"You have to follow the character representation rule shown below.\n"
				"  ASCII character (0x21 <= value <= 0x5B or 0x5D <= value <= 7E).\n"
				"  \\\\ for \\ character (value = 0x5C).\n"
				"  \\ooo style octal representation (value = any).\n");
		fprintf(stderr, "You can obtain " AUTHENTICATION_DATA "interactively by executing '%s --make'\n", self);
		return 1;
	}
	if (strcmp(argv[1], "--make") == 0) {
		int trial;
		struct termios tp, tp0;
		tcgetattr(0, &tp0); tp = tp0;
		tp.c_lflag &= ~(ICANON | ECHO);
		tp.c_cc[VTIME] = 0;
		tp.c_cc[VMIN] = 1;
		fprintf(stderr, "You have choosen make mode, so I will generate script template for you.\n\n"
				"Before you start, you need to decide the password and acceptable interval.\n\n"
				"You need to type same password for three times. "
				"For the first time, type the password with acceptable slowest interval. "
				"For the second time, type the same password with acceptable fastest interval. "
				"The third time is for your practice to confirm you can type with interval between the slowest and the fastest.\n"
				"If you typed the same password for three times with appropriate interval, the script will be printed to stdout.\n\n");
	retry3: ;
		fprintf(stderr, "Press Enter to start.");
		getchar();
		for (trial = 0; trial < 3; trial++) {
			int pos;
		retry: ;
			switch (trial) {
			case 0:
				fprintf(stderr, "RECORD %d: Enter password with acceptable slowest interval.\n", trial);
				break;
			case 1:
				fprintf(stderr, "RECORD %d: Enter the same password with acceptable fastest interval.\n", trial);
				break;
			default:
				fprintf(stderr, "VERIFY: Enter the same password with between slowest and fastest.\n");
				break;
			}
			pos = 0;
			tcsetattr(0, TCSANOW, &tp);
			while (1) {
				int key, interval;
				struct timeval tv0, tv1;
				struct timezone tz;
				gettimeofday(&tv0, &tz);
				key = getc(stdin);
				gettimeofday(&tv1, &tz);
				if (key != '\n') fputc('*', stderr); else fputc('\n', stderr);
				interval = (tv1.tv_sec - tv0.tv_sec) * 1000 + (tv1.tv_usec - tv0.tv_usec) / 1000;
				if (trial == 0) {
					authdata_list_len = pos + 1;
					if ((authdata_list = (ELEMENT *) realloc(authdata_list, sizeof(ELEMENT) * authdata_list_len)) == NULL) {
						tcsetattr(0, TCSANOW, &tp0);
						fprintf(stderr, "Out of memory.\n");
						exit(1);
					}
					authdata_list[pos].key = key;
				} else {
					if (pos >= authdata_list_len || authdata_list[pos].key != key) {
					retry2: ;
						tcsetattr(0, TCSANOW, &tp0);
						fprintf(stderr, "Oops! You didn't hit the same key. Restarting this trial.\n");
						goto retry;
					}
				}
				if (trial == 0) authdata_list[pos++].max_interval = interval;
				else if (trial == 1) authdata_list[pos++].min_interval = interval;
				else authdata_list[pos++].interval = interval;
				if (key == '\n') {
					if (pos != authdata_list_len) goto retry2;
					if (trial == 2) {
						for (pos = 0; pos < authdata_list_len; pos++) {
							if (authdata_list[pos].interval < authdata_list[pos].min_interval || authdata_list[pos].interval > authdata_list[pos].max_interval) {
								tcsetattr(0, TCSANOW, &tp0);
								if (0) {
									fprintf(stderr, "Oops! You didn't hit within acceptable speed. (%d<=%d<=%d) Restarting from the begininng.\n", authdata_list[pos].min_interval, authdata_list[pos].interval, authdata_list[pos].max_interval);
									goto retry3;
								} else {
									fprintf(stderr, "Oops! You didn't hit within acceptable speed. (%d<=%d<=%d) Incremented " MAX_TIMING_ERRORS ".\n", authdata_list[pos].min_interval, authdata_list[pos].interval, authdata_list[pos].max_interval);
									max_timing_errors++;
								}
							}
						}
					}
					break;
				}
			}
			tcsetattr(0, TCSANOW, &tp0);
		}
		{
			int i;
			char *self = canonicalize_file_name(argv[0]);
			exec_on_success = getenv("SHELL");
			if (!exec_on_success) exec_on_success = "/bin/sh";
			fprintf(stderr, "Printing the content of authentication script.\n");
			printf("#! %s\n", self);
			printf(PASSWORD_PROMPT   "%s\n", password_prompt);
			printf(MAX_TRIALS        "%d\n", max_trials);
			printf(SLEEP_ON_FAILURE  "%d\n", sleep_on_failure);
			printf(MAX_TIMING_ERRORS "%d\n", max_timing_errors);
			printf(EXEC_ON_SUCCESS   "%s\n", exec_on_success);
			printf(AUTHENTICATION_DATA);
			for (i = 0; i < authdata_list_len; i++) {
				unsigned char c = authdata_list[i].key;
				printf("%u-%u ", authdata_list[i].min_interval, authdata_list[i].max_interval);
				if (c == '\\') printf("\\\\ ");
				else if (c > 32 && c < 127) printf("%c ", c);
				else printf("\\%03o ", c);
			}
			printf("\n");
			printf(SINGLE_FAILURE_MESSAGE   "%s\n", single_failure_message);
			printf(COMPLETE_FAILURE_MESSAGE "%s\n", complete_failure_message);
			return 0;
		}
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
			} else if (sscanf(buffer, MAX_TIMING_ERRORS "%u", &v) == 1) {
				max_timing_errors = v;
			} else if (strncmp(buffer, EXEC_ON_SUCCESS, strlen(EXEC_ON_SUCCESS)) == 0) {
				exec_on_success = strdup(buffer + strlen(EXEC_ON_SUCCESS));
			} else if (strncmp(buffer, AUTHENTICATION_DATA, strlen(AUTHENTICATION_DATA)) == 0) {
				authentication_data = strdup(buffer + strlen(AUTHENTICATION_DATA));
			} else if (strncmp(buffer, SINGLE_FAILURE_MESSAGE, strlen(SINGLE_FAILURE_MESSAGE)) == 0) {
				single_failure_message = strdup(buffer + strlen(SINGLE_FAILURE_MESSAGE));
			} else if (strncmp(buffer, COMPLETE_FAILURE_MESSAGE, strlen(COMPLETE_FAILURE_MESSAGE)) == 0) {
				complete_failure_message = strdup(buffer + strlen(COMPLETE_FAILURE_MESSAGE));
			}
		}
		fclose(fp);
	}
	if (!authentication_data) {
		fprintf(stderr, "No authentication data found.\n");
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
	{
		char *cp = strtok(authentication_data, " ");
		while (cp) {
			unsigned int a, b, c, v;
			if (sscanf(cp, "%u-%u", &a, &b) != 2) break;
			if ((cp = strtok(NULL, " ")) == NULL) break;
			if (strcmp(cp, "\\\\") == 0) {
				c = '\\';
			} else if (sscanf(cp, "\\%o", &v) == 1) {
				c = v;
			} else if ((v = * (unsigned char *) cp) >= 0x21 && v <= 0x7E && v != 0x5c) {
				c = v;
			} else {
				break;
			}
			if ((authdata_list = (ELEMENT *) realloc(authdata_list, sizeof(ELEMENT) * (authdata_list_len + 1))) == NULL) break;
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

	{
		int trial;
		struct termios tp, tp0;
		tcgetattr(0, &tp0); tp = tp0;
		tp.c_lflag &= ~(ICANON | ECHO);
		tp.c_cc[VTIME] = 0;
		tp.c_cc[VMIN] = 1;
		for (trial = 0; trial < max_trials; trial++) {
			int errors = 0, pos = 0, failed = 0;
			printf("%s: ", password_prompt);
			tcsetattr(0, TCSANOW, &tp);
			while (1) {
				int key, interval;
				struct timeval tv0, tv1;
				struct timezone tz;
				gettimeofday(&tv0, &tz);
				key = getc(stdin);
				gettimeofday(&tv1, &tz);
				if (key != '\n') putchar('*'); else putchar('\n');
				interval = (tv1.tv_sec - tv0.tv_sec) * 1000 + (tv1.tv_usec - tv0.tv_usec) / 1000;
				if (!failed && pos < authdata_list_len) {
					if (authdata_list[pos].key != key) {
						failed = 1;
					} else if (interval < authdata_list[pos].min_interval || interval > authdata_list[pos].max_interval) {
						if (++errors > max_timing_errors) failed = 1;
					}
				} else {
					failed = 1;
				}
				pos++;
				if (key == '\n') break;
			}
			tcsetattr(0, TCSANOW, &tp0);
			if (pos == authdata_list_len && !failed) {
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
