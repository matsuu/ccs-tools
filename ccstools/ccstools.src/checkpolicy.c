/*
 * checkpolicy.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-rc   2009/03/04
 *
 */
#include "ccstools.h"

static _Bool strendswith(const char *name, const char *tail)
{
	int len;
	if (!name || !tail)
		return false;
	len = strlen(name) - strlen(tail);
	return len >= 0 && !strcmp(name + len, tail);
}

static int parse_ulong(unsigned long *result, char **str)
{
	const char *cp = *str;
	char *ep;
	int base = 10;
	if (*cp == '0') {
		char c = *(cp + 1);
		if (c == 'x' || c == 'X') {
			base = 16;
			cp += 2;
		} else if (c >= '0' && c <= '7') {
			base = 8;
			cp++;
		}
	}
	*result = strtoul(cp, &ep, base);
	if (cp == ep)
		return 0;
	*str = ep;
	return base == 16 ? VALUE_TYPE_HEXADECIMAL :
		(base == 8 ? VALUE_TYPE_OCTAL : VALUE_TYPE_DECIMAL);
}

static char *find_condition_part(char *data)
{
	char *cp = strstr(data, " if ");
	char *cp2;
	if (cp) {
		while (true) {
			cp2 = strstr(cp + 3, " if ");
			if (!cp2)
				break;
			cp = cp2;
		}
		*cp++ = '\0';
	} else {
		cp = strstr(data, " ; set ");
		if (cp)
			*cp++ = '\0';
	}
	return cp;
}

static unsigned int line = 0;
static unsigned int errors = 0;
static unsigned int warnings = 0;

static _Bool check_condition(char *condition)
{
	enum { TASK_UID,
	       TASK_EUID,
	       TASK_SUID,
	       TASK_FSUID,
	       TASK_GID,
	       TASK_EGID,
	       TASK_SGID,
	       TASK_FSGID,
	       TASK_PID,
	       TASK_PPID,
	       EXEC_ARGC,
	       EXEC_ENVC,
	       TASK_STATE_0,
	       TASK_STATE_1,
	       TASK_STATE_2,
	       TYPE_SOCKET,
	       TYPE_SYMLINK,
	       TYPE_FILE,
	       TYPE_BLOCK_DEV,
	       TYPE_DIRECTORY,
	       TYPE_CHAR_DEV,
	       TYPE_FIFO,
	       MODE_SETUID,
	       MODE_SETGID,
	       MODE_STICKY,
	       MODE_OWNER_READ,
	       MODE_OWNER_WRITE,
	       MODE_OWNER_EXECUTE,
	       MODE_GROUP_READ,
	       MODE_GROUP_WRITE,
	       MODE_GROUP_EXECUTE,
	       MODE_OTHERS_READ,
	       MODE_OTHERS_WRITE,
	       MODE_OTHERS_EXECUTE,
	       TASK_TYPE,
	       TASK_EXECUTE_HANDLER,
	       PATH1_UID,
	       PATH1_GID,
	       PATH1_INO,
	       PATH1_PARENT_UID,
	       PATH1_PARENT_GID,
	       PATH1_PARENT_INO,
	       PATH2_PARENT_UID,
	       PATH2_PARENT_GID,
	       PATH2_PARENT_INO,
	       PATH1_TYPE,
	       PATH1_MAJOR,
	       PATH1_MINOR,
	       PATH1_DEV_MAJOR,
	       PATH1_DEV_MINOR,
	       PATH1_MODE,
	       PATH1_PARENT_MODE,
	       PATH2_PARENT_MODE,
	       EXEC_ARGV,
	       EXEC_ENVP,
	       MAX_KEYWORD };
	static const char *condition_control_keyword[MAX_KEYWORD] = {
		[TASK_UID]             = "task.uid",
		[TASK_EUID]            = "task.euid",
		[TASK_SUID]            = "task.suid",
		[TASK_FSUID]           = "task.fsuid",
		[TASK_GID]             = "task.gid",
		[TASK_EGID]            = "task.egid",
		[TASK_SGID]            = "task.sgid",
		[TASK_FSGID]           = "task.fsgid",
		[TASK_PID]             = "task.pid",
		[TASK_PPID]            = "task.ppid",
		[EXEC_ARGC]            = "exec.argc",
		[EXEC_ENVC]            = "exec.envc",
		[TASK_STATE_0]         = "task.state[0]",
		[TASK_STATE_1]         = "task.state[1]",
		[TASK_STATE_2]         = "task.state[2]",
		[TYPE_SOCKET]          = "socket",
		[TYPE_SYMLINK]         = "symlink",
		[TYPE_FILE]            = "file",
		[TYPE_BLOCK_DEV]       = "block",
		[TYPE_DIRECTORY]       = "directory",
		[TYPE_CHAR_DEV]        = "char",
		[TYPE_FIFO]            = "fifo",
		[MODE_SETUID]          = "setuid",
		[MODE_SETGID]          = "setgid",
		[MODE_STICKY]          = "sticky",
		[MODE_OWNER_READ]      = "owner_read",
		[MODE_OWNER_WRITE]     = "owner_write",
		[MODE_OWNER_EXECUTE]   = "owner_execute",
		[MODE_GROUP_READ]      = "group_read",
		[MODE_GROUP_WRITE]     = "group_write",
		[MODE_GROUP_EXECUTE]   = "group_execute",
		[MODE_OTHERS_READ]     = "others_read",
		[MODE_OTHERS_WRITE]    = "others_write",
		[MODE_OTHERS_EXECUTE]  = "others_execute",
		[TASK_TYPE]            = "task.type",
		[TASK_EXECUTE_HANDLER] = "execute_handler",
		[PATH1_UID]            = "path1.uid",
		[PATH1_GID]            = "path1.gid",
		[PATH1_INO]            = "path1.ino",
		[PATH1_PARENT_UID]     = "path1.parent.uid",
		[PATH1_PARENT_GID]     = "path1.parent.gid",
		[PATH1_PARENT_INO]     = "path1.parent.ino",
		[PATH2_PARENT_UID]     = "path2.parent.uid",
		[PATH2_PARENT_GID]     = "path2.parent.gid",
		[PATH2_PARENT_INO]     = "path2.parent.ino",
		[PATH1_TYPE]           = "path1.type",
		[PATH1_MAJOR]          = "path1.major",
		[PATH1_MINOR]          = "path1.minor",
		[PATH1_DEV_MAJOR]      = "path1.dev_major",
		[PATH1_DEV_MINOR]      = "path1.dev_minor",
		[PATH1_MODE]           = "path1.perm",
		[PATH1_PARENT_MODE]    = "path1.parent.perm",
		[PATH2_PARENT_MODE]    = "path2.parent.perm",
		[EXEC_ARGV]            = "exec.argv[",
		[EXEC_ENVP]            = "exec.envp[\"",
	};
	static u8 condition_control_keyword_len[MAX_KEYWORD];
	char *start = condition;
	u8 left;
	u8 right;
	u8 i;
	unsigned long left_min = 0;
	unsigned long left_max = 0;
	unsigned long right_min = 0;
	unsigned long right_max = 0;
	u8 post_state[4] = { 0, 0, 0, 0 };
	_Bool first = true;
	if (first) {
		first = false;
		for (i = 0; i < MAX_KEYWORD; i++)
			condition_control_keyword_len[i]
				= strlen(condition_control_keyword[i]);
	}
	condition = strstr(condition, "; set ");
	if (condition) {
		*condition = '\0';
		condition += 6;
		while (true) {
			while (*condition == ' ')
				condition++;
			if (!*condition)
				break;
			if (!strncmp(condition, "task.state[0]=", 14))
				i = 0;
			else if (!strncmp(condition, "task.state[1]=", 14))
				i = 1;
			else if (!strncmp(condition, "task.state[2]=", 14))
				i = 2;
			else
				goto out;
			condition += 14;
			if (post_state[3] & (1 << i))
				goto out;
			post_state[3] |= 1 << i;
			if (!parse_ulong(&right_min, &condition) ||
			    right_min > 255)
				goto out;
			post_state[i] = (u8) right_min;
		}
	}
	condition = start;
	if (!strncmp(condition, "if ", 3))
		condition += 3;
	else if (*condition)
		goto out;
	start = condition;
	while (true) {
		while (*condition == ' ')
			condition++;
		if (!*condition)
			break;
		for (left = 0; left < MAX_KEYWORD; left++) {
			int len = condition_control_keyword_len[left];
			if (strncmp(condition,
				    condition_control_keyword[left],
				    len))
				continue;
			condition += len;
			break;
		}
		if (left == EXEC_ARGV) {
			if (!parse_ulong(&left_min, &condition))
				goto out;
			if (*condition++ != ']')
				goto out;
		} else if (left == EXEC_ENVP) {
			char *tmp = condition;
			while (true) {
				const char c = *condition;
				/*
				 * Since environment variable names don't
				 * contain '=', I can treat '"]=' and '"]!='
				 * sequences as delimiters.
				 */
				if (!strncmp(condition, "\"]=", 3) ||
				    !strncmp(condition, "\"]!=", 4))
					break;
				if (!c || c == ' ')
					goto out;
				condition++;
			}
			*condition = '\0';
			if (!savename(tmp))
				goto out;
			*condition = '"';
			condition += 2;
		} else if (left == MAX_KEYWORD) {
			if (!parse_ulong(&left_min, &condition))
				goto out;
			if (*condition == '-') {
				condition++;
				if (!parse_ulong(&left_max, &condition) ||
				    left_min > left_max)
					goto out;
			}
		}
		if (!strncmp(condition, "!=", 2))
			condition += 2;
		else if (*condition == '=')
			condition++;
		else
			goto out;
		if (left == EXEC_ENVP && !strncmp(condition, "NULL", 4)) {
			char c;
			condition += 4;
			c = *condition;
			if (!c || c == ' ')
				continue;
			goto out;
		} else if (left == EXEC_ARGV || left == EXEC_ENVP) {
			char c;
			char *tmp;
			if (*condition++ != '"')
				goto out;
			tmp = condition;
			while (true) {
				c = *condition++;
				if (!c || c == ' ')
					goto out;
				if (c != '"')
					continue;
				c = *condition;
				if (!c || c == ' ')
					break;
			}
			c = *--condition;
			*condition = '\0';
			if (!savename(tmp))
				goto out;
			*condition = c;
			condition++;
			continue;
		}
		for (right = 0; right < MAX_KEYWORD; right++) {
			int len = condition_control_keyword_len[right];
			if (strncmp(condition,
				    condition_control_keyword[right],
				    len))
				continue;
			condition += len;
			break;
		}
		if (right == MAX_KEYWORD) {
			if (!parse_ulong(&right_min, &condition))
				goto out;
			if (*condition == '-') {
				condition++;
				if (!parse_ulong(&right_max, &condition) ||
				    right_min > right_max)
					goto out;
			}
		}
	}
	return true;
out:
	printf("%u: ERROR: '%s' is an illegal condition.\n", line, start);
	errors++;
	return false;
}

static void check_capability_policy(char *data)
{
	static const char *capability_keywords[] = {
		"inet_tcp_create", "inet_tcp_listen", "inet_tcp_connect",
		"use_inet_udp", "use_inet_ip", "use_route", "use_packet",
		"SYS_MOUNT", "SYS_UMOUNT", "SYS_REBOOT", "SYS_CHROOT",
		"SYS_KILL", "SYS_VHANGUP", "SYS_TIME", "SYS_NICE",
		"SYS_SETHOSTNAME", "use_kernel_module", "create_fifo",
		"create_block_dev", "create_char_dev", "create_unix_socket",
		"SYS_LINK", "SYS_SYMLINK", "SYS_RENAME", "SYS_UNLINK",
		"SYS_CHMOD", "SYS_CHOWN", "SYS_IOCTL", "SYS_KEXEC_LOAD",
		"SYS_PIVOT_ROOT", "SYS_PTRACE", NULL
	};
	int i;
	for (i = 0; capability_keywords[i]; i++) {
		if (!strcmp(data, capability_keywords[i]))
			return;
	}
	printf("%u: ERROR: '%s' is a bad capability name.\n", line, data);
	errors++;
}

static void check_signal_policy(char *data)
{
	int sig;
	char *cp;
	cp = strchr(data, ' ');
	if (!cp) {
		printf("%u: ERROR: Too few parameters.\n", line);
		errors++;
		return;
	}
	*cp++ = '\0';
	if (sscanf(data, "%d", &sig) != 1) {
		printf("%u: ERROR: '%s' is a bad signal number.\n", line, data);
		errors++;
	}
	if (!is_correct_domain(cp)) {
		printf("%u: ERROR: '%s' is a bad domainname.\n", line, cp);
		errors++;
	}
}

static void check_argv0_policy(char *data)
{
	char *argv0 = strchr(data, ' ');
	if (!argv0) {
		printf("%u: ERROR: Too few parameters.\n", line);
		errors++;
		return;
	}
	*argv0++ = '\0';
	if (!is_correct_path(data, 1, 0, -1)) {
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, data);
		errors++;
	}
	if (!is_correct_path(argv0, -1, 0, -1) || strchr(argv0, '/')) {
		printf("%u: ERROR: '%s' is a bad argv[0] name.\n", line, data);
		errors++;
	}
}

static void check_env_policy(char *data)
{
	if (!is_correct_path(data, 0, 0, 0)) {
		printf("%u: ERROR: '%s' is a bad variable name.\n", line, data);
		errors++;
	}
}

static void check_network_policy(char *data)
{
	int sock_type;
	int operation;
	u16 min_address[8];
	u16 max_address[8];
	unsigned int min_port;
	unsigned int max_port;
	int count;
	char *cp1 = NULL;
	char *cp2 = NULL;
	cp1 = strchr(data, ' ');
	if (!cp1)
		goto out;
	cp1++;
	if (!strncmp(data, "TCP ", 4))
		sock_type = SOCK_STREAM;
	else if (!strncmp(data, "UDP ", 4))
		sock_type = SOCK_DGRAM;
	else if (!strncmp(data, "RAW ", 4))
		sock_type = SOCK_RAW;
	else
		goto out;
	cp2 = strchr(cp1, ' ');
	if (!cp2)
		goto out;
	cp2++;
	if (!strncmp(cp1, "bind ", 5)) {
		operation = (sock_type == SOCK_STREAM) ? NETWORK_ACL_TCP_BIND :
			(sock_type == SOCK_DGRAM) ? NETWORK_ACL_UDP_BIND :
			NETWORK_ACL_RAW_BIND;
	} else if (!strncmp(cp1, "connect ", 8)) {
		operation = (sock_type == SOCK_STREAM) ?
			NETWORK_ACL_TCP_CONNECT : (sock_type == SOCK_DGRAM) ?
			NETWORK_ACL_UDP_CONNECT : NETWORK_ACL_RAW_CONNECT;
	} else if (sock_type == SOCK_STREAM && !strncmp(cp1, "listen ", 7)) {
		operation = NETWORK_ACL_TCP_LISTEN;
	} else if (sock_type == SOCK_STREAM && !strncmp(cp1, "accept ", 7)) {
		operation = NETWORK_ACL_TCP_ACCEPT;
	} else {
		goto out;
	}
	cp1 = strchr(cp2, ' ');
	if (!cp1)
		goto out;
	cp1++;
	count = sscanf(cp2, "%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx-"
		       "%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx",
		       &min_address[0], &min_address[1], &min_address[2],
		       &min_address[3], &min_address[4], &min_address[5],
		       &min_address[6], &min_address[7], &max_address[0],
		       &max_address[1], &max_address[2], &max_address[3],
		       &max_address[4], &max_address[5], &max_address[6],
		       &max_address[7]);
	if (count == 8 || count == 16) {
		int i;
		for (i = 0; i < 8; i++) {
			min_address[i] = htons(min_address[i]);
			max_address[i] = htons(max_address[i]);
		}
		if (count == 8)
			memmove(max_address, min_address, sizeof(min_address));
		goto next;
	}
	count = sscanf(cp2, "%hu.%hu.%hu.%hu-%hu.%hu.%hu.%hu",
		       &min_address[0], &min_address[1], &min_address[2],
		       &min_address[3], &max_address[0], &max_address[1],
		       &max_address[2], &max_address[3]);
	if (count == 4 || count == 8) {
		u32 ip = htonl((((u8) min_address[0]) << 24) +
			       (((u8) min_address[1]) << 16) +
			       (((u8) min_address[2]) << 8) +
			       (u8) min_address[3]);
		memmove(min_address, &ip, sizeof(ip));
		if (count == 8)
			ip = htonl((((u8) max_address[0]) << 24) +
				   (((u8) max_address[1]) << 16) +
				   (((u8) max_address[2]) << 8) +
				   (u8) max_address[3]);
		memmove(max_address, &ip, sizeof(ip));
		goto next;
	}
	if (*cp2 != '@') /* Don't reject address_group. */
		goto out;
next:
	if (strchr(cp1, ' '))
		goto out;
	count = sscanf(cp1, "%u-%u", &min_port, &max_port);
	if (count == 1 || count == 2) {
		if (count == 1)
			max_port = min_port;
		if (min_port <= max_port && max_port < 65536)
			return;
	}
out:
	printf("%u: ERROR: Bad network address.\n", line);
	errors++;
}

static void check_file_policy(char *data)
{
	static const struct {
		const char * const keyword;
		const int paths;
	} acl_type_array[] = {
		{ "execute",    1 },
		{ "read/write", 1 },
		{ "read",       1 },
		{ "write",      1 },
		{ "create",     1 },
		{ "unlink",     1 },
		{ "mkdir",      1 },
		{ "rmdir",      1 },
		{ "mkfifo",     1 },
		{ "mksock",     1 },
		{ "mkblock",    1 },
		{ "mkchar",     1 },
		{ "truncate",   1 },
		{ "symlink",    1 },
		{ "link",       2 },
		{ "rename",     2 },
		{ "rewrite",    1 },
		{ NULL, 0 }
	};
	char *filename = strchr(data, ' ');
	char *cp;
	unsigned int perm;
	if (!filename) {
		printf("%u: ERROR: Unknown command '%s'\n", line, data);
		errors++;
		return;
	}
	*filename++ = '\0';
	if (sscanf(data, "%u", &perm) == 1 && perm > 0 && perm <= 7) {
		/* Don't reject path_group. */
		if (filename[0] != '@' && strendswith(filename, "/")) {
			printf("%u: WARNING: Only 'mkdir' and 'rmdir' are "
			       "valid for directory '%s'.\n", line, filename);
			warnings++;
		}
		if (!is_correct_path(filename, 0, 0, 0))
			goto out1;
		/* "1", "3", "5", "7" don't accept patterns. */
		if (filename[0] != '@' && (perm & 1) == 1 &&
		    !is_correct_path(filename, 1, -1, -1))
			goto out1;
		return;
	}
	if (!strncmp(data, "allow_", 6)) {
		int type;
		for (type = 0; acl_type_array[type].keyword; type++) {
			if (strcmp(data + 6, acl_type_array[type].keyword))
				continue;
			if (acl_type_array[type].paths == 2) {
				cp = strchr(filename, ' ');
				if (!cp || !is_correct_path(cp + 1, 0, 0, 0))
					break;
				*cp = '\0';
			}
			if (!is_correct_path(filename, 0, 0, 0))
				break;
			/* "allow_execute" doesn't accept patterns. */
			if (!type && filename[0] != '@' &&
			    !is_correct_path(filename, 1, -1, -1))
				break;
			return;
		}
		if (!acl_type_array[type].keyword)
			goto out2;
out1:
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, filename);
		errors++;
		return;
	}
out2:
	printf("%u: ERROR: Invalid permission '%s %s'\n", line, data, filename);
	errors++;
}

static void check_mount_policy(char *data)
{
	char *cp;
	char *cp2;
	const char *dev;
	const char *dir;
	unsigned int flags;
	cp2 = data;
	cp = strchr(cp2, ' ');
	if (!cp)
		goto out;
	*cp = '\0';
	dev = cp2;
	cp2 = cp + 1;
	cp = strchr(cp2, ' ');
	if (!cp)
		goto out;
	*cp = '\0';
	dir = cp2;
	cp2 = strchr(cp + 1, ' ');
	if (!cp2)
		goto out;
	if (!is_correct_path(dev, 0, 0, 0)) {
		printf("%u: ERROR: '%s' is a bad device name.\n", line, dir);
		errors++;
	}
	if (!is_correct_path(dir, 0, 0, 0)) {
		printf("%u: ERROR: '%s' is a bad mount point.\n", line, dir);
		errors++;
	}
	if (sscanf(cp2 + 1, "0x%X", &flags) != 1) {
		printf("%u: ERROR: '%s' is a bad mount option.\n",
		       line, cp2 + 1);
		errors++;
	}
	return;
out:
	printf("%u: ERROR: Too few parameters.\n", line);
	errors++;
}

static void check_pivot_root_policy(char *data)
{
	char *cp = strchr(data, ' ');
	if (!cp)
		goto out;
	*cp++ = '\0';
	if (!is_correct_path(data, 1, 0, 1)) {
		printf("%u: ERROR: '%s' is a bad directory.\n", line, data);
		errors++;
	}
	if (!is_correct_path(cp, 1, 0, 1)) {
		printf("%u: ERROR: '%s' is a bad directory.\n", line, cp);
		errors++;
	}
	return;
out:
	printf("%u: ERROR: Too few parameters.\n", line);
	errors++;
}

static void check_reserved_port_policy(char *data)
{
	unsigned int from;
	unsigned int to;
	if (strchr(data, ' '))
		goto out;
	if (sscanf(data, "%u-%u", &from, &to) == 2) {
		if (from <= to && to < 65536)
			return;
	} else if (sscanf(data, "%u", &from) == 1) {
		if (from < 65536)
			return;
	} else {
		printf("%u: ERROR: Too few parameters.\n", line);
		errors++;
		return;
	}
out:
	printf("%u: ERROR: '%s' is a bad port number.\n", line, data);
	errors++;
}

static void check_domain_initializer_entry(const char *domainname,
					const char *program)
{
	if (!is_correct_path(program, 1, 0, -1)) {
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, program);
		errors++;
	}
	if (domainname && !is_correct_path(domainname, 1, -1, -1) &&
	    !is_correct_domain(domainname)) {
		printf("%u: ERROR: '%s' is a bad domainname.\n",
		       line, domainname);
		errors++;
	}
}

static void check_domain_initializer_policy(char *data)
{
	char *cp = strstr(data, " from ");
	if (cp) {
		*cp = '\0';
		check_domain_initializer_entry(cp + 6, data);
	} else {
		check_domain_initializer_entry(NULL, data);
	}
}

static void check_domain_keeper_entry(const char *domainname,
				      const char *program)
{
	if (!is_correct_path(domainname, 1, -1, -1) &&
	    !is_correct_domain(domainname)) {
		printf("%u: ERROR: '%s' is a bad domainname.\n",
		       line, domainname);
		errors++;
	}
	if (program && !is_correct_path(program, 1, 0, -1)) {
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, program);
		errors++;
	}
}

static void check_domain_keeper_policy(char *data)
{
	char *cp = strstr(data, " from ");
	if (cp) {
		*cp = '\0';
		check_domain_keeper_entry(cp + 6, data);
	} else {
		check_domain_keeper_entry(data, NULL);
	}
}

static void check_path_group_policy(char *data)
{
	char *cp = strchr(data, ' ');
	if (!cp) {
		printf("%u: ERROR: Too few parameters.\n", line);
		errors++;
		return;
	}
	*cp++ = '\0';
	if (!is_correct_path(data, 0, 0, 0)) {
		printf("%u: ERROR: '%s' is a bad group name.\n", line, data);
		errors++;
	}
	if (!is_correct_path(cp, 0, 0, 0)) {
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, cp);
		errors++;
	}
}

static void check_address_group_policy(char *data)
{
	char *cp = strchr(data, ' ');
	u16 min_address[8];
	u16 max_address[8];
	int count;
	if (!cp) {
		printf("%u: ERROR: Too few parameters.\n", line);
		errors++;
		return;
	}
	*cp++ = '\0';
	count = sscanf(cp, "%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx-"
		       "%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx",
		       &min_address[0], &min_address[1], &min_address[2],
		       &min_address[3], &min_address[4], &min_address[5],
		       &min_address[6], &min_address[7], &max_address[0],
		       &max_address[1], &max_address[2], &max_address[3],
		       &max_address[4], &max_address[5], &max_address[6],
		       &max_address[7]);
	if (count == 8 || count == 16)
		return;
	count = sscanf(cp, "%hu.%hu.%hu.%hu-%hu.%hu.%hu.%hu",
		       &min_address[0], &min_address[1], &min_address[2],
		       &min_address[3], &max_address[0], &max_address[1],
		       &max_address[2], &max_address[3]);
	if (count == 4 || count == 8)
		return;
	printf("%u: ERROR: '%s' is a bad address.\n", line, cp);
	errors++;
}

static void check_domain_policy(void)
{
	static int domain = EOF;
	_Bool is_delete = false;
	_Bool is_select = false;
	_Bool is_undelete = false;
	if (str_starts(shared_buffer, KEYWORD_DELETE))
		is_delete = true;
	else if (str_starts(shared_buffer, KEYWORD_SELECT))
		is_select = true;
	else if (str_starts(shared_buffer, KEYWORD_UNDELETE))
		is_undelete = true;
	if (is_domain_def(shared_buffer)) {
		if (!is_correct_domain(shared_buffer) ||
		    strlen(shared_buffer) >= CCS_MAX_PATHNAME_LEN) {
			printf("%u: ERROR: '%s' is a bad domainname.\n",
			       line, shared_buffer);
			errors++;
		} else {
			if (is_delete)
				domain = EOF;
			else
				domain = 0;
		}
	} else if (is_select) {
		printf("%u: ERROR: Command 'select' is valid for selecting "
		       "domains only.\n", line);
		errors++;
	} else if (is_undelete) {
		printf("%u: ERROR: Command 'undelete' is valid for undeleting "
		       "domains only.\n", line);
		errors++;
	} else if (domain == EOF) {
		printf("%u: WARNING: '%s' is unprocessed because domain is not "
		       "selected.\n", line, shared_buffer);
		warnings++;
	} else if (str_starts(shared_buffer, KEYWORD_USE_PROFILE)) {
		unsigned int profile;
		if (sscanf(shared_buffer, "%u", &profile) != 1 ||
		    profile >= 256) {
			printf("%u: ERROR: '%s' is a bad profile.\n",
			       line, shared_buffer);
			errors++;
		}
	} else if (!strcmp(shared_buffer, "ignore_global_allow_read")) {
		/* Nothing to do. */
	} else if (!strcmp(shared_buffer, "ignore_global_allow_env")) {
		/* Nothing to do. */
	} else if (str_starts(shared_buffer, "execute_handler ") ||
		   str_starts(shared_buffer, "denied_execute_handler ")) {
		if (!is_correct_path(shared_buffer, 1, -1, -1)) {
			printf("%u: ERROR: '%s' is a bad pathname.\n",
			       line, shared_buffer);
			errors++;
		}
	} else if (!strcmp(shared_buffer, "transition_failed")) {
		/* Nothing to do. */
	} else if (!strcmp(shared_buffer, "quota_exceeded")) {
		/* Nothing to do. */
	} else {
		char *cp = find_condition_part(shared_buffer);
		if (cp && !check_condition(cp))
			return;
		if (str_starts(shared_buffer, KEYWORD_ALLOW_CAPABILITY))
			check_capability_policy(shared_buffer);
		else if (str_starts(shared_buffer, KEYWORD_ALLOW_NETWORK))
			check_network_policy(shared_buffer);
		else if (str_starts(shared_buffer, KEYWORD_ALLOW_SIGNAL))
			check_signal_policy(shared_buffer);
		else if (str_starts(shared_buffer, KEYWORD_ALLOW_ARGV0))
			check_argv0_policy(shared_buffer);
		else if (str_starts(shared_buffer, KEYWORD_ALLOW_ENV))
			check_env_policy(shared_buffer);
		else
			check_file_policy(shared_buffer);
	}
}

static void check_exception_policy(void)
{
	str_starts(shared_buffer, KEYWORD_DELETE);
	if (str_starts(shared_buffer, KEYWORD_ALLOW_READ)) {
		if (!is_correct_path(shared_buffer, 1, 0, -1)) {
			printf("%u: ERROR: '%s' is a bad pathname.\n",
			       line, shared_buffer);
			errors++;
		}
	} else if (str_starts(shared_buffer, KEYWORD_INITIALIZE_DOMAIN)) {
		check_domain_initializer_policy(shared_buffer);
	} else if (str_starts(shared_buffer, KEYWORD_NO_INITIALIZE_DOMAIN)) {
		check_domain_initializer_policy(shared_buffer);
	} else if (str_starts(shared_buffer, KEYWORD_KEEP_DOMAIN)) {
		check_domain_keeper_policy(shared_buffer);
	} else if (str_starts(shared_buffer, KEYWORD_NO_KEEP_DOMAIN)) {
		check_domain_keeper_policy(shared_buffer);
	} else if (str_starts(shared_buffer, KEYWORD_PATH_GROUP)) {
		check_path_group_policy(shared_buffer);
	} else if (str_starts(shared_buffer, KEYWORD_ADDRESS_GROUP)) {
		check_address_group_policy(shared_buffer);
	} else if (str_starts(shared_buffer, KEYWORD_ALIAS)) {
		char *cp = strchr(shared_buffer, ' ');
		if (!cp) {
			printf("%u: ERROR: Too few parameters.\n", line);
			errors++;
		} else {
			*cp++ = '\0';
			if (!is_correct_path(shared_buffer, 1, -1, -1)) {
				printf("%u: ERROR: '%s' is a bad pathname.\n",
				       line, shared_buffer);
				errors++;
			}
			if (!is_correct_path(cp, 1, -1, -1)) {
				printf("%u: ERROR: '%s' is a bad pathname.\n",
				       line, cp);
				errors++;
			}
		}
	} else if (str_starts(shared_buffer, KEYWORD_AGGREGATOR)) {
		char *cp = strchr(shared_buffer, ' ');
		if (!cp) {
			printf("%u: ERROR: Too few parameters.\n", line);
			errors++;
		} else {
			*cp++ = '\0';
			if (!is_correct_path(shared_buffer, 1, 0, -1)) {
				printf("%u: ERROR: '%s' is a bad pattern.\n",
				       line, shared_buffer);
				errors++;
			}
			if (!is_correct_path(cp, 1, -1, -1)) {
				printf("%u: ERROR: '%s' is a bad pathname.\n",
				       line, cp);
				errors++;
			}
		}
	} else if (str_starts(shared_buffer, KEYWORD_FILE_PATTERN)) {
		if (!is_correct_path(shared_buffer, 0, 1, 0)) {
			printf("%u: ERROR: '%s' is a bad pattern.\n",
			       line, shared_buffer);
			errors++;
		}
	} else if (str_starts(shared_buffer, KEYWORD_DENY_REWRITE)) {
		if (!is_correct_path(shared_buffer, 0, 0, 0)) {
			printf("%u: ERROR: '%s' is a bad pattern.\n",
			       line, shared_buffer);
			errors++;
		}
	} else if (str_starts(shared_buffer, KEYWORD_ALLOW_ENV)) {
		if (!is_correct_path(shared_buffer, 0, 0, 0)) {
			printf("%u: ERROR: '%s' is a bad variable name.\n",
			       line, shared_buffer);
			errors++;
		}
	} else {
		printf("%u: ERROR: Unknown command '%s'.\n",
		       line, shared_buffer);
		errors++;
	}
}

static void check_system_policy(void)
{
	str_starts(shared_buffer, KEYWORD_DELETE);
	if (str_starts(shared_buffer, KEYWORD_ALLOW_MOUNT)) {
		check_mount_policy(shared_buffer);
	} else if (str_starts(shared_buffer, KEYWORD_DENY_UNMOUNT)) {
		if (!is_correct_path(shared_buffer, 1, 0, 1)) {
			printf("%u: ERROR: '%s' is a bad pattern.\n",
			       line, shared_buffer);
			errors++;
		}
	} else if (str_starts(shared_buffer, KEYWORD_ALLOW_CHROOT)) {
		if (!is_correct_path(shared_buffer, 1, 0, 1)) {
			printf("%u: ERROR: '%s' is a bad pattern.\n",
			       line, shared_buffer);
			errors++;
		}
	} else if (str_starts(shared_buffer, KEYWORD_ALLOW_PIVOT_ROOT)) {
		check_pivot_root_policy(shared_buffer);
	} else if (str_starts(shared_buffer, KEYWORD_DENY_AUTOBIND)) {
		check_reserved_port_policy(shared_buffer);
	} else {
		printf("%u: ERROR: Unknown command '%s'.\n",
		       line, shared_buffer);
		errors++;
	}
}

int checkpolicy_main(int argc, char *argv[])
{
	int policy_type = POLICY_TYPE_UNKNOWN;
	if (argc > 1) {
		switch (argv[1][0]) {
		case 's':
			policy_type = POLICY_TYPE_SYSTEM_POLICY;
			break;
		case 'e':
			policy_type = POLICY_TYPE_EXCEPTION_POLICY;
			break;
		case 'd':
			policy_type = POLICY_TYPE_DOMAIN_POLICY;
			break;
		}
	}
	if (policy_type == POLICY_TYPE_UNKNOWN) {
		fprintf(stderr, "%s s|e|d < policy_to_check\n", argv[0]);
		return 0;
	}
	get();
	while (memset(shared_buffer, 0, sizeof(shared_buffer)),
	       fgets(shared_buffer, sizeof(shared_buffer) - 1, stdin)) {
		char *cp = strchr(shared_buffer, '\n');
		line++;
		if (!cp) {
			printf("%u: ERROR: Line too long.\n", line);
			errors++;
			break;
		}
		*cp = '\0';
		{
			int c;
			for (c = 1; c < 256; c++) {
				if (c == '\t' || c == '\r' ||
				    (c >= ' ' && c < 127))
					continue;
				if (!strchr(shared_buffer, c))
					continue;
				printf("%u: WARNING: Line contains illegal "
				       "character (\\%03o).\n", line, c);
				warnings++;
				break;
			}
		}
		normalize_line(shared_buffer);
		if (!shared_buffer[0])
			continue;
		switch (policy_type) {
		case POLICY_TYPE_DOMAIN_POLICY:
			check_domain_policy();
			break;
		case POLICY_TYPE_EXCEPTION_POLICY:
			check_exception_policy();
			break;
		case POLICY_TYPE_SYSTEM_POLICY:
			check_system_policy();
			break;
		}
	}
	put();
	printf("Total:   %u Line%s   %u Error%s   %u Warning%s\n",
	       line, line > 1 ? "s" : "", errors, errors > 1 ? "s" : "",
	       warnings, warnings > 1 ? "s" : "");
	return errors ? 2 : (warnings ? 1 : 0);
}
