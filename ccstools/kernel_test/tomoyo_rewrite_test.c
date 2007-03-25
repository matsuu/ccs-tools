/*
 * tomoyo_rewrite_test.c
 *
 * Testing program for fs/tomoyo_file.c
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.4   2007/04/01
 *
 */
#include "include.h"

static int is_enforce = 0;

static void ShowPrompt(const char *str) {
	printf("Testing %35s: (%s) ", str, is_enforce ? "must fail" : "must success");
	errno = 0;
}

static void ShowResult(int result) {
	if (is_enforce) {
		if (result == EOF) {
			if (errno == EPERM) printf("OK: Permission denied.\n");
			else printf("BUG!\n");
		} else {
			printf("BUG!\n");
		}
	} else {
		if (result != EOF) printf("OK\n");
		else printf("BUG!\n");
	}
}


static void SetStatus(int status) {
	char buffer[128];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "MAC_FOR_FILE=%d\n", status);
	WriteStatus(buffer);
}

static void AddDomainPolicy(const char *data) {
	char buffer[4096];
	FILE *fp;
	SetStatus(0);
	fp = fopen("/proc/ccs/info/self_domain", "r");
	if (fp) {
		fgets(buffer, sizeof(buffer) - 1, fp);
		fclose(fp);
	} else {
		fprintf(stderr, "BUG! Can't read /proc/ccs/info/self_domain\n");
	}
	fp = fopen("/proc/ccs/policy/domain_policy", "w");
	if (fp) {
		fprintf(fp, "%s\n", buffer);
		fprintf(fp, "%s\n", data);
		fclose(fp);
	} else {
		fprintf(stderr, "BUG! Can't write /proc/ccs/policy/domain_policy\n");
	}
}

static void AddExceptionPolicy(const char *data) {
	FILE *fp;
	SetStatus(0);
	fp = fopen("/proc/ccs/policy/exception_policy", "w");
	if (fp) {
		fprintf(fp, "%s\n", data);
		fclose(fp);
	} else {
		fprintf(stderr, "BUG! Can't write /proc/ccs/policy/exception_policy\n");
	}
}

#define REWRITE_PATH "/tmp/rewrite_test"

static void StageRewriteTest(void) {
	int fd;

	/* Start up */
	AddDomainPolicy("6 " REWRITE_PATH);
	AddDomainPolicy("allow_truncate " REWRITE_PATH);
	AddDomainPolicy("allow_create " REWRITE_PATH);
	AddDomainPolicy("allow_unlink " REWRITE_PATH);
	AddExceptionPolicy("deny_rewrite " REWRITE_PATH);
	close(open(REWRITE_PATH, 0600 | O_WRONLY | O_APPEND | O_CREAT));

	/* Enforce mode */
	SetStatus(3);
	is_enforce = 0;

	ShowPrompt("open(O_RDONLY)");
	fd = open(REWRITE_PATH, O_RDONLY);
	ShowResult(fd);
	close(fd);

	ShowPrompt("open(O_WRONLY | O_APPEND)");
	fd = open(REWRITE_PATH, O_WRONLY | O_APPEND);
	ShowResult(fd);
	close(fd);

	is_enforce = 1;
	ShowPrompt("open(O_WRONLY)");
	fd = open(REWRITE_PATH, O_WRONLY);
	ShowResult(fd);
	close(fd);

	ShowPrompt("open(O_WRONLY | O_TRUNC)");
	fd = open(REWRITE_PATH, O_WRONLY | O_TRUNC);
	ShowResult(fd);
	close(fd);

	ShowPrompt("open(O_WRONLY | O_TRUNC | O_APPEND)");
	fd = open(REWRITE_PATH, O_WRONLY | O_TRUNC | O_APPEND);
	ShowResult(fd);
	close(fd);

	ShowPrompt("truncate()");
	ShowResult(truncate(REWRITE_PATH, 0));

	fd = open(REWRITE_PATH, O_WRONLY | O_APPEND);
	ShowPrompt("ftruncate()");
	ShowResult(ftruncate(fd, 0));
	
	ShowPrompt("fcntl(F_SETFL, ~O_APPEND)");
	ShowResult(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_APPEND));
	close(fd);

	/* Permissive mode */
	SetStatus(2);
	is_enforce = 0;

	ShowPrompt("open(O_RDONLY)");
	fd = open(REWRITE_PATH, O_RDONLY);
	ShowResult(fd);
	close(fd);

	ShowPrompt("open(O_WRONLY | O_APPEND)");
	fd = open(REWRITE_PATH, O_WRONLY | O_APPEND);
	ShowResult(fd);
	close(fd);

	ShowPrompt("open(O_WRONLY)");
	fd = open(REWRITE_PATH, O_WRONLY);
	ShowResult(fd);
	close(fd);

	ShowPrompt("open(O_WRONLY | O_TRUNC)");
	fd = open(REWRITE_PATH, O_WRONLY | O_TRUNC);
	ShowResult(fd);
	close(fd);

	ShowPrompt("open(O_WRONLY | O_TRUNC | O_APPEND)");
	fd = open(REWRITE_PATH, O_WRONLY | O_TRUNC | O_APPEND);
	ShowResult(fd);
	close(fd);

	ShowPrompt("truncate()");
	ShowResult(truncate(REWRITE_PATH, 0));

	fd = open(REWRITE_PATH, O_WRONLY | O_APPEND);
	ShowPrompt("ftruncate()");
	ShowResult(ftruncate(fd, 0));
	
	ShowPrompt("fcntl(F_SETFL, ~O_APPEND)");
	ShowResult(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_APPEND));
	close(fd);

	/* Clean up */
	unlink(REWRITE_PATH);
	AddExceptionPolicy("delete " "deny_rewrite " REWRITE_PATH);
	printf("\n\n");
}

int main(int argc, char *argv[]) {
	Init();
	StageRewriteTest();
	ClearStatus();
	return 0;
}
