/*
 * tomoyo_rewrite_test.c
 *
 * Testing program for fs/tomoyo_file.c
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7+   2009/04/08
 *
 */
#include "include.h"

static int is_enforce = 0;

static void show_prompt(const char *str)
{
	printf("Testing %35s: (%s) ", str,
	       is_enforce ? "must fail" : "must success");
	errno = 0;
}

static void show_result(int result)
{
	if (is_enforce) {
		if (result == EOF) {
			if (errno == EPERM)
				printf("OK: Permission denied.\n");
			else
				printf("BUG!\n");
		} else {
			printf("BUG!\n");
		}
	} else {
		if (result != EOF)
			printf("OK\n");
		else
			printf("BUG!\n");
	}
}


static void set_status(int status)
{
	char buffer[128];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "MAC_FOR_FILE=%d\n", status);
	write_status(buffer);
}

static void add_domain_policy(const char *data)
{
	char buffer[4096];
	FILE *fp;
	set_status(0);
	fp = fopen(proc_policy_self_domain, "r");
	if (fp) {
		fgets(buffer, sizeof(buffer) - 1, fp);
		fclose(fp);
	} else {
		fprintf(stderr, "BUG! Can't read %s\n",
			proc_policy_self_domain);
	}
	fp = fopen(proc_policy_domain_policy, "w");
	if (fp) {
		fprintf(fp, "%s\n", buffer);
		fprintf(fp, "%s\n", data);
		fclose(fp);
	} else {
		fprintf(stderr, "BUG! Can't write %s\n",
			proc_policy_domain_policy);
	}
}

static void add_exception_policy(const char *data)
{
	FILE *fp;
	set_status(0);
	fp = fopen(proc_policy_exception_policy, "w");
	if (fp) {
		fprintf(fp, "%s\n", data);
		fclose(fp);
	} else {
		fprintf(stderr, "BUG! Can't write %s\n",
			proc_policy_exception_policy);
	}
}

#define REWRITE_PATH "/tmp/rewrite_test"

static void stage_rewrite_test(void)
{
	int fd;

	/* Start up */
	add_domain_policy("6 " REWRITE_PATH);
	add_domain_policy("allow_truncate " REWRITE_PATH);
	add_domain_policy("allow_create " REWRITE_PATH);
	add_domain_policy("allow_unlink " REWRITE_PATH);
	add_exception_policy("deny_rewrite " REWRITE_PATH);
	close(open(REWRITE_PATH, O_WRONLY | O_APPEND | O_CREAT, 0600));

	/* Enforce mode */
	set_status(3);
	is_enforce = 0;

	show_prompt("open(O_RDONLY)");
	fd = open(REWRITE_PATH, O_RDONLY);
	show_result(fd);
	close(fd);

	show_prompt("open(O_WRONLY | O_APPEND)");
	fd = open(REWRITE_PATH, O_WRONLY | O_APPEND);
	show_result(fd);
	close(fd);

	is_enforce = 1;
	show_prompt("open(O_WRONLY)");
	fd = open(REWRITE_PATH, O_WRONLY);
	show_result(fd);
	close(fd);

	show_prompt("open(O_WRONLY | O_TRUNC)");
	fd = open(REWRITE_PATH, O_WRONLY | O_TRUNC);
	show_result(fd);
	close(fd);

	show_prompt("open(O_WRONLY | O_TRUNC | O_APPEND)");
	fd = open(REWRITE_PATH, O_WRONLY | O_TRUNC | O_APPEND);
	show_result(fd);
	close(fd);

	show_prompt("truncate()");
	show_result(truncate(REWRITE_PATH, 0));

	fd = open(REWRITE_PATH, O_WRONLY | O_APPEND);
	show_prompt("ftruncate()");
	show_result(ftruncate(fd, 0));

	show_prompt("fcntl(F_SETFL, ~O_APPEND)");
	show_result(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_APPEND));
	close(fd);

	/* Permissive mode */
	set_status(2);
	is_enforce = 0;

	show_prompt("open(O_RDONLY)");
	fd = open(REWRITE_PATH, O_RDONLY);
	show_result(fd);
	close(fd);

	show_prompt("open(O_WRONLY | O_APPEND)");
	fd = open(REWRITE_PATH, O_WRONLY | O_APPEND);
	show_result(fd);
	close(fd);

	show_prompt("open(O_WRONLY)");
	fd = open(REWRITE_PATH, O_WRONLY);
	show_result(fd);
	close(fd);

	show_prompt("open(O_WRONLY | O_TRUNC)");
	fd = open(REWRITE_PATH, O_WRONLY | O_TRUNC);
	show_result(fd);
	close(fd);

	show_prompt("open(O_WRONLY | O_TRUNC | O_APPEND)");
	fd = open(REWRITE_PATH, O_WRONLY | O_TRUNC | O_APPEND);
	show_result(fd);
	close(fd);

	show_prompt("truncate()");
	show_result(truncate(REWRITE_PATH, 0));

	fd = open(REWRITE_PATH, O_WRONLY | O_APPEND);
	show_prompt("ftruncate()");
	show_result(ftruncate(fd, 0));

	show_prompt("fcntl(F_SETFL, ~O_APPEND)");
	show_result(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_APPEND));
	close(fd);

	/* Clean up */
	unlink(REWRITE_PATH);
	add_exception_policy("delete " "deny_rewrite " REWRITE_PATH);
	printf("\n\n");
}

int main(int argc, char *argv[])
{
	ccs_test_init();
	if (access(proc_policy_domain_policy, F_OK)) {
		fprintf(stderr, "You can't use this program for this kernel."
			"\n");
		return 1;
	}
	stage_rewrite_test();
	clear_status();
	return 0;
}
