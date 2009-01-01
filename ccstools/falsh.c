/*
 * falsh.c
 *
 * A tiny shell without built-in commands.
 * ( http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf )
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.5   2008/11/11
 *
 * This program is intended to provide a login shell
 * to allow users do extra authentications (CERBERUS) safely.
 * Most shells contain built-in commands that allow attackers
 * do bad things (for example, terminate processes using "kill",
 * dull the response by an infinite loop using "for").
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <signal.h>
#include <wordexp.h>

static int do_shell(const char *commandline)
{
	int status;
	int err;
	pid_t pid;
	struct sigaction sa;
	struct sigaction intr;
	struct sigaction quit;
	sigset_t omask;
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, &intr) < 0)
		goto out;
	if (sigaction(SIGQUIT, &sa, &quit) < 0)
		goto out_restore_sigint;
	sigaddset(&sa.sa_mask, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sa.sa_mask, &omask) == EOF) {
		sigaction(SIGQUIT, &quit, (struct sigaction *) NULL);
out_restore_sigint:
		sigaction(SIGINT, &intr, (struct sigaction *) NULL);
out:
		return -1;
	}
	pid = fork();
	switch (pid) {
		wordexp_t p;
	case 0:
		sigaction(SIGINT, &intr, (struct sigaction *) NULL);
		sigaction(SIGQUIT, &quit, (struct sigaction *) NULL);
		sigprocmask(SIG_SETMASK, &omask, (sigset_t *) NULL);
		if (wordexp(commandline, &p, WRDE_NOCMD) == 0) {
			char **args = (char **) calloc(p.we_wordc + 1,
						       sizeof(char *));
			int i;
			for (i = 0; i < p.we_wordc; i++)
				args[i] = p.we_wordv[i];
			execvp(args[0], args);
			err = errno;
			free(args);
			wordfree(&p);
			fprintf(stderr,
				"ERROR: Can't execute. %s : %s\n",
				commandline, strerror(err));
		} else {
			fprintf(stderr, "ERROR: Can't parse. %s\n",
				commandline);
			err = EINVAL;
		}
		_exit(err);
		break;
	case -1:
		err = errno;
		fprintf(stderr, "ERROR: Can't fork. : %s\n", strerror(err));
		status = -1;
		break;
	default:
		while (1) {
			err = waitpid(pid, &status, 0);
			if (err != EOF)
				break;
			if (errno != EINTR)
				break;
		}
		if (err != pid)
			status = -1;
	}
	sigaction(SIGINT, &intr, (struct sigaction *) NULL);
	sigaction(SIGQUIT, &quit, (struct sigaction *) NULL);
	sigprocmask(SIG_SETMASK, &omask, (sigset_t *) NULL);
	return status;
}

int main(int argc, char *argv[])
{
	static char buffer[1024];
	static char hostname[1024];
	static char cwd[1024];
	struct passwd *pw = getpwuid(getuid());
	char *line;
	int shelllevel = 0;
	if (argc == 3 && !strcmp(argv[1], "-c"))
		return do_shell(argv[2]);
	else if (argc != 1)
		return 1;
	{
		const char *shlvl = getenv("SHLVL");
		if (shlvl)
			shelllevel = atoi(shlvl);
		shelllevel++;
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer) - 1, "%d", shelllevel);
		setenv("SHLVL", buffer, 1);
	}
	setenv("SHELL", "/bin/sh", 1);
	memset(buffer, 0, sizeof(buffer));
	memset(hostname, 0, sizeof(hostname));
	memset(cwd, 0, sizeof(cwd));
	getcwd(cwd, sizeof(cwd) - 1);
	gethostname(hostname, sizeof(hostname) - 1);
	snprintf(buffer, sizeof(buffer) - 1, "[%s@%s %s (SHLVL=%d)]# ",
		 pw ? pw->pw_name : "I have no name!", hostname, cwd,
		 shelllevel);
	stifle_history(20);
	while (1) {
		line = readline(buffer);
		if (!line)
			break;
		if (*line) {
			add_history(line);
			do_shell(line);
		}
		free(line);
		line = NULL;
	}
	if (line)
		free(line);
	printf("\n");
	return 0;
}
