/*
 * sakura_capability_test.c
 *
 * Testing program for fs/sakura_capability.c
 *
 * Copyright (C) 2005-2006  NTT DATA CORPORATION
 *
 * Version: 1.3.1   2006/12/08
 *
 */
#include "include.h"

#define DROP_KEYWORD "\\\\disable"

static void ShowPrompt(const char *type, const char *range, const char *command) {
	printf("%6s: Testing %12s %20s : (must fail) ", type, range, command);
	errno = 0;
}

int main(int argc, char *argv[]) {
	int child = 0;
	const char *type = "parent";

	{
		// Am I child?
		if (argc == 2 && strcmp(argv[1], "--inherit") == 0) {
			child = 1;
			type = "child";
		}
	}

	{
		// Check the existence of dropping capability interface.
		execl(DROP_KEYWORD, DROP_KEYWORD, NULL);
		if (errno != EAGAIN) {
			fprintf(stderr, "You can't run this program for this kernel.\n");
			return 0;
		}
	}

	{
		switch (fork()) {
		case 0:
			// Drop capability for chroot(), pivot_root(), mount() if I'm parent.
			if (!child) execl(DROP_KEYWORD, DROP_KEYWORD, "chroot", "pivotroot", "mount", NULL);
			
			ShowPrompt(type, "local", "chroot");
			if (chroot("/") == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			
			ShowPrompt(type, "local", "pivotroot");
			if (pivot_root("/", "/") == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			fflush(stdout);
			
			ShowPrompt(type, "local", "mount");
			if (mount("/", "/", "nonexistent", 0, NULL) == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
	}
	{
		switch (fork()) {
		case 0:
			// Drop capability for chroot(), pivot_root(), mount() if I'm parent.
			if (!child) execl(DROP_KEYWORD, DROP_KEYWORD, "all-chroot", "all-pivotroot", "all-mount", NULL);
			
			ShowPrompt(type, "inheritable", "chroot");
			if (chroot("/") == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			
			ShowPrompt(type, "inheritable", "pivotroot");
			if (pivot_root("/", "/") == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			fflush(stdout);
			
			ShowPrompt(type, "inheritable", "mount");
			if (mount("/", "/", "nonexistent", 0, NULL) == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
	}
	
	{
		switch (fork()) {
		case 0:
			// Drop capability for becoming euid = 0 when euid = 0 if I'm parent.
			if (!child) execl(DROP_KEYWORD, DROP_KEYWORD, "euid0", NULL);
			ShowPrompt(type, "local", "euid0(euid=0)");
			// Become euid != 0.
			seteuid(1);
			// Do something that involves pathname resolution.
			chdir(".");
			// Become euid = 0.
			seteuid(0);
			// This must be rejected.
			if (chdir(".") == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
	}
	{
		switch (fork()) {
		case 0:
			// Drop capability for becoming euid = 0 when euid = 0 if I'm parent.
			if (!child) execl(DROP_KEYWORD, DROP_KEYWORD, "all-euid0", NULL);
			ShowPrompt(type, "inheritable", "euid0(euid=0)");
			// Become euid != 0.
			seteuid(1);
			// Do something that involves pathname resolution.
			chdir(".");
			// Become euid = 0.
			seteuid(0);
			// This must be rejected.
			if (chdir(".") == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
	}

	{
		switch (fork()) {
		case 0:
			// Become euid != 0.
			seteuid(1);
			// Drop capability for becoming euid = 0 when euid != 0 if I'm parent.
			if (!child) execl(DROP_KEYWORD, DROP_KEYWORD, "euid0", NULL);
			ShowPrompt(type, "local", "euid0(euid!=0)");
			// Do something that involves pathname resolution if I'm child.
			if (child) chdir(".");
			// Become euid = 0.
			seteuid(0);
			// This must be rejected.
			if (chdir(".") == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
	}
	{
		switch (fork()) {
		case 0:
			// Become euid != 0.
			seteuid(1);
			// Drop capability for becoming euid = 0 when euid != 0 if I'm parent.
			if (!child) execl(DROP_KEYWORD, DROP_KEYWORD, "all-euid0", NULL);
			ShowPrompt(type, "inheritable", "euid0(euid!=0)");
			// Do something that involves pathname resolution if I'm child.
			if (child) chdir(".");
			// Become euid = 0.
			seteuid(0);
			// This must be rejected.
			if (chdir(".") == 0 || errno != EPERM) printf("BUG: %s\n", strerror(errno));
			else printf("OK: Permission denied.\n");
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
	}

	{
		switch (fork()) {
		case 0:
			// Drop capability for execve().
			execl(DROP_KEYWORD, DROP_KEYWORD, "execve", NULL);
			{
				char *argv[] = { (char *) 1, NULL }, *envp[] = { (char *) 1, NULL };
				/* Use invalid address so that execve() won't terminate this program. */
				ShowPrompt(type, "local", "execve");
				execve("/bin/true", argv, envp);
				if (errno != EPERM) printf("BUG: %s\n", strerror(errno));
				else printf("OK: Permission denied.\n");
			}
			fflush(stdout);
			_exit(0);
		}
		wait(NULL);
	}
	{
		// Inheriting dropped capability for execve() is impossible, for
		// using execve() needs capability for execve().
		// Therefore, "all-execve" keyword is not implemented.
	}
	
	// No more tests for child.
	if (child) return 0;
	
	{
		switch(fork()) {
		case 0:
			// Drop capability for chroot(), pivot_root(), mount(), euid0.
			execl(DROP_KEYWORD, DROP_KEYWORD, "all-chroot", "all-pivotroot", "all-mount", "all-euid0", NULL);
			// Run myself to check dropped capability is inherited.
			execlp(argv[0], argv[0], "--inherit", NULL);
			_exit(0);
		}
		wait(NULL);
	}
	return 0;
} 
