/*
 * ccstools.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.4       2007/04/01
 *
 *   gcc -Wall -O3 -o ccstools ccstools.c -lncurses
 *
 * Be sure to make symlinks or links to this program.
 *  
 */

/***** CONFIGURATION START *****/
/* Comment out what you don't want to compile. */

#define NEED_SORTPOLICY
#define NEED_SETPROFILE
#define NEED_SETLEVEL
#define NEED_SAVEPOLICY
#define NEED_PATHMATCH
#define NEED_LOADPOLICY
#define NEED_LDWATCH
#define NEED_FINDTEMP
#define NEED_EDITPOLICY
#define NEED_CHECKPOLICY
#define NEED_CCSTREE
#define NEED_CCSQUERYD
#define NEED_CCSAUDITD
#define NEED_PATTERNIZE

/***** CONFIGURATION END *****/

/***** CONSTANTS DEFINITION START *****/

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define u8 __u8
#define u16 __u16
#define u32 __u32
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <asm/types.h>
#include <curses.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define SYSTEM_POLICY_FILE    "system_policy"
#define EXCEPTION_POLICY_FILE "exception_policy"
#define DOMAIN_POLICY_FILE    "domain_policy"

#define SCREEN_SYSTEM_LIST    0
#define SCREEN_EXCEPTION_LIST 1
#define SCREEN_DOMAIN_LIST    2
#define SCREEN_ACL_LIST       3
#define MAXSCREEN             4

#define POLICY_TYPE_UNKNOWN          0
#define POLICY_TYPE_DOMAIN_POLICY    1
#define POLICY_TYPE_EXCEPTION_POLICY 2
#define POLICY_TYPE_SYSTEM_POLICY    3

#define VALUE_TYPE_DECIMAL     1
#define VALUE_TYPE_OCTAL       2
#define VALUE_TYPE_HEXADECIMAL 3

#define NETWORK_ACL_UDP_BIND    0
#define NETWORK_ACL_UDP_CONNECT 1
#define NETWORK_ACL_TCP_BIND    2
#define NETWORK_ACL_TCP_LISTEN  3
#define NETWORK_ACL_TCP_CONNECT 4
#define NETWORK_ACL_TCP_ACCEPT  5
#define NETWORK_ACL_RAW_BIND    6
#define NETWORK_ACL_RAW_CONNECT 7

#define KEYWORD_AGGREGATOR               "aggregator "
#define KEYWORD_AGGREGATOR_LEN           (sizeof(KEYWORD_AGGREGATOR) - 1)
#define KEYWORD_ALIAS                    "alias "
#define KEYWORD_ALIAS_LEN                (sizeof(KEYWORD_ALIAS) - 1)
#define KEYWORD_ALLOW_ARGV0              "allow_argv0 "
#define KEYWORD_ALLOW_ARGV0_LEN          (sizeof(KEYWORD_ALLOW_ARGV0) - 1)
#define KEYWORD_ALLOW_CAPABILITY         "allow_capability "
#define KEYWORD_ALLOW_CAPABILITY_LEN     (sizeof(KEYWORD_ALLOW_CAPABILITY) - 1)
#define KEYWORD_ALLOW_CHROOT             "allow_chroot "
#define KEYWORD_ALLOW_CHROOT_LEN         (sizeof(KEYWORD_ALLOW_CHROOT) - 1)
#define KEYWORD_ALLOW_MOUNT              "allow_mount "
#define KEYWORD_ALLOW_MOUNT_LEN          (sizeof(KEYWORD_ALLOW_MOUNT) - 1)
#define KEYWORD_ALLOW_NETWORK            "allow_network "
#define KEYWORD_ALLOW_NETWORK_LEN        (sizeof(KEYWORD_ALLOW_NETWORK) - 1)
#define KEYWORD_ALLOW_PIVOT_ROOT         "allow_pivot_root "
#define KEYWORD_ALLOW_PIVOT_ROOT_LEN     (sizeof(KEYWORD_ALLOW_PIVOT_ROOT) - 1)
#define KEYWORD_ALLOW_READ               "allow_read "
#define KEYWORD_ALLOW_READ_LEN           (sizeof(KEYWORD_ALLOW_READ) - 1)
#define KEYWORD_ALLOW_SIGNAL             "allow_signal "
#define KEYWORD_ALLOW_SIGNAL_LEN         (sizeof(KEYWORD_ALLOW_SIGNAL) - 1)
#define KEYWORD_DELETE                   "delete "
#define KEYWORD_DELETE_LEN               (sizeof(KEYWORD_DELETE) - 1)
#define KEYWORD_DENY_AUTOBIND            "deny_autobind "
#define KEYWORD_DENY_AUTOBIND_LEN        (sizeof(KEYWORD_DENY_AUTOBIND) - 1)
#define KEYWORD_DENY_REWRITE             "deny_rewrite "
#define KEYWORD_DENY_REWRITE_LEN         (sizeof(KEYWORD_DENY_REWRITE) - 1)
#define KEYWORD_DENY_UNMOUNT             "deny_unmount "
#define KEYWORD_DENY_UNMOUNT_LEN         (sizeof(KEYWORD_DENY_UNMOUNT) - 1)
#define KEYWORD_FILE_PATTERN             "file_pattern "
#define KEYWORD_FILE_PATTERN_LEN         (sizeof(KEYWORD_FILE_PATTERN) - 1)
#define KEYWORD_INITIALIZER              "initializer "
#define KEYWORD_INITIALIZER_LEN          (sizeof(KEYWORD_INITIALIZER) - 1)
#define KEYWORD_MAC_FOR_CAPABILITY       "MAC_FOR_CAPABILITY::"
#define KEYWORD_MAC_FOR_CAPABILITY_LEN   (sizeof(KEYWORD_MAC_FOR_CAPABILITY) - 1)
#define KEYWORD_SELECT                   "select "
#define KEYWORD_SELECT_LEN               (sizeof(KEYWORD_SELECT) - 1)
#define KEYWORD_UNDELETE                 "undelete "
#define KEYWORD_UNDELETE_LEN             (sizeof(KEYWORD_UNDELETE) - 1)
#define KEYWORD_DOMAIN_KEEPER            "keep_domain "
#define KEYWORD_DOMAIN_KEEPER_LEN        (sizeof(KEYWORD_DOMAIN_KEEPER) - 1)
#define KEYWORD_USE_PROFILE              "use_profile "
#define KEYWORD_USE_PROFILE_LEN          (sizeof(KEYWORD_USE_PROFILE) - 1)
#define KEYWORD_INITIALIZE_DOMAIN        "initialize_domain "
#define KEYWORD_INITIALIZE_DOMAIN_LEN    (sizeof(KEYWORD_INITIALIZE_DOMAIN) - 1)
#define KEYWORD_KEEP_DOMAIN              "keep_domain "
#define KEYWORD_KEEP_DOMAIN_LEN          (sizeof(KEYWORD_KEEP_DOMAIN) - 1)
#define KEYWORD_PATH_GROUP               "path_group "
#define KEYWORD_PATH_GROUP_LEN           (sizeof(KEYWORD_PATH_GROUP) - 1)
#define KEYWORD_NO_INITIALIZER           "no_" KEYWORD_INITIALIZER
#define KEYWORD_NO_INITIALIZER_LEN       (sizeof(KEYWORD_NO_INITIALIZER) - 1)
#define KEYWORD_NO_INITIALIZE_DOMAIN     "no_" KEYWORD_INITIALIZE_DOMAIN
#define KEYWORD_NO_INITIALIZE_DOMAIN_LEN (sizeof(KEYWORD_NO_INITIALIZE_DOMAIN) - 1)
#define KEYWORD_NO_KEEP_DOMAIN           "no_" KEYWORD_KEEP_DOMAIN
#define KEYWORD_NO_KEEP_DOMAIN_LEN       (sizeof(KEYWORD_NO_KEEP_DOMAIN) - 1)

#define MAXBUFSIZE                       8192
#define CCS_AUDITD_MAX_FILES             2
#define SAVENAME_MAX_HASH                256
#define PAGE_SIZE                        4096
#define CCS_MAX_PATHNAME_LEN             4000
#define ROOT_NAME                        "<kernel>"
#define ROOT_NAME_LEN                    (sizeof(ROOT_NAME) - 1)

/***** CONSTANTS DEFINITION END *****/

/***** UTILITY FUNCTIONS START *****/

static void OutOfMemory(void) {
	fprintf(stderr, "Out of memory. Aborted.\n");
	exit(1);
}

static void NormalizeLine(unsigned char *line) {
	unsigned char *sp = line, *dp = line;
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

/* Copied from kernel source. */
static inline unsigned long partial_name_hash(unsigned long c, unsigned long prevhash) {
	return (prevhash + (c << 4) + (c >> 4)) * 11;
}

/* Copied from kernel source. */
static inline unsigned int full_name_hash(const unsigned char *name, unsigned int len) {
	unsigned long hash = 0;
	while (len--) hash = partial_name_hash(*name++, hash);
	return (unsigned int) hash;
}

static char *alloc_element(const unsigned int size) {
	static char *buf = NULL;
	static unsigned int buf_used_len = PAGE_SIZE;
	char *ptr = NULL;
	if (size > PAGE_SIZE) return NULL;
	if (buf_used_len + size > PAGE_SIZE) {
		if ((ptr = malloc(PAGE_SIZE)) == NULL) OutOfMemory();
		buf = ptr;
		memset(buf, 0, PAGE_SIZE);
		buf_used_len = size;
		ptr = buf;
	} else if (size) {
		int i;
		ptr = buf + buf_used_len;
		buf_used_len += size;
		for (i = 0; i < size; i++) if (ptr[i]) OutOfMemory();
	}
	return ptr;
}

static int PathDepth(const char *pathname) {
	int i = 0;
	if (pathname) {
		char *ep = strchr(pathname, '\0');
		if (pathname < ep--) {
			if (*ep != '/') i++;
			while (pathname <= ep) if (*ep-- == '/') i += 2;
		}
	}
	return i;
}

static int const_part_length(const char *filename) {
	int len = 0;
	if (filename) {
		char c;
		while ((c = *filename++) != '\0') {
			if (c != '\\') { len++; continue; }
			switch (c = *filename++) {
			case '\\':  /* "\\" */
				len += 2; continue;
			case '0':   /* "\ooo" */
			case '1':
			case '2':
			case '3':
				if ((c = *filename++) >= '0' && c <= '7' && (c = *filename++) >= '0' && c <= '7') { len += 4; continue; }
			}
			break;
		}
	}
	return len;
}

static int IsDomainDef(const unsigned char *domainname) {
	return strncmp(domainname, ROOT_NAME, ROOT_NAME_LEN) == 0 && (domainname[ROOT_NAME_LEN] == '\0' || domainname[ROOT_NAME_LEN] == ' ');
}

static int IsCorrectDomain(const unsigned char *domainname) {
	unsigned char c, d, e;
	if (!domainname || strncmp(domainname, ROOT_NAME, ROOT_NAME_LEN)) goto out;
	domainname += ROOT_NAME_LEN;
	if (!*domainname) return 1;
	do {
		if (*domainname++ != ' ') goto out;
		if (*domainname++ != '/') goto out;
		while ((c = *domainname) != '\0' && c != ' ') {
			domainname++;
			if (c == '\\') {
				switch ((c = *domainname++)) {
				case '\\':  /* "\\" */
					continue;
				case '0':   /* "\ooo" */
				case '1':
				case '2':
				case '3':
					if ((d = *domainname++) >= '0' && d <= '7' && (e = *domainname++) >= '0' && e <= '7') {
						const unsigned char f =
							(((unsigned char) (c - '0')) << 6) +
							(((unsigned char) (d - '0')) << 3) +
							(((unsigned char) (e - '0')));
						if (f && (f <= ' ' || f >= 127)) continue; /* pattern is not \000 */
					}
				}
				goto out;
			} else if (c < ' ' || c >= 127) {
				goto out;
			}
		}
	} while (*domainname);
	return 1;
 out:
	return 0;
}

static void fprintf_encoded(FILE *fp, const char *pathname) {
	unsigned char c;
	while ((c = * (const unsigned char *) pathname++) != 0) {
		if (c == '\\') {
			fputc('\\', fp);
			fputc('\\', fp);
		} else if (c > 32 && c < 127) {
			fputc(c, fp);
		} else {
			fprintf(fp, "\\%c%c%c", (c >> 6) + '0', ((c >> 3) & 7) + '0', (c & 7) + '0'); 
		}
	}
}

static int decode(const char *ascii, char *bin) {
	char c, d, e;
	while ((c = *bin++ = *ascii++) != '\0') {
		if (c == '\\') {
			c = *ascii++;
			switch (c) {
			case '\\':      /* "\\" */
				continue;
			case '0':       /* "\ooo" */
			case '1':
			case '2':
			case '3':
				if ((d = *ascii++) >= '0' && d <= '7' && (e = *ascii++) >= '0' && e <= '7') {
					const unsigned char f =
						(((unsigned char) (c - '0')) << 6) +
						(((unsigned char) (d - '0')) << 3) +
						(((unsigned char) (e - '0')));
					if (f && (f <= ' ' || f >= 127)) {
						*(bin - 1) = f;
						continue; /* pattern is not \000 */
					}
				}
			}
			return 0;
		} else if (c <= ' ' || c >= 127) {
			return 0;
		}
	}
	return 1;
}

static void RemoveHeader(char *line, const int len) {
	memmove(line, line + len, strlen(line + len) + 1); 
}

static int strendswith(const char *name, const char *tail) {
	int len;
	if (!name || !tail) return 0;
	len = strlen(name) - strlen(tail);
	return len >= 0 && strcmp(name + len, tail) == 0;
}

static int IsCorrectPath(const char *filename, const int start_type, const int pattern_type, const int end_type) {
	int contains_pattern = 0;
	char c, d, e;
	if (!filename) goto out;
	c = *filename;
	if (start_type == 1) { /* Must start with '/' */
		if (c != '/') goto out;
	} else if (start_type == -1) { /* Must not start with '/' */
		if (c == '/') goto out;
	}
	if (c) c = * (strchr(filename, '\0') - 1);
	if (end_type == 1) { /* Must end with '/' */
		if (c != '/') goto out;
	} else if (end_type == -1) { /* Must not end with '/' */
		if (c == '/') goto out;
	}
	while ((c = *filename++) != '\0') {
		if (c == '\\') {
			switch ((c = *filename++)) {
			case '\\':  /* "\\" */
				continue;
			case '$':   /* "\$" */
			case '+':   /* "\+" */
			case '?':   /* "\?" */
			case '*':   /* "\*" */
			case '@':   /* "\@" */
			case 'x':   /* "\x" */
			case 'X':   /* "\X" */
			case 'a':   /* "\a" */
			case 'A':   /* "\A" */
				if (pattern_type == -1) break; /* Must not contain pattern */
				contains_pattern = 1;
				continue;
			case '0':   /* "\ooo" */
			case '1':
			case '2':
			case '3':
				if ((d = *filename++) >= '0' && d <= '7' && (e = *filename++) >= '0' && e <= '7') {
					const unsigned char f =
						(((unsigned char) (c - '0')) << 6) +
						(((unsigned char) (d - '0')) << 3) +
						(((unsigned char) (e - '0')));
					if (f && (f <= ' ' || f >= 127)) continue; /* pattern is not \000 */
				}
			}
			goto out;
		} else if (c <= ' ' || c >= 127) {
			goto out;
		}
	}
	if (pattern_type == 1) { /* Must contain pattern */
		if (!contains_pattern) goto out;
	}
	return 1;
 out:
	return 0;
}

static int FileMatchesToPattern(const char *filename, const char *filename_end, const char *pattern, const char *pattern_end) {
	while (filename < filename_end && pattern < pattern_end) {
		if (*pattern != '\\') {
			if (*filename++ != *pattern++) return 0;
		} else {
			char c = *filename;
			pattern++;
			switch (*pattern) {
			case '?':
				if (c == '/') {
					return 0;
				} else if (c == '\\') {
					if ((c = filename[1]) == '\\') {
						filename++; /* safe because filename is \\ */
					} else if (c >= '0' && c <= '3' && (c = filename[2]) >= '0' && c <= '7' && (c = filename[3]) >= '0' && c <= '7') {
						filename += 3; /* safe because filename is \ooo */
					} else {
						return 0;
					}
				}
				break;
			case '\\':
				if (c != '\\') return 0;
				if (*++filename != '\\') return 0; /* safe because *filename != '\0' */
				break;
			case '+':
				if (c < '0' || c > '9') return 0;
				break;
			case 'x':
				if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) return 0;
				break;
			case 'a':
				if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) return 0;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
				if (c == '\\' && (c = filename[1]) >= '0' && c <= '3' && c == *pattern
					&& (c = filename[2]) >= '0' && c <= '7' && c == pattern[1]
					&& (c = filename[3]) >= '0' && c <= '7' && c == pattern[2]) {
					filename += 3; /* safe because filename is \ooo */
					pattern += 2; /* safe because pattern is \ooo  */
					break;
				}
				return 0; /* Not matched. */
			case '*':
			case '@':
				{
					int i;
					for (i = 0; i <= filename_end - filename; i++) {
						if (FileMatchesToPattern(filename + i, filename_end, pattern + 1, pattern_end)) return 1;
						if ((c = filename[i]) == '.' && *pattern == '@') break;
						if (c == '\\') {
							if ((c = filename[i + 1]) == '\\') {
								i++; /* safe because filename is \\ */
							} else if (c >= '0' && c <= '3' && (c = filename[i + 2]) >= '0' && c <= '7' && (c = filename[i + 3]) >= '0' && c <= '7') {
								i += 3; /* safe because filename is \ooo */
							} else {
								break; /* Bad pattern. */
							}
						}
					}
					return 0; /* Not matched. */
				}
			default:
				{
					int i, j = 0;
					if ((c = *pattern) == '$') {
						while ((c = filename[j]) >= '0' && c <= '9') j++;
					} else if (c == 'X') {
						while (((c = filename[j]) >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) j++;
					} else if (c == 'A') {
						while (((c = filename[j]) >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) j++;
					}
					for (i = 1; i <= j; i++) {
						if (FileMatchesToPattern(filename + i, filename_end, pattern + 1, pattern_end)) return 1;
					}
				}
				return 0; /* Not matched or bad pattern. */
			}
			filename++; /* safe because *filename != '\0' */
			pattern++; /* safe because *pattern != '\0' */
		}
	}
	while (*pattern == '\\' && (*(pattern + 1) == '*' || *(pattern + 1) == '@')) pattern += 2;
	return (filename == filename_end && pattern == pattern_end);
}

/*
 * Check whether the given filename is patterened.
 * Returns nonzero if patterned, zero otherwise.
 */
static int PathContainsPattern(const char *filename) {
	if (filename) {
		char c, d, e;
		while ((c = *filename++) != '\0') {
			if (c != '\\') continue;
			switch (c = *filename++) {
			case '\\':  /* "\\" */
				continue;
			case '0':   /* "\ooo" */
			case '1':
			case '2':
			case '3':
				if ((d = *filename++) >= '0' && d <= '7' && (e = *filename++) >= '0' && e <= '7'
					&& (c != '0' || d != '0' || e != '0')) continue; /* pattern is not \000 */
			}
			return 1;
		}
	}
	return 0;
}

static int string_compare(const void *a, const void *b) {
	return strcmp(* (char **) a, * (char **) b);
}

static int string_acl_compare(const void *a, const void *b) {
	const char *a0 = * (char **) a;
	const char *b0 = * (char **) b;
	if (*a0 && *b0) return strcmp(a0 + 1, b0 + 1);
	return 0;
}

static char *ReadFile(const char *filename) {
	char *read_buffer = NULL;
	int fd;
	if ((fd = open(filename, O_RDONLY)) != EOF) {
		int read_buffer_len = 0;
		while (1) {
			char *cp = realloc(read_buffer, read_buffer_len + 4096);
			int len;
			if (!cp) {
				free(read_buffer);
				return NULL;
			}
			read_buffer = cp;
			len = read(fd, read_buffer + read_buffer_len, 4095);
			if (len <= 0) break;
			read_buffer_len += len;
		}
		close(fd);
		read_buffer[read_buffer_len] = '\0';
	}
	return read_buffer;
}

static char *FindConditionPart(char *data) {
	char *cp = strstr(data, " if "), *cp2;
	if (cp) {
		while ((cp2 = strstr(cp + 4, " if ")) != NULL) cp = cp2;
		*cp++ = '\0';
	}
	return cp;
}

static int parse_ulong(unsigned long *result, const char **str) {
	const char *cp = *str;
	char *ep;
	int base = 10;
	if (*cp == '0') {
		char c = * (cp + 1);
		if (c == 'x' || c == 'X') {
			base = 16; cp += 2;
		} else if (c >= '0' && c <= '7') {
			base = 8; cp++;
		}
	}
	*result = strtoul(cp, &ep, base);
	if (cp == ep) return 0;
	*str = ep;
	return (base == 16 ? VALUE_TYPE_HEXADECIMAL : (base == 8 ? VALUE_TYPE_OCTAL : VALUE_TYPE_DECIMAL));
}

static pid_t GetPPID(const pid_t pid) {
	char buffer[1024];
	FILE *fp;
	pid_t ppid = 1;
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/status", pid);
	if ((fp = fopen(buffer, "r")) != NULL) {
		while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 1, fp)) {
			if (sscanf(buffer, "PPid: %u", &ppid) == 1) break;
		}
		fclose(fp);
	}
	return ppid;
}

static char *GetName(const pid_t pid) {
	char buffer[1024];
	FILE *fp;
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/status", pid);
	if ((fp = fopen(buffer, "r")) != NULL) {
		while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 1, fp)) {
			if (strncmp(buffer, "Name:", 5) == 0) {
				char *cp = buffer + 5;
				while (*cp == ' ' || *cp == '\t') cp++;
				memmove(buffer, cp, strlen(cp) + 1);
				if ((cp = strchr(buffer, '\n')) != NULL) *cp = '\0';
				break;
			}
		}
		fclose(fp);
		if (buffer[0]) return strdup(buffer);
	}
	return NULL;
}

/***** UTILITY FUNCTIONS END *****/

/***** STRUCTURES START *****/

typedef struct path_info {
    const char *name;
    u32 hash;        /* = full_name_hash(name, strlen(name)) */
    u16 total_len;   /* = strlen(name)                       */
    u16 const_len;   /* = const_part_length(name)            */
    u8 is_dir;       /* = strendswith(name, "/")             */
    u8 is_patterned; /* = PathContainsPattern(name)          */
    u16 depth;       /* = PathDepth(name)                    */
} PATH_INFO;

typedef struct {
    const struct path_info *group_name;
	const struct path_info **member_name;
	int member_name_len;
} GROUP_ENTRY;

typedef struct savename_entry {
	struct savename_entry *next;
	struct path_info entry;
} SAVENAME_ENTRY;

typedef struct free_memory_block_list {
	struct free_memory_block_list *next;
	char *ptr;
	int len;
} FREE_MEMORY_BLOCK_LIST;

typedef struct {
	char *pathname;
	char *real_pathname;
} DLL_PATHNAME_ENTRY;

typedef struct domain_initializer_entry {
    const struct path_info *domainname;    /* This may be NULL */
    const struct path_info *program;
    unsigned char is_not:1;
	unsigned char is_last_name:1;
	unsigned char is_oldstyle:1;
} DOMAIN_INITIALIZER_ENTRY;

typedef struct domain_keeper_entry {
    const struct path_info *domainname;
    const struct path_info *program;       /* This may be NULL */
	unsigned char is_not:1;
	unsigned char is_last_name:1;
} DOMAIN_KEEPER_ENTRY;

typedef struct domain_info {
	const struct path_info *domainname;
	const struct domain_initializer_entry *domain_initializer; /* This may be NULL */
	const struct domain_keeper_entry *domain_keeper;           /* This may be NULL */
	const struct path_info **string_ptr;
	int string_count;
	int number; /* domain number (-1 if is_domain_initializer_source or is_domain_deleted) */ 
	u8 profile;
	unsigned char is_domain_initializer_source:1;
	unsigned char is_domain_initializer_target:1;
	unsigned char is_domain_keeper:1;
	unsigned char is_domain_unreachable:1;
	unsigned char is_domain_deleted:1;
} DOMAIN_INFO;

typedef struct {
	pid_t pid;
	pid_t ppid;
	u8 done;
} TASK_ENTRY;

/***** STRUCTURES END *****/

/***** UTILITY FUNCTIONS START *****/

static inline int pathcmp(const struct path_info *a, const struct path_info *b) {
	return a->hash != b->hash || strcmp(a->name, b->name);
}

static void fill_path_info(struct path_info *ptr) {
    const char *name = ptr->name;
    const int len = strlen(name);
    ptr->total_len = len;
    ptr->const_len = const_part_length(name);
    ptr->is_dir = len && (name[len - 1] == '/');
    ptr->is_patterned = (ptr->const_len < len);
    ptr->hash = full_name_hash(name, len);
    ptr->depth = PathDepth(name);
}

static const struct path_info *SaveName(const char *name) {
	static FREE_MEMORY_BLOCK_LIST fmb_list = { NULL, NULL, 0 };
	static SAVENAME_ENTRY name_list[SAVENAME_MAX_HASH]; /* The list of names. */
	SAVENAME_ENTRY *ptr, *prev = NULL;
	unsigned int hash;
	FREE_MEMORY_BLOCK_LIST *fmb = &fmb_list;
	int len;
	static int first_call = 1;
	if (!name) return NULL;
	len = strlen(name) + 1;
	if (len > CCS_MAX_PATHNAME_LEN) {
		fprintf(stderr, "ERROR: Name too long for SaveName().\n");
		return NULL;
	}
	hash = full_name_hash((const unsigned char *) name, len - 1);
	if (first_call) {
		int i;
		first_call = 0;
		memset(&name_list, 0, sizeof(name_list));
		for (i = 0; i < SAVENAME_MAX_HASH; i++) {
			name_list[i].entry.name = "/";
			fill_path_info(&name_list[i].entry);
		}
		if (CCS_MAX_PATHNAME_LEN > PAGE_SIZE) abort();
	}
	ptr = &name_list[hash % SAVENAME_MAX_HASH];
	while (ptr) {
		if (hash == ptr->entry.hash && strcmp(name, ptr->entry.name) == 0) goto out;
		prev = ptr; ptr = ptr->next;
	}
	while (len > fmb->len) {
		if (fmb->next) {
			fmb = fmb->next;
		} else {
			char *cp;
			if ((cp = (char *) malloc(PAGE_SIZE)) == NULL || (fmb->next = (FREE_MEMORY_BLOCK_LIST *) alloc_element(sizeof(FREE_MEMORY_BLOCK_LIST))) == NULL) OutOfMemory();
			memset(cp, 0, PAGE_SIZE);
			fmb = fmb->next;
			fmb->ptr = cp;
			fmb->len = PAGE_SIZE;
		}
	}
	if ((ptr = (SAVENAME_ENTRY *) alloc_element(sizeof(SAVENAME_ENTRY))) == NULL) OutOfMemory();
	memset(ptr, 0, sizeof(SAVENAME_ENTRY));
	ptr->entry.name = fmb->ptr;
	memmove(fmb->ptr, name, len);
	fill_path_info(&ptr->entry);
	fmb->ptr += len;
	fmb->len -= len;
	prev->next = ptr; /* prev != NULL because name_list is not empty. */
	if (fmb->len == 0) {
		FREE_MEMORY_BLOCK_LIST *ptr = &fmb_list;
		while (ptr->next != fmb) ptr = ptr->next; ptr->next = fmb->next;
	}
 out:
	return ptr ? &ptr->entry : NULL;
}

static int PathMatchesToPattern(const struct path_info *pathname0, const struct path_info *pattern0) {
	//if (!pathname || !pattern) return 0;
	const char *pathname = pathname0->name, *pattern = pattern0->name;
	const int len = pattern0->const_len;
	if (!pattern0->is_patterned) return !pathcmp(pathname0, pattern0);
	if (pathname0->depth != pattern0->depth) return 0;
	if (strncmp(pathname, pattern, len)) return 0;
	pathname += len; pattern += len;
	while (*pathname && *pattern) {
		const char *pathname_delimiter = strchr(pathname, '/'), *pattern_delimiter = strchr(pattern, '/');
		if (!pathname_delimiter) pathname_delimiter = strchr(pathname, '\0');
		if (!pattern_delimiter) pattern_delimiter = strchr(pattern, '\0');
		if (!FileMatchesToPattern(pathname, pathname_delimiter, pattern, pattern_delimiter)) return 0;
		pathname = *pathname_delimiter ? pathname_delimiter + 1 : pathname_delimiter;
		pattern = *pattern_delimiter ? pattern_delimiter + 1 : pattern_delimiter;
	}
	while (*pattern == '\\' && (*(pattern + 1) == '*' || *(pattern + 1) == '@')) pattern += 2;
	return (!*pathname && !*pattern);
}

/***** UTILITY FUNCTIONS END *****/

/***** MAIN START *****/

static GROUP_ENTRY *group_list = NULL;
static int group_list_len = 0;

static DOMAIN_INFO *domain_list[2] = { NULL, NULL };
static int domain_list_count[2] = { 0, 0 };
static unsigned char *domain_list_selected = NULL;

static const char *DomainName(const int index) {
	return domain_list[0][index].domainname->name;
}

static const char *GetLastName(const int index) {
	const char *cp0 = DomainName(index), *cp1;
	if ((cp1 = strrchr(cp0, ' ')) != NULL) return cp1 + 1;
	return cp0;
}

static int AddStringEntry(const char *entry, const int index, const int type) {
	const struct path_info **acl_ptr;
	int acl_count;
	const struct path_info *cp;
	int i;
	if (index < 0 || index >= domain_list_count[type]) {
		fprintf(stderr, "AddStringEntry: ERROR: domain is out of range.\n");
		return -EINVAL;
	}
	if (!entry || !*entry) return -EINVAL;
	if ((cp = SaveName(entry)) == NULL) OutOfMemory();

	acl_ptr = domain_list[type][index].string_ptr;
	acl_count = domain_list[type][index].string_count;

	// Check for the same entry.
	for (i = 0; i < acl_count; i++) {
		// Faster comparison, for they are SaveName'd.
		if (cp == acl_ptr[i]) return 0;
	}

	if ((acl_ptr = (const struct path_info **) realloc(acl_ptr, (acl_count + 1) * sizeof(const struct path_info *))) == NULL) OutOfMemory();
	acl_ptr[acl_count++] = cp;
	domain_list[type][index].string_ptr = acl_ptr;
	domain_list[type][index].string_count = acl_count;
	return 0;
}

static int DelStringEntry(const char *entry, const int index, const int type) {
	const struct path_info **acl_ptr;
	int acl_count;
	const struct path_info *cp;
	int i;
	if (index < 0 || index >= domain_list_count[type]) {
		fprintf(stderr, "DelStringEntry: ERROR: domain is out of range.\n");
		return -EINVAL;
	}
	if (!entry || !*entry) return -EINVAL;
	if ((cp = SaveName(entry)) == NULL) OutOfMemory();

	acl_ptr = domain_list[type][index].string_ptr;
	acl_count = domain_list[type][index].string_count;

	for (i = 0; i < acl_count; i++) {
		// Faster comparison, for they are SaveName'd.
		if (cp != acl_ptr[i]) continue;
		domain_list[type][index].string_count--;
		for (; i < acl_count - 1; i++) acl_ptr[i] = acl_ptr[i + 1];
		return 0;
	}
	return -ENOENT;
}

static void ClearDomainPolicy(const int type) {
	int index;
	for (index = 0; index < domain_list_count[type]; index++) {
		free(domain_list[type][index].string_ptr);
		domain_list[type][index].string_ptr = NULL;
		domain_list[type][index].string_count = 0;
	}
	free(domain_list[type]);
	domain_list[type] = NULL;
	domain_list_count[type]= 0;
}

static int FindDomain(const char *domainname0, const int type, const int is_domain_initializer_source, const int is_domain_deleted) {
	int i;
	struct path_info domainname;
	domainname.name = domainname0;
	fill_path_info(&domainname);
	for (i = 0; i < domain_list_count[type]; i++) {
		if (domain_list[type][i].is_domain_initializer_source == is_domain_initializer_source && domain_list[type][i].is_domain_deleted == is_domain_deleted && !pathcmp(&domainname, domain_list[type][i].domainname)) return i;
	}
	return EOF;
}

static int FindOrAssignNewDomain(const char *domainname, const int type, const int is_domain_initializer_source, const int is_domain_deleted) {
	const struct path_info *saved_domainname;
	int index;
	if ((index = FindDomain(domainname, type, is_domain_initializer_source, is_domain_deleted)) == EOF) {
		if (IsCorrectDomain(domainname)) {
			if ((domain_list[type] = (DOMAIN_INFO *) realloc(domain_list[type], (domain_list_count[type] + 1) * sizeof(DOMAIN_INFO))) == NULL) OutOfMemory();
			memset(&domain_list[type][domain_list_count[type]], 0, sizeof(DOMAIN_INFO));
			if ((saved_domainname = SaveName(domainname)) == NULL) OutOfMemory();
			domain_list[type][domain_list_count[type]].domainname = saved_domainname;
			domain_list[type][domain_list_count[type]].is_domain_initializer_source = is_domain_initializer_source;
			domain_list[type][domain_list_count[type]].is_domain_deleted = is_domain_deleted;
			index = domain_list_count[type]++;
		} else {
			fprintf(stderr, "FindOrAssignNewDomain: Invalid domainname '%s'\n", domainname);
		}
	}
	return index;
}

static void DeleteDomain(const int index, const int type) {
	if (index > 0 && index < domain_list_count[type]) {
		int i;
		free(domain_list[type][index].string_ptr);
		for (i = index; i < domain_list_count[type] - 1; i++) domain_list[type][i] = domain_list[type][i + 1];
		domain_list_count[type]--;
	}
}

static int domainname_compare(const void *a, const void *b) {
	return strcmp(((DOMAIN_INFO *) a)->domainname->name, ((DOMAIN_INFO *) b)->domainname->name);
}

static int path_info_compare(const void *a, const void *b) {
	const char *a0 = (* (struct path_info **) a)->name;
	const char *b0 = (* (struct path_info **) b)->name;
	if (*a0 && *b0) return strcmp(a0 + 1, b0 + 1);
	return 0;
}

static void SortPolicy(const int type) {
	int i;
	qsort(domain_list[type], domain_list_count[type], sizeof(DOMAIN_INFO), domainname_compare);
	for (i = 0; i < domain_list_count[type]; i++) qsort(domain_list[type][i].string_ptr, domain_list[type][i].string_count, sizeof(struct path_info *), path_info_compare);
}

static int WriteDomainPolicy(const int fd, const int type) {
	int i, j;
	for (i = 0; i < domain_list_count[type]; i++) {
		const struct path_info **string_ptr = domain_list[type][i].string_ptr;
		const int string_count = domain_list[type][i].string_count;
		write(fd, domain_list[type][i].domainname->name, domain_list[0][i].domainname->total_len);
		write(fd, "\n\n", 2);
		for (j = 0; j < string_count; j++) {
			write(fd, string_ptr[j]->name, string_ptr[j]->total_len);
			write(fd, "\n", 1);
		}
		write(fd, "\n", 1);
	}
	return 0;
}

static int IsSameDomainList(void) {
	if (domain_list_count[0] == domain_list_count[1]) {
		int i, j;
		for (i = 0; i < domain_list_count[0]; i++) {
			const struct path_info **string_ptr = domain_list[0][i].string_ptr;
			const int string_count = domain_list[0][i].string_count;
			if (string_count == domain_list[1][i].string_count) {
				const struct path_info **ptr = domain_list[1][i].string_ptr;
				for (j = 0; j < string_count; j++) {
					/* Faster comparison, for they are SaveName'd and sorted pointers. */
					if (string_ptr[j] != ptr[j]) break;
				}
				if (j == string_count) continue;
			}
			break;
		}
		if (i == domain_list_count[0]) return 1;
	}
	return 0;
}

static int IsKeeperDomain(const int index) {
	return domain_list[0][index].is_domain_keeper;
}

static int IsInitializerSource(const int index) {
	return domain_list[0][index].is_domain_initializer_source;
}

static int IsInitializerTarget(const int index) {
	return domain_list[0][index].is_domain_initializer_target;
}

static int IsDomainUnreachable(const int index) {
	return domain_list[0][index].is_domain_unreachable;
}

static int IsDeletedDomain(const int index) {
	return domain_list[0][index].is_domain_deleted;
}

static const int shared_buffer_len = MAXBUFSIZE;
static char *shared_buffer = NULL;
static int buffer_lock = 0;
static void get(void) {
	if (buffer_lock) OutOfMemory();
	if (!shared_buffer && (shared_buffer = malloc(shared_buffer_len)) == NULL) OutOfMemory();
	buffer_lock++;
}
static void put(void) {
	if (buffer_lock != 1) OutOfMemory();
	buffer_lock--;
}

static int freadline(FILE *fp) {
	char *cp;
	memset(shared_buffer, 0, shared_buffer_len);
	if (fgets(shared_buffer, shared_buffer_len - 1, fp) == NULL ||
		(cp = strchr(shared_buffer, '\n')) == NULL) return 0;
	*cp = '\0';
	NormalizeLine(shared_buffer);
	return 1;
}

static void ReadDomainPolicy(const char *filename, const int type) {
	FILE *fp = stdin;
	int index;
	if (filename) {
		if ((fp = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "Can't open %s\n", filename);
			return;
		}
	}
	index = EOF;
	get();
	while (freadline(fp)) {
		if (IsDomainDef(shared_buffer)) {
			index = FindOrAssignNewDomain(shared_buffer, type, 0, 0);
		} else if (index >= 0 && shared_buffer[0]) {
			AddStringEntry(shared_buffer, index, type);
		}
	}
	put();
	if (fp != stdin) fclose(fp);
	SortPolicy(type);
}

/***** sortpolicy start *****/

static int sortpolicy_main(int argc, char *argv[]) {
	ReadDomainPolicy(NULL, 0);
	WriteDomainPolicy(1, 0);
	return 0;
}

/***** sortpolicy end *****/

/***** setprofile start *****/

static int setprofile_main(int argc, char *argv[]) {
	FILE *fp_in, *fp_out;
	int profile = 0;
	int recursive = 0;
	int i, start = 2;
	if (argc > 1 && strcmp(argv[1], "-r") == 0) {
		recursive = 1;
		start = 3;
	}
	if (argc <= start || sscanf(argv[start - 1], "%u", &profile) != 1) {
		fprintf(stderr, "%s [-r] profile domainname [domainname ...]\n", argv[0]);
		return 0;
	}
	for (i = start; i < argc; i++) NormalizeLine(argv[i]);
	{
		const int fd = open("/proc/ccs/policy/.domain_status", O_RDWR);
		if (fd == EOF) {
			fprintf(stderr, "You can't run this daemon for this kernel.\n");
			return 1;
		} else if (write(fd, "", 0) != 0) {
			fprintf(stderr, "You need to register this program to /proc/ccs/policy/manager to run this program.\n");
			return 1;
		}
		close(fd);
	}
	{
		int profile_found = 0;
		if ((fp_in = fopen("/proc/ccs/status", "r")) == NULL) {
			fprintf(stderr, "Can't open policy file.\n");
			exit(1);
		}
		get();
		while (freadline(fp_in)) {
			if (atoi(shared_buffer) == profile) {
				profile_found = 1;
				break;
			}
		}
		put();
		fclose(fp_in);
		if (!profile_found) {
			fprintf(stderr, "Profile %u not defined.\n", profile);
			exit(1);
		}
	}
	if ((fp_in = fopen("/proc/ccs/policy/.domain_status", "r")) == NULL || (fp_out = fopen("/proc/ccs/policy/.domain_status", "w")) == NULL) {
		fprintf(stderr, "Can't open policy file.\n");
		exit(1);
	}
	get();
	while (freadline(fp_in)) {
		char *cp = strchr(shared_buffer, ' ');
		if (!cp) break;
		*cp++ = '\0';
		for (i = start; i < argc; i++) {
			const int len = strlen(argv[i]);
			if (strncmp(cp, argv[i], len)) continue;
			if (!recursive) {
				if (cp[len]) continue;
			} else {
				if (cp[len] && cp[len] != ' ') continue;
			}
			fprintf(fp_out, "%d %s\n", profile, cp);
			printf("%d %s\n", profile, cp);
		}
	}
	put();
	fclose(fp_in); fclose(fp_out);
	return 0;
}

/***** setprofile end *****/

/***** setlevel start *****/

static int setlevel_main(int argc, char *argv[]) {
	static const char *policy_file = "/proc/ccs/status";
	int i, fd;
	char c;
	if (access("/proc/ccs/", F_OK)) {
		fprintf(stderr, "You can't use this command for this kernel.\n");
		return 1;
	}
	if ((fd = open(policy_file, O_RDWR)) == EOF) {
		fprintf(stderr, "Can't open %s\n", policy_file);
		return 1;
	} else if (write(fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to /proc/ccs/policy/manager to run this program.\n");
		return 1;
	}
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			write(fd, argv[i], strlen(argv[i])); write(fd, "\n", 1);
		}
	}
	printf("<<< Access Control Status >>>\n");
	while (read(fd, &c, 1) == 1) putchar(c);
	close(fd);
	return 0;
}

/***** setlevel end *****/

/***** savepolicy start *****/

static int savepolicy_main(int argc, char *argv[]) {
	int remount_root = 0;
	char filename[1024];
	int save_system_policy = 0;
	int save_exception_policy = 0;
	int save_domain_policy = 0;
	int force_save = 0;
	int repeat;
	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	memset(filename, 0, sizeof(filename));
	if (access("/proc/self/", F_OK)) mount("/proc", "/proc", "proc", 0, NULL);
	if (access("/proc/ccs/policy/", F_OK)) {
		fprintf(stderr, "You can't run this program for this kernel.\n");
		return 0;
	}
	if (argc == 1) {
		force_save = save_system_policy = save_exception_policy = save_domain_policy = 1;
	} else {
		int i;
		for (i = 1; i < argc; i++) {
			char *p = argv[i];
			char *s = strchr(p, 's');
			char *e = strchr(p, 'e');
			char *d = strchr(p, 'd');
			char *a = strchr(p, 'a');
			char *f = strchr(p, 'f');
			if (s || a) save_system_policy = 1;
			if (e || a) save_exception_policy = 1;
			if (d || a) save_domain_policy = 1;
			if (f) force_save = 1;
			if (strcspn(p, "sedaf")) {
				printf("%s [s][e][d][a][f]\n"
					   "s : Save system_policy.\n"
					   "e : Save exception_policy.\n"
					   "d : Save domain_policy.\n"
					   "a : Save all policies.\n"
					   "f : Save even if on-disk policy and on-memory policy are the same.\n\n"
					   "If no options given, this program assumes 'a' and 'f' are given.\n", argv[0]);
				return 0;
			}
		}
	}
	if (chdir("/etc/ccs/")) {
		printf("Directory /etc/ccs/ doesn't exist.\n");
		return 1;
	}
	if (access(".", W_OK) == EOF) {
		if (errno != EROFS || mount("/", "/", "rootfs", MS_REMOUNT, NULL) == EOF) {
			printf("Can't remount for read-write. (%s)\n", strerror(errno));
			return 1;
		}
		remount_root = 1;
	}

	/* Exclude nonexistent policy. */
	if (access("/proc/ccs/policy/system_policy", R_OK)) save_system_policy = 0;
	if (access("/proc/ccs/policy/exception_policy", R_OK)) save_exception_policy = 0;
	if (access("/proc/ccs/policy/domain_policy", R_OK)) save_domain_policy = 0;

	/* Repeat twice so that necessary permissions for this program are included in domain policy. */
	for (repeat = 0; repeat < 2; repeat++) {

		if (save_system_policy) {
			char *new_policy = ReadFile("/proc/ccs/policy/system_policy");
			char *old_policy = ReadFile("system_policy.txt");
			if (new_policy && (force_save || !old_policy || strcmp(new_policy, old_policy))) {
				int fd;
				snprintf(filename, sizeof(filename) - 1, "system_policy.%02d-%02d-%02d.%02d:%02d:%02d.txt", tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
				if ((fd = open(filename, O_WRONLY | O_CREAT, 0600)) != EOF) {
					ftruncate(fd, 0);
					write(fd, new_policy, strlen(new_policy));
					close(fd);
					unlink("system_policy.txt");
					symlink(filename, "system_policy.txt");
				} else {
					printf("Can't create %s\n", filename);
				}
			}
			free(old_policy);
			free(new_policy);
		}
		
		if (save_exception_policy) {
			char *new_policy = ReadFile("/proc/ccs/policy/exception_policy");
			char *old_policy = ReadFile("exception_policy.txt");
			if (new_policy && (force_save || !old_policy || strcmp(new_policy, old_policy))) {
				int fd;
				snprintf(filename, sizeof(filename) - 1, "exception_policy.%02d-%02d-%02d.%02d:%02d:%02d.txt", tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
				if ((fd = open(filename, O_WRONLY | O_CREAT, 0600)) != EOF) {
					ftruncate(fd, 0);
					write(fd, new_policy, strlen(new_policy));
					close(fd);
					unlink("exception_policy.txt");
					symlink(filename, "exception_policy.txt");
				} else {
					printf("Can't create %s\n", filename);
				}
			}
			free(old_policy);
			free(new_policy);
		}

	}
	
	if (save_domain_policy) {
		ReadDomainPolicy("/proc/ccs/policy/domain_policy", 0);
		for (repeat = 0; repeat < 10; repeat++) {
			//if (repeat) printf("Domain policy has changed while saving domain policy. Retrying.\n");
			if (access("domain_policy.txt", R_OK) == 0) ReadDomainPolicy("domain_policy.txt", 1);
			/* Need to save domain policy? */
			if (force_save || !IsSameDomainList()) {
				int fd;
				snprintf(filename, sizeof(filename) - 1, "domain_policy.%02d-%02d-%02d.%02d:%02d:%02d.txt", tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
				if ((fd = open(filename, O_WRONLY | O_CREAT, 0600)) != EOF) {
					ftruncate(fd, 0);
					WriteDomainPolicy(fd, 0);
					close(fd);
					unlink("domain_policy.txt");
					symlink(filename, "domain_policy.txt");
				} else {
					printf("Can't create %s\n", filename);
				}
			}
			/* Has domain policy changed while saving domain policy? */
			ClearDomainPolicy(0);
			ReadDomainPolicy("/proc/ccs/policy/domain_policy", 0);
			if (IsSameDomainList()) break;
			ClearDomainPolicy(1);
		}
		ClearDomainPolicy(0);
		ClearDomainPolicy(1);
	}
	if (remount_root) mount("/", "/", "rootfs", MS_REMOUNT | MS_RDONLY, NULL);
	return 0;
}

/***** savepolicy end *****/

/***** pathmatch start *****/

static int print_path_needs_separator = 0;

static void print_path(const char *dir, const char *file, const char *trailer) {
	if (print_path_needs_separator) putchar(' ');
	print_path_needs_separator = 1;
	fprintf_encoded(stdout, dir);
	fprintf_encoded(stdout, file);
	fprintf_encoded(stdout, trailer);
}

static char *scandir_filter_current_part = NULL;

static int scandir_filter(const struct dirent *buf) {
	char buffer[1024];
	char c;
	char *dp = buffer;
	const char *cp = buf->d_name;
	if (buf->d_type == DT_DIR) {
		if (strcmp(cp, ".") == 0 || strcmp(cp, "..") == 0) return 0;
	}
	if (strlen(cp) > 255) return 0;
	while ((c = *cp++) != '\0') {
		if (c == '\\') {
			*dp++ = '\\';
			*dp++ = '\\';
		} else if (c > ' ' && c < 127) {
			*dp++ = c;
		} else {
			*dp++ = '\\';
			*dp++ = ((c >> 6) + '0');
			*dp++ = (((c >> 3) & 7) + '0');
			*dp++ = ((c & 7) + '0');
		}
	}
	*dp = '\0';
	//printf("Compare: %s %s\n", buffer, scandir_filter_current_part);
	if (FileMatchesToPattern(buffer, dp, scandir_filter_current_part, strchr(scandir_filter_current_part, '\0'))) return 1;
	return 0;
}

static int scandir_target_is_dir = 0; 
static int scandir_target_depth = 0;
static char **scandir_target_part = NULL;

static void ScanDir(const char *path, int depth) {
	struct dirent **namelist;
	int i, n;
	scandir_filter_current_part = scandir_target_part[depth];
	//printf("Scan: %d %s\n", depth, scandir_filter_current_part);
	if ((n = scandir(path, &namelist, scandir_filter, 0)) >= 0) {	
		for (i = 0; i < n; i++) {
			const char *cp = namelist[i]->d_name;
			const unsigned char type = namelist[i]->d_type;
			if (depth < scandir_target_depth - 1) {
				if (type == DT_DIR) {
					const int len = strlen(path) + strlen(cp) + 4;
					char *child_path = malloc(len);
					if (!child_path) OutOfMemory();
					snprintf(child_path, len - 1, "%s%s/", path, cp);
					//printf("Check: %s\n", child_path);
					ScanDir(child_path, depth + 1);
					free(child_path);
				}
			} else if (scandir_target_is_dir) {
				if (type == DT_DIR) {
					print_path(path, cp, "/");
				}
			} else if (type != DT_DIR) {
				print_path(path, cp, "");
			}
			free((void *) namelist[i]);
		}
		free((void *) namelist);
	}
}

static void do_pathmatch_main(char *target) {
	if (strcmp(target, "/") == 0) {
		printf("/\n");
	} else if (target[0] != '/') {
		putchar('\n');
	} else {
		char *cp;
		int i;
		scandir_target_is_dir = (*(strchr(target, '\0') - 1) == '/');
		scandir_target_depth = 0;
		cp = target + 1;
		for (i = 1; ; i++) {
			char c = target[i];
			if (c == '/' || c == '\0') {
				target[i] = '\0';
				scandir_target_part = (char **) realloc(scandir_target_part, (scandir_target_depth + 1) * sizeof(char *));
				if (target + i != cp) scandir_target_part[scandir_target_depth++] = cp;
				cp = target + i + 1;
				if (!c) break;
			}
		}
		//for (i = 0; i < target_depth; i++) printf("%d %s\n", i, scandir_target_part[i]);
		print_path_needs_separator = 0;
		ScanDir("/", 0);
		putchar('\n');
	}
}

static int pathmatch_main(int argc, char *argv[]) {
	if (argc > 1) {
		int i;
		for (i = 1; i < argc; i++) do_pathmatch_main(argv[i]);
	} else {
		get();
		while (freadline(stdin)) do_pathmatch_main(shared_buffer);
		put();
	}
	return 0;
}

/***** pathmatch end *****/

/***** loadpolicy start *****/

static int loadpolicy_main(int argc, char *argv[]) {
	int load_system_policy = 0;
	int load_exception_policy = 0;
	int load_domain_policy = 0;
	int refresh_policy = 0;
	if (access("/proc/ccs/policy/", F_OK)) {
		fprintf(stderr, "You can't run this program for this kernel.\n");
		return 0;
	}
	if (argc == 1) {
		goto usage;
	} else {
		int i;
		for (i = 1; i < argc; i++) {
			char *p = argv[i];
			char *s = strchr(p, 's');
			char *e = strchr(p, 'e');
			char *d = strchr(p, 'd');
			char *a = strchr(p, 'a');
			char *f = strchr(p, 'f');
			if (s || a) load_system_policy = 1;
			if (e || a) load_exception_policy = 1;
			if (d || a) load_domain_policy = 1;
			if (f) refresh_policy = 1;
			if (strcspn(p, "sedaf")) {
			usage: ;
				printf("%s [s][e][d][a][f]\n"
					   "s : Load system_policy.\n"
					   "e : Load exception_policy.\n"
					   "d : Load domain_policy.\n"
					   "a : Load all policies.\n"
					   "f : Delete on-memory policy before loading on-disk policy.\n\n", argv[0]);
				printf("Note that 'accept mode' might be unable to append permissions for this program if 'f' is used "
					   "because 'f' might delete 'domain for this program' and 'execute permission for parent domain'.\n");
				return 0;
			}
		}
	}
	if (chdir("/etc/ccs/")) {
		printf("Directory /etc/ccs/ doesn't exist.\n");
		return 1;
	}

	if (load_system_policy) {
		FILE *file_fp, *proc_fp;
		if ((file_fp = fopen("system_policy.txt", "r")) == NULL) {
			fprintf(stderr, "Can't open system_policy.txt\n");
			goto out_system;
		}
		if ((proc_fp = fopen("/proc/ccs/policy/system_policy", "w")) == NULL) {
			fprintf(stderr, "Can't open /proc/ccs/policy/system_policy\n");
			fclose(file_fp);
			goto out_system;
		}
		if (refresh_policy) {
			FILE *proc_clear_fp = fopen("/proc/ccs/policy/system_policy", "r");
			if (!proc_clear_fp) {
				fprintf(stderr, "Can't open /proc/ccs/policy/system_policy\n");
				fclose(file_fp);
				fclose(proc_fp);
				goto out_system;
			}
			get();
			while (freadline(proc_clear_fp)) {
				if (shared_buffer[0]) fprintf(proc_fp, "delete %s\n", shared_buffer);
			}
			put();
			fclose(proc_clear_fp);
			fflush(proc_fp);
		}
		get();
		while (freadline(file_fp)) {
			if (shared_buffer[0]) fprintf(proc_fp, "%s\n", shared_buffer);
		}
		put();
		fclose(proc_fp);
		fclose(file_fp);
	}
 out_system: ;
	
	if (load_exception_policy) {
		FILE *file_fp, *proc_fp;
		if ((file_fp = fopen("exception_policy.txt", "r")) == NULL) {
			fprintf(stderr, "Can't open exception_policy.txt\n");
			goto out_exception;
		}
		if ((proc_fp = fopen("/proc/ccs/policy/exception_policy", "w")) == NULL) {
			fprintf(stderr, "Can't open /proc/ccs/policy/exception_policy\n");
			fclose(file_fp);
			goto out_exception;
		}
		if (refresh_policy) {
			FILE *proc_clear_fp = fopen("/proc/ccs/policy/exception_policy", "r");
			if (!proc_clear_fp) {
				fprintf(stderr, "Can't open /proc/ccs/policy/exception_policy\n");
				fclose(file_fp);
				fclose(proc_fp);
				goto out_exception;
			}
			get();
			while (freadline(proc_clear_fp)) {
				if (shared_buffer[0]) fprintf(proc_fp, "delete %s\n", shared_buffer);
			}
			put();
			fclose(proc_clear_fp);
			fflush(proc_fp);
		}
		get();
		while (freadline(file_fp)) {
			if (shared_buffer[0]) fprintf(proc_fp, "%s\n", shared_buffer);
		}
		put();
		fclose(proc_fp);
		fclose(file_fp);
	}
 out_exception: ;

	if (load_domain_policy) {
		int new_index;
		FILE *proc_fp = fopen("/proc/ccs/policy/domain_policy", "w");
		struct path_info reserved;
		reserved.name = "";
		fill_path_info(&reserved);
		if (!proc_fp) {
			fprintf(stderr, "Can't open /proc/ccs/policy/domain_policy\n");
			goto out_domain;
		}
		ReadDomainPolicy("domain_policy.txt", 0);
		ReadDomainPolicy("/proc/ccs/policy/domain_policy", 1);
		if (domain_list_count[0] == 0) {
			fprintf(stderr, "Can't open domain_policy.txt\n");
			fclose(proc_fp);
			goto out_domain;
		}
		for (new_index = 0; new_index < domain_list_count[0]; new_index++) {
			const char *domainname = DomainName(new_index);
			const struct path_info **new_string_ptr = domain_list[0][new_index].string_ptr;
			const int new_string_count = domain_list[0][new_index].string_count;
			const int old_index = FindDomain(domainname, 1, 0, 0);
			int i, j;
			if (refresh_policy && old_index >= 0) {
				/* Old policy for this domain found. */
				const struct path_info **old_string_ptr = domain_list[1][old_index].string_ptr;
				const int old_string_count = domain_list[1][old_index].string_count;
				fprintf(proc_fp, "select %s\n", domainname);
				for (j = 0; j < old_string_count; j++) {
					for (i = 0; i < new_string_count; i++) {
						if (new_string_ptr[i] == old_string_ptr[j]) break;
					}
					/* Delete this entry from old policy if not found in new policy. */
					if (i == new_string_count) fprintf(proc_fp, "delete %s\n", old_string_ptr[j]->name);
				}
			} else {
				/* Old policy for this domain not found or Append to old policy. */
				fprintf(proc_fp, "%s\n", domainname);
			}
			for (i = 0; i < new_string_count; i++) fprintf(proc_fp, "%s\n", new_string_ptr[i]->name);
			if (old_index >= 0) domain_list[1][old_index].domainname = &reserved; /* Don't delete this domain later. */
		}
		if (refresh_policy) {
			int old_index;
			/* Delete all domains that are not defined in new policy. */
			for (old_index = 0; old_index < domain_list_count[1]; old_index++) {
				if (domain_list[1][old_index].domainname != &reserved) fprintf(proc_fp, "delete %s\n", domain_list[1][old_index].domainname->name);
			}
		}
		fclose(proc_fp);
	}
 out_domain: ;

	return 0;
}

/***** loadpolicy end *****/

/***** ld-watch start *****/

static int ldwatch_main(int argc, char *argv[]) {
	DLL_PATHNAME_ENTRY *entry_list = NULL;
	int entry_list_count = 0;
	FILE *fp_policy;
	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		printf("Usage: %s file_to_exclude1 [file_to_exclude2 [...]]\n\n", argv[0]);
		printf("This program automatically registers files shown by 'ldconfig -NXp' as globally readable files.\n");
		printf("This program registers all files shown by 'ldconfig -NXp' by default, but you can specify files that you don't want to register by command line.\n");
		printf("For example, if you invoke\n");
		printf("  %s /lib/libcustom-1.0.0.so /lib/libcustom.so.1\n", argv[0]);
		printf("then, /lib/libcustom-1.0.0.so and /lib/libcustom.so.1 will be excluded from the result of 'ldconfig -NXp'.\n\n");
		printf("Start this program in one window, then update packages in another window.\n");
		printf("After you finished updating, wait for several seconds and terminate this program with 'Ctrl-C'.\n");
		return 0;
	}
	{
		const int fd = open("/proc/ccs/policy/exception_policy", O_RDWR);
		if (fd == EOF) {
			fprintf(stderr, "You can't run this daemon for this kernel.\n");
			return 1;
		} else if (write(fd, "", 0) != 0) {
			fprintf(stderr, "You need to register this program to /proc/ccs/policy/manager to run this program.\n");
			return 1;
		}
		close(fd);
	}
	if ((fp_policy = fopen("/proc/ccs/policy/exception_policy", "w")) == NULL) {
		fprintf(stderr, "Can't open policy file.\n");
		exit(1);
	}
	while (1) {
		struct stat64 buf;
		static time_t last_modified = 0;
		int i;
		if (stat64("/etc/ld.so.cache", &buf) == 0 && buf.st_mtime != last_modified) {
			FILE *fp_ldconfig;
			if ((fp_ldconfig = popen("/sbin/ldconfig -NXp", "r")) != NULL) {
				char buffer[16384];
				last_modified = buf.st_mtime;
				while (memset(buffer, 0, sizeof(buffer)), fgets(buffer, sizeof(buffer) - 1, fp_ldconfig)) {
					char *cp, *pathname, *real_pathname;
					if ((cp = strchr(buffer, '\n')) == NULL) continue;
					*cp = '\0';
					cp = strrchr(buffer, ' ');
					if (!cp || *++cp != '/') continue;
					// Check for duplicated entries.
					if ((real_pathname = realpath(cp, NULL)) == NULL) continue;
					for (i = 0; i < entry_list_count; i++) {
						if (strcmp(entry_list[i].real_pathname, real_pathname) == 0) break;
					}
					if (i < entry_list_count) {
						free(real_pathname);
						continue;
					}
					// Exclude if listed by command line.
					for (i = 1; i < argc; i++) {
						if (strcmp(argv[i], real_pathname) == 0 || strcmp(argv[i], cp) == 0) break;
					}
					if (i < argc) {
						printf("Skipped %s : %s\n", cp, real_pathname);
						free(real_pathname);
						continue;
					}
					// Add an entry.
					pathname = strdup(cp);
					entry_list = (DLL_PATHNAME_ENTRY *) realloc(entry_list, (entry_list_count + 1) * sizeof(DLL_PATHNAME_ENTRY));
					entry_list[entry_list_count].pathname = pathname;
					entry_list[entry_list_count++].real_pathname = real_pathname;
					printf("Added %s : %s\n", pathname, real_pathname);
					fprintf(fp_policy, KEYWORD_ALLOW_READ);
					fprintf_encoded(fp_policy, real_pathname);
					fprintf(fp_policy, "\n");
					fflush(fp_policy);
				}
				pclose(fp_ldconfig);
			}
			printf("Monitoring %d files.\n", entry_list_count);
		}
		// Check entries for update.
		for (i = 0; i < entry_list_count; i++) {
			DLL_PATHNAME_ENTRY *ptr = &entry_list[i];
			char *real_pathname = realpath(ptr->pathname, NULL);
			if (real_pathname && strcmp(ptr->real_pathname, real_pathname)) {
				printf("Changed %s : %s -> %s\n", ptr->pathname, ptr->real_pathname, real_pathname);
				fprintf(fp_policy, KEYWORD_ALLOW_READ);
				fprintf_encoded(fp_policy, real_pathname);
				fprintf(fp_policy, "\n");
				fflush(fp_policy);
				free(ptr->real_pathname); ptr->real_pathname = real_pathname; real_pathname = NULL;
			}
			free(real_pathname);
		}
		sleep(1);
	}
	fclose(fp_policy);
	return 0;
}

/***** ld-watch end *****/

/***** findtemp start *****/

static int findtemp_main(int argc, char *argv[]) {
	const char **pattern_list = NULL;
	int pattern_list_count = 0;
	int i;
	char buffer[16384], buffer2[sizeof(buffer)];
	if (argc > 1) {
		if (strcmp(argv[1], "--all")) {
			printf("%s < domain_policy\n\n", argv[0]);
			return 0;
		}
	}
	while (memset(buffer, 0, sizeof(buffer)), fscanf(stdin, "%16380s", buffer) == 1) {
		if (buffer[0] != '/') continue;
		{
			struct stat64 buf;
			if (!decode(buffer, buffer2)) continue;
			if (lstat64(buffer2, &buf) == 0) continue;
		}
		for (i = 0; i < pattern_list_count; i++) {
			if (strcmp(pattern_list[i], buffer) == 0) break;
		}
		if (i < pattern_list_count) continue;
		if ((pattern_list = (const char **) realloc(pattern_list, sizeof(const char *) * (pattern_list_count + 1))) == NULL ||
			(pattern_list[pattern_list_count++] = strdup(buffer)) == NULL) {
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}
	}
	qsort(pattern_list, pattern_list_count, sizeof(char *), string_compare);
	for (i = 0; i < pattern_list_count; i++) printf("%s\n", pattern_list[i]);
	return 0;
}

/***** findtemp end *****/

/***** editpolicy start *****/

static const char *policy_file = DOMAIN_POLICY_FILE;
static const char *list_caption = NULL;
static char *current_domain = NULL;

static int current_screen = SCREEN_DOMAIN_LIST;

// List for generic policy.
static char **generic_acl_list = NULL;
static int generic_acl_list_count = 0;
static unsigned char *generic_acl_list_selected = NULL;

static DOMAIN_KEEPER_ENTRY *domain_keeper_list = NULL;
static int domain_keeper_list_len = 0;
static DOMAIN_INITIALIZER_ENTRY *domain_initializer_list = NULL;
static int domain_initializer_list_len = 0;

///////////////////////////  ACL HANDLER  //////////////////////////////

static const DOMAIN_KEEPER_ENTRY *IsDomainKeeper(const struct path_info *domainname, const char *program) {
	int i;
	const DOMAIN_KEEPER_ENTRY *flag = NULL;
	struct path_info last_name;
	if ((last_name.name = strrchr(domainname->name, ' ')) != NULL) last_name.name++;
	else last_name.name = domainname->name;
	fill_path_info(&last_name);
	for (i = 0; i < domain_keeper_list_len; i++) {
		DOMAIN_KEEPER_ENTRY *ptr = &domain_keeper_list[i];
		if (!ptr->is_last_name) {
			if (pathcmp(ptr->domainname, domainname)) continue;
		} else {
			if (pathcmp(ptr->domainname, &last_name)) continue;
		}
		if (ptr->program && strcmp(ptr->program->name, program)) continue;
        if (ptr->is_not) return NULL;
        flag = ptr;
	}
	return flag;
}

static const DOMAIN_INITIALIZER_ENTRY *IsDomainInitializer(const struct path_info *domainname, const char *program) {
	int i;
	const DOMAIN_INITIALIZER_ENTRY *flag = NULL;
	if (strcmp(domainname->name, ROOT_NAME)) {
		struct path_info last_name;
		if ((last_name.name = strrchr(domainname->name, ' ')) != NULL) last_name.name++;
		else last_name.name = domainname->name;
		fill_path_info(&last_name);
		for (i = 0; i < domain_initializer_list_len; i++) {
			DOMAIN_INITIALIZER_ENTRY *ptr = &domain_initializer_list[i];
			if (ptr->domainname) {
				if (!ptr->is_last_name) {
					if (pathcmp(ptr->domainname, domainname)) continue;
				} else {
					if (pathcmp(ptr->domainname, &last_name)) continue;
				}
			}
			if (strcmp(ptr->program->name, program)) continue;
			if (ptr->is_not) return NULL;
			flag = ptr;
		}
	}
	return flag;
}

///////////////////////////  UTILITY FUNCTIONS  //////////////////////////////

static int offline_mode = 0;
static int persistent_fd = EOF;

static void SendFD(char *data, int *fd) {
	struct msghdr msg;
	struct iovec iov = { data, strlen(data) };
	char cmsg_buf[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *cmsg = (struct cmsghdr *) cmsg_buf; 
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsg_buf;
	msg.msg_controllen = sizeof(cmsg_buf);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	msg.msg_controllen = cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	memmove(CMSG_DATA(cmsg), fd, sizeof(int));
	sendmsg(persistent_fd, &msg, 0);
	close(*fd);
}

static FILE *open_read(const char *filename) {
	if (offline_mode) {
		char request[1024];
		int fd[2];
		FILE *fp;
		if (socketpair(PF_UNIX, SOCK_STREAM, 0, fd)) {
			fprintf(stderr, "socketpair()\n");
			exit(1);
		}
		if (shutdown(fd[0], SHUT_WR) || (fp = fdopen(fd[0], "r")) == NULL) {
			close(fd[1]); close(fd[0]);
			exit(1);
		}
		memset(request, 0, sizeof(request));
		snprintf(request, sizeof(request) - 1, "GET %s", filename);
		SendFD(request, &fd[1]);
		return fp;
	} else {
		return fopen(filename, "r");
	}
}

static FILE *open_write(const char *filename) {
	if (offline_mode) {
		char request[1024];
		int fd[2];
		if (socketpair(PF_UNIX, SOCK_STREAM, 0, fd)) {
			fprintf(stderr, "socketpair()\n");
			exit(1);
		}
		if (shutdown(fd[0], SHUT_RD)) {
			close(fd[1]); close(fd[0]);
			exit(1);
		}
		memset(request, 0, sizeof(request));
		snprintf(request, sizeof(request) - 1, "POST %s", filename);
		SendFD(request, &fd[1]);
		return fdopen(fd[0], "w");
	} else {
		return fdopen(open(filename, O_WRONLY), "w");
	}
}

static void ReadGenericPolicy(void) {
	FILE *fp;
	while (generic_acl_list_count) free(generic_acl_list[--generic_acl_list_count]);
	if ((fp = open_read(policy_file)) != NULL) {
		if (current_screen == SCREEN_ACL_LIST) {
			int flag = 0;
			get();
			while (freadline(fp)) {
				if (IsDomainDef(shared_buffer)) {
					flag = strcmp(shared_buffer, current_domain) == 0 ? 1 : 0;
				} else if (flag && shared_buffer[0] && strncmp(shared_buffer, KEYWORD_USE_PROFILE, KEYWORD_USE_PROFILE_LEN)) {
					if ((generic_acl_list = (char **) realloc(generic_acl_list, (generic_acl_list_count + 1) * sizeof(char *))) == NULL
						|| (generic_acl_list[generic_acl_list_count++] = strdup(shared_buffer)) == NULL) OutOfMemory();
				}
			}
			put();
			qsort(generic_acl_list, generic_acl_list_count, sizeof(char *), string_acl_compare);
		} else {
			get();
			while (freadline(fp)) {
				if (!shared_buffer[0]) continue;
				if ((generic_acl_list = (char **) realloc(generic_acl_list, (generic_acl_list_count + 1) * sizeof(char *))) == NULL
					|| (generic_acl_list[generic_acl_list_count++] = strdup(shared_buffer)) == NULL) OutOfMemory();
			}
			put();
			qsort(generic_acl_list, generic_acl_list_count, sizeof(char *), string_compare);
		}
		fclose(fp);
	}
	generic_acl_list_selected = realloc(generic_acl_list_selected, generic_acl_list_count);
	if (generic_acl_list_count && !generic_acl_list_selected) OutOfMemory();
	memset(generic_acl_list_selected, 0, generic_acl_list_count);
}

static int AddDomainInitializerEntry(const char *domainname, const char *program, const int is_not, const int is_oldstyle) {
	DOMAIN_INITIALIZER_ENTRY *ptr;
	int is_last_name = 0;
	if (!IsCorrectPath(program, 1, 0, -1)) return -EINVAL;
	if (domainname) {
		if (IsCorrectPath(domainname, 1, -1, -1)) {
			is_last_name = 1;
		} else if (!IsCorrectDomain(domainname)) {
			return -EINVAL;
		}
	}
	if ((domain_initializer_list = (DOMAIN_INITIALIZER_ENTRY *) realloc(domain_initializer_list, (domain_initializer_list_len + 1) * sizeof(DOMAIN_INITIALIZER_ENTRY))) == NULL) OutOfMemory();
	ptr = &domain_initializer_list[domain_initializer_list_len++];
	memset(ptr, 0, sizeof(DOMAIN_INITIALIZER_ENTRY));
	if ((ptr->program = SaveName(program)) == NULL) OutOfMemory();
	if (domainname && (ptr->domainname = SaveName(domainname)) == NULL) OutOfMemory();
	ptr->is_not = is_not;
	ptr->is_last_name = is_last_name;
	ptr->is_oldstyle = is_oldstyle;
	return 0;
}

static int AddDomainInitializerPolicy(char *data, const int is_not, const int is_oldstyle) {
	char *cp = strstr(data, " from ");
    if (cp) {
        *cp = '\0';
        return AddDomainInitializerEntry(cp + 6, data, is_not, is_oldstyle);
    } else {
        return AddDomainInitializerEntry(NULL, data, is_not, is_oldstyle);
    }
}

static int AddDomainKeeperEntry(const char *domainname, const char *program, const int is_not) {
	DOMAIN_KEEPER_ENTRY *ptr;
	int is_last_name = 0;
	if (IsCorrectPath(domainname, 1, -1, -1)) {
		is_last_name = 1;
	} else if (!IsCorrectDomain(domainname)) {
		return -EINVAL;
	}
	if (program && !IsCorrectPath(program, 1, 0, -1)) return -EINVAL;
	if ((domain_keeper_list = (DOMAIN_KEEPER_ENTRY *) realloc(domain_keeper_list, (domain_keeper_list_len + 1) * sizeof(DOMAIN_KEEPER_ENTRY))) == NULL) OutOfMemory();
	ptr = &domain_keeper_list[domain_keeper_list_len++];
	memset(ptr, 0, sizeof(DOMAIN_KEEPER_ENTRY));
	if ((ptr->domainname = SaveName(domainname)) == NULL) OutOfMemory();
	if (program && (ptr->program = SaveName(program)) == NULL) OutOfMemory();
	ptr->is_not = is_not;
	ptr->is_last_name = is_last_name;
	return 0;
}

static int AddDomainKeeperPolicy(char *data, const int is_not) {
	char *cp = strstr(data, " from ");
    if (cp) {
        *cp = '\0';
        return AddDomainKeeperEntry(cp + 6, data, is_not);
    } else {
        return AddDomainKeeperEntry(data, NULL, is_not);
    }
}

static int AddGroupEntry(const char *group_name, const char *member_name, const int is_delete) {
	const struct path_info *saved_group_name, *saved_member_name;
	int i, j;
	GROUP_ENTRY *group = NULL;
	if (!IsCorrectPath(group_name, 0, 0, 0) ||
		!IsCorrectPath(member_name, 0, 0, 0)) return -EINVAL;
	if ((saved_group_name = SaveName(group_name)) == NULL ||
		(saved_member_name = SaveName(member_name)) == NULL) return -ENOMEM;
	for (i = 0; i < group_list_len; i++) {
		group = &group_list[i];
		if (saved_group_name != group->group_name) continue;
		for (j = 0; j < group->member_name_len; j++) {
			if (group->member_name[j] == saved_member_name) {
				if (is_delete) {
					while (j < group->member_name_len - 1) group->member_name[j] = group->member_name[j + 1];
					group->member_name_len--;
				} else {
					return 0;
				}
			}
		}
		break;
	}
	if (is_delete) return -ENOENT;
	if (i == group_list_len) {
		if ((group_list = (GROUP_ENTRY *) realloc(group_list, (group_list_len + 1) * sizeof(GROUP_ENTRY))) == NULL) OutOfMemory();
		group = &group_list[group_list_len++];
		memset(group, 0, sizeof(GROUP_ENTRY));
		group->group_name = saved_group_name;
	}
	if ((group->member_name = (const struct path_info **) realloc(group->member_name, (group->member_name_len + 1) * sizeof(const struct path_info *))) == NULL) OutOfMemory();
	group->member_name[group->member_name_len++] = saved_member_name;
	return 0;
}

static int AddGroupPolicy(char *data, const int is_delete) {
	char *cp = strchr(data, ' ');
	if (!cp) return -EINVAL;
	*cp++ = '\0';
	return AddGroupEntry(data, cp, is_delete);
}

static GROUP_ENTRY *FindGroup(const char *group_name) {
	int i;
	for (i = 0; i < group_list_len; i++) {
		if (strcmp(group_name, group_list[i].group_name->name) == 0) return &group_list[i];
	}
	return NULL;
}

static void AssignDomainInitializerSource(const struct path_info *domainname, const char *program) {
	if (IsDomainInitializer(domainname, program)) {
		get();
		memset(shared_buffer, 0, shared_buffer_len);
		snprintf(shared_buffer, shared_buffer_len - 1, "%s %s", domainname->name, program);
		NormalizeLine(shared_buffer);
		if (FindOrAssignNewDomain(shared_buffer, 0, 1, 0) == EOF) OutOfMemory();
		put();
	}
}

static int domainname_attribute_compare(const void *a, const void *b) {
	const int k = strcmp(((DOMAIN_INFO *) a)->domainname->name, ((DOMAIN_INFO *) b)->domainname->name);
	if (k > 0 || (k == 0 && ((DOMAIN_INFO *) a)->is_domain_initializer_source < ((DOMAIN_INFO *) b)->is_domain_initializer_source)) return 1;
	return k;
}

static int unnumbered_domain_count = 0;

static void ReadDomainAndExceptionPolicy(void) {
	FILE *fp;
	int i, j;
	ClearDomainPolicy(0);
	domain_keeper_list_len = 0;
	domain_initializer_list_len = 0;
	group_list_len = 0;
	FindOrAssignNewDomain(ROOT_NAME, 0, 0, 0);

	// Load domain_initializer list, domain_keeper list.
	if ((fp = open_read(EXCEPTION_POLICY_FILE)) != NULL) {
		get();
		while (freadline(fp)) {
			if (strncmp(shared_buffer, KEYWORD_INITIALIZER, KEYWORD_INITIALIZER_LEN) == 0) {
				AddDomainInitializerPolicy(shared_buffer + KEYWORD_INITIALIZER_LEN, 0, 1);
			} else if (strncmp(shared_buffer, KEYWORD_NO_INITIALIZER, KEYWORD_NO_INITIALIZER_LEN) == 0) {
				AddDomainInitializerPolicy(shared_buffer + KEYWORD_NO_INITIALIZER_LEN, 1, 1);
			} else if (strncmp(shared_buffer, KEYWORD_INITIALIZE_DOMAIN, KEYWORD_INITIALIZE_DOMAIN_LEN) == 0) {
				AddDomainInitializerPolicy(shared_buffer + KEYWORD_INITIALIZE_DOMAIN_LEN, 0, 0);
			} else if (strncmp(shared_buffer, KEYWORD_NO_INITIALIZE_DOMAIN, KEYWORD_NO_INITIALIZE_DOMAIN_LEN) == 0) {
				AddDomainInitializerPolicy(shared_buffer + KEYWORD_NO_INITIALIZE_DOMAIN_LEN, 1, 0);
			} else if (strncmp(shared_buffer, KEYWORD_KEEP_DOMAIN, KEYWORD_KEEP_DOMAIN_LEN) == 0) {
				AddDomainKeeperPolicy(shared_buffer + KEYWORD_KEEP_DOMAIN_LEN, 0);
			} else if (strncmp(shared_buffer, KEYWORD_NO_KEEP_DOMAIN, KEYWORD_NO_KEEP_DOMAIN_LEN) == 0) {
				AddDomainKeeperPolicy(shared_buffer + KEYWORD_NO_KEEP_DOMAIN_LEN, 1);
			} else if (strncmp(shared_buffer, KEYWORD_PATH_GROUP, KEYWORD_PATH_GROUP_LEN) == 0) {
				AddGroupPolicy(shared_buffer + KEYWORD_PATH_GROUP_LEN, 0);
			}
		}
		put();
		fclose(fp);
	}

	// Load all domain list.
	if ((fp = open_read(DOMAIN_POLICY_FILE)) != NULL) {
		int index = EOF;
		get();
		while (freadline(fp)) {
			char *cp, *cp2;
			unsigned int profile;
			if (IsDomainDef(shared_buffer)) {
				index = FindOrAssignNewDomain(shared_buffer, 0, 0, 0);
			} else if (index >= 0 && (atoi(shared_buffer) & 1) == 1 && (cp = strchr(shared_buffer, ' ')) != NULL) {
				cp++;
				if ((cp2 = strchr(cp, ' ')) != NULL) *cp2 = '\0';
				if (*cp == '@' || IsCorrectPath(cp, 1, 0, -1)) AddStringEntry(cp, index, 0);
			} else if (index >= 0 && sscanf(shared_buffer, "use_profile %u", &profile) == 1) {
				domain_list[0][index].profile = (unsigned char) profile;
			}
		}
		put();
		fclose(fp);
	}
	
	{
		int index, max_index = domain_list_count[0];
		
		// Find unreachable domains.
		for (index = 0; index < max_index; index++) {
			char *cp;
			get();
			memset(shared_buffer, 0, shared_buffer_len);
			snprintf(shared_buffer, shared_buffer_len - 1, "%s", DomainName(index));
			while ((cp = strrchr(shared_buffer, ' ')) != NULL) {
				const struct domain_initializer_entry *domain_initializer;
				const struct domain_keeper_entry *domain_keeper;
				struct path_info parent;
				*cp++ = '\0';
				parent.name = shared_buffer;
				fill_path_info(&parent);
				if ((domain_initializer = IsDomainInitializer(&parent, cp)) != NULL) {
					domain_list[0][index].domain_initializer = domain_initializer;
					domain_list[0][index].domain_keeper = NULL;
				} else if ((domain_keeper = IsDomainKeeper(&parent, cp)) != NULL) {
					domain_list[0][index].domain_initializer = NULL;
					domain_list[0][index].domain_keeper = domain_keeper;
				}
			}
			put();
			if (domain_list[0][index].domain_initializer || domain_list[0][index].domain_keeper) domain_list[0][index].is_domain_unreachable = 1;
		}
		
		// Find domain initializer target domains.
		for (index = 0; index < max_index; index++) {
			char *cp;
			if ((cp = strchr(DomainName(index), ' ')) != NULL && strchr(cp + 1, ' ') == NULL) {
				for (i = 0; i < domain_initializer_list_len; i++) {
					DOMAIN_INITIALIZER_ENTRY *ptr = &domain_initializer_list[i];
					if (ptr->is_not) continue;
					if (strcmp(ptr->program->name, cp + 1)) continue;
					domain_list[0][index].is_domain_initializer_target = 1;
				}
			}
		}

		// Find domain keeper domains.
		for (index = 0; index < max_index; index++) {
			for (i = 0; i < domain_keeper_list_len; i++) {
				DOMAIN_KEEPER_ENTRY *ptr = &domain_keeper_list[i];
				if (ptr->is_not) continue;
				if (!ptr->is_last_name) {
					if (pathcmp(ptr->domainname, domain_list[0][index].domainname)) continue;
				} else {
					char *cp = strrchr(domain_list[0][index].domainname->name, ' ');
					if (!cp || strcmp(ptr->domainname->name, cp + 1)) continue;
				}
				domain_list[0][index].is_domain_keeper = 1;
			}
		}

		// Create domain initializer source domains.
		for (index = 0; index < max_index; index++) {
			const struct path_info *domainname = domain_list[0][index].domainname;
			const struct path_info **string_ptr = domain_list[0][index].string_ptr;
			const int max_count = domain_list[0][index].string_count;
			for (i = 0; i < max_count; i++) {
				const struct path_info *cp = string_ptr[i];
				if (cp->name[0] == '@') {
					GROUP_ENTRY *group = FindGroup(cp->name + 1);
					if (group) {
						for (j = 0; j < group->member_name_len; j++) AssignDomainInitializerSource(domainname, group->member_name[j]->name);
					}
				} else {
					AssignDomainInitializerSource(domainname, cp->name);
				}
			}
		}

		// Create missing parent domains.
		for (index = 0; index < max_index; index++) {
			char *cp;
			get();
			memset(shared_buffer, 0, shared_buffer_len);
			snprintf(shared_buffer, shared_buffer_len - 1, "%s", DomainName(index));
			while ((cp = strrchr(shared_buffer, ' ')) != NULL) {
				*cp = '\0';
				if (FindDomain(shared_buffer, 0, 0, 0) != EOF) continue;
				if (FindOrAssignNewDomain(shared_buffer, 0, 0, 1) == EOF) OutOfMemory();
			}
			put();
		}

	}
	// Sort by domain name.
	qsort(domain_list[0], domain_list_count[0], sizeof(DOMAIN_INFO), domainname_attribute_compare);

	// Assign domain numbers.
	{
		int number = 0, index;
		unnumbered_domain_count= 0;
		for (index = 0; index < domain_list_count[0]; index++) {
			if (IsDeletedDomain(index) || IsInitializerSource(index)) {
				domain_list[0][index].number = -1;
				unnumbered_domain_count++;
			} else {
				domain_list[0][index].number = number++;
			}
		}
	}

	domain_list_selected = realloc(domain_list_selected, domain_list_count[0]);
	if (domain_list_count[0] && !domain_list_selected) OutOfMemory();
	memset(domain_list_selected, 0, domain_list_count[0]);
}

////////////////////////////  UI HANDLER  ////////////////////////////

static int getch0(void) {
	int c = getch();
	if (c == 127) c = KEY_BACKSPACE;
	//syslog(LOG_INFO, "getch0='%c' (%d)\n", c, c);
	return c;
}

static int getch2(void) {
	static int c0 = 0, c1 = 0, c2 = 0, c3 = 0, len = 0;
	if (len > 0) { c0 = c1; c1 = c2; c2 = c3; len--; return c0; }
	c0 = getch0(); if (c0 != 0x1B) return c0;
	c1 = getch0(); if (c1 != '[') { len = 1; return c0; }
	c2 = getch0(); if (c2 < '1' || c2 > '6') { len = 2; return c0; }
	c3 = getch0(); if (c3 != '~') { len = 3; return c0; }
	//syslog(LOG_INFO, "getch2='%c'\n", c2);
	switch (c2) {
	case '1':
		return KEY_HOME;
	case '2':
		return KEY_IC;
	case '3':
		return KEY_DC;
	case '4':
		return KEY_END;
	case '5':
		return KEY_PPAGE;
	case '6':
		return KEY_NPAGE;
	}
	return 0;
}

static int query_fd = EOF;
static char *initial_readline_data = NULL;
				
static char *simple_readline(const int start_y, const int start_x, const char *prompt, const char *history[], const int history_count, const int max_length, const int scroll_width) {
	const int prompt_len = prompt && *prompt ? strlen(prompt) : 0;
	int buffer_len = 0, line_pos = 0, cur_pos = 0, history_pos = 0, tmp_saved = 0;
	static char *buffer = NULL, *tmp_buffer = NULL;
	{
		int i;
		for (i = 0; i < history_count; i++) if (!history[i]) return NULL;
	}
	{
		char *tmp;
		tmp = realloc(buffer, max_length);
		if (!tmp) return NULL;
		buffer = tmp;
		tmp = realloc(tmp_buffer, max_length);
		if (!tmp) return NULL;
		tmp_buffer = tmp;
		memset(buffer, 0, max_length);
		memset(tmp_buffer, 0, max_length);
	}
	move(start_y, start_x);
	history_pos = history_count;
	if (initial_readline_data) {
		strncpy(buffer, initial_readline_data, max_length);
		buffer_len = strlen(buffer);
		ungetch(KEY_END);
	}
	while (1) {
		int window_width, window_height;
		int c, x, y, i;
		getmaxyx(stdscr, window_height, window_width);
		window_width -= prompt_len;
		getyx(stdscr, y, x);
		move(y, 0);
		while (cur_pos > window_width - 1) {
			cur_pos--;
			line_pos++;
		}
		if (prompt_len) printw("%s", prompt);
		for (i = line_pos; i < line_pos + window_width; i++) {
			if (i < buffer_len) addch(buffer[i]);
			else break;
		}
		clrtoeol();
		move(y, cur_pos + prompt_len);
		refresh();
		c = getch2();
		if (query_fd != EOF) write(query_fd, "\n", 1);
		if (c == 4) { /* Ctrl-D */
			if (!buffer_len) buffer_len = -1;
			break;
		} else if (c == KEY_IC) {
			scrollok(stdscr, 1);
			printw("\n");
			for (i = 0; i < history_count; i++) {
				printw("%d: '%s'\n", i, history[i]);
			}
			scrollok(stdscr, 0);
		} else if (c >= 0x20 && c <= 0x7E && buffer_len < max_length - 1) {
			for (i = buffer_len - 1; i >= line_pos + cur_pos; i--) buffer[i + 1] = buffer[i];
			buffer[line_pos + cur_pos] = c;
			buffer[++buffer_len] = '\0';
			if (cur_pos < window_width - 1) cur_pos++;
			else line_pos++;
		} else if (c == '\r') {
			break;
		} else if (c == KEY_BACKSPACE) {
			if (line_pos + cur_pos) {
				buffer_len--;
				for (i = line_pos + cur_pos - 1; i < buffer_len; i++) buffer[i] = buffer[i + 1];
				buffer[buffer_len] = '\0';
				if (line_pos >= scroll_width && cur_pos == 0) { line_pos -= scroll_width; cur_pos += scroll_width - 1; }
				else if (cur_pos) cur_pos--;
				else if (line_pos) line_pos--;
			}
		} else if (c == KEY_DC) {
			if (line_pos + cur_pos < buffer_len) {
				buffer_len--;
				for (i = line_pos + cur_pos; i < buffer_len; i++) buffer[i] = buffer[i + 1];
				buffer[buffer_len] = '\0';
			}
		} else if (c == KEY_UP) {
			if (history_pos) {
				if (!tmp_saved) {
					tmp_saved = 1;
					strncpy(tmp_buffer, buffer, max_length);
				}
				history_pos--;
				strncpy(buffer, history[history_pos], max_length);
				buffer_len = strlen(buffer);
				goto end_key;
			}
		} else if (c == KEY_DOWN) {
			if (history_pos < history_count - 1) {
				history_pos++;
				strncpy(buffer, history[history_pos], max_length);
				buffer_len = strlen(buffer);
				goto end_key;
			} else if (tmp_saved) {
				tmp_saved = 0;
				history_pos = history_count;
				strncpy(buffer, tmp_buffer, max_length);
				buffer_len = strlen(buffer);
				goto end_key;
			}
		} else if (c == KEY_HOME) {
			cur_pos = 0;
			line_pos = 0;
		} else if (c == KEY_END) {
		end_key: ;
			cur_pos = buffer_len;
			line_pos = 0;
			if (cur_pos > window_width - 1) {
				line_pos = buffer_len - (window_width - 1);
				cur_pos = window_width - 1;
			}
		} else if (c == KEY_LEFT) {
			if (line_pos >= scroll_width && cur_pos == 0) { line_pos -= scroll_width; cur_pos += scroll_width - 1; }
			else if (cur_pos) cur_pos--;
			else if (line_pos) line_pos--;
		} else if (c == KEY_RIGHT) {
			if (line_pos + cur_pos < buffer_len) {
				if (cur_pos < window_width - 1) cur_pos++;
				else if (line_pos + cur_pos < buffer_len - scroll_width && cur_pos >= scroll_width - 1) { cur_pos -= scroll_width - 1; line_pos += scroll_width; }
				else line_pos++;
			}
		}
	}
	if (buffer_len == -1) return NULL;
	NormalizeLine(buffer);
	return strdup(buffer);
}

static int simple_add_history(const char *buffer, const char **history, const int history_count, const int max_history) {
	char *cp = buffer ? strdup(buffer) : NULL;
	if (!cp) return history_count;
	if (history_count && strcmp(history[history_count - 1], cp) == 0) {
		free(cp);
		return history_count;
	}
	if (history_count < max_history) {
		history[history_count] = cp;
		return history_count + 1;
	} else if (max_history) {
		int i;
		free((char *) history[0]);
		for (i = 0; i < history_count - 1; i++) history[i] = history[i + 1];
		history[history_count - 1] = cp;
		return history_count;
	}
	return 0;
}

static void ShowCurrent(void);

static int window_width = 0, window_height = 0;
static int current_y[MAXSCREEN], current_item_index[MAXSCREEN], list_item_count[MAXSCREEN];

static const int header_lines = 3;
static int body_lines = 0;

static int max_eat_col[MAXSCREEN];
static int eat_col = 0;
static int max_col = 0;

static const char *eat(const char *str) {
	while (*str && eat_col) {
		str++; eat_col--;
	}
	return str;
}
 
static void ShowList(void) {
	const int offset = current_item_index[current_screen];
	int i, tmp_col;
	if (current_screen == SCREEN_DOMAIN_LIST) list_item_count[SCREEN_DOMAIN_LIST] = domain_list_count[0];
	else list_item_count[current_screen] = generic_acl_list_count;
	clear();
	if (window_height < header_lines + 1) {
		mvprintw(0, 0, "Please resize window. This program needs at least %d lines.\n", header_lines + 1);
		refresh();
		return;
	}
	if (current_screen == SCREEN_DOMAIN_LIST) mvprintw(0, 0, "<<< Domain Transition Editor >>>      %d domain%c    '?' for help", list_item_count[SCREEN_DOMAIN_LIST] - unnumbered_domain_count, list_item_count[SCREEN_DOMAIN_LIST] - unnumbered_domain_count > 1 ? 's' : ' ');
	else mvprintw(0, 0, "<<< %s Editor >>>      %d entr%s    '?' for help", list_caption, list_item_count[current_screen], list_item_count[current_screen] > 1 ? "ies" : "y");
	eat_col = max_eat_col[current_screen];
	max_col = 0;
	if (current_screen == SCREEN_ACL_LIST) {
		get();
		memset(shared_buffer, 0, shared_buffer_len);
		snprintf(shared_buffer, shared_buffer_len - 1, "%s", eat(current_domain));
		mvprintw(2, 0, "%s", shared_buffer);
		put();
	}
	for (i = 0; i < body_lines; i++) {
		const int index = offset + i;
		eat_col = max_eat_col[current_screen];
		tmp_col = 0;
		if (index >= list_item_count[current_screen]) break;
		if (current_screen == SCREEN_DOMAIN_LIST) {
			const struct domain_initializer_entry *domain_initializer;
			const struct domain_keeper_entry *domain_keeper;
			const char *sp, *cp;
			const int number = domain_list[0][index].number;
			if (number >= 0) mvprintw(header_lines + i, 0, "%c%4d:%3u %c%c%c ", domain_list_selected[index] ? '&' : ' ', number, domain_list[0][index].profile, IsKeeperDomain(index) ? '#' : ' ', IsInitializerTarget(index) ? '*' : ' ', IsDomainUnreachable(index) ? '!' : ' ');
			else mvprintw(header_lines + i, 0, "              ");
			tmp_col += 14;
			sp = DomainName(index);
			while ((cp = strchr(sp, ' ')) != NULL) { printw("%s", eat("    ")); tmp_col += 4; sp = cp + 1; }
			if (IsDeletedDomain(index)) { printw("%s", eat("( ")); tmp_col += 2; }
			printw("%s", eat(sp)); tmp_col += strlen(sp);
			if (IsDeletedDomain(index)) { printw("%s", eat(" )")); tmp_col += 2; }
			if ((domain_initializer = domain_list[0][index].domain_initializer) != NULL) {
				get();
				memset(shared_buffer, 0, shared_buffer_len);
				if (domain_initializer->domainname) snprintf(shared_buffer, shared_buffer_len - 1, " ( %s%s from %s )", domain_initializer->is_oldstyle ? KEYWORD_INITIALIZER : KEYWORD_INITIALIZE_DOMAIN, domain_initializer->program->name, domain_initializer->domainname->name);
				else snprintf(shared_buffer, shared_buffer_len - 1, " ( %s%s )", domain_initializer->is_oldstyle ? KEYWORD_INITIALIZER : KEYWORD_INITIALIZE_DOMAIN, domain_initializer->program->name);
				printw("%s", eat(shared_buffer)); tmp_col += strlen(shared_buffer);
				put();
			} else if ((domain_keeper = domain_list[0][index].domain_keeper) != NULL) {
				get();
				memset(shared_buffer, 0, shared_buffer_len);
				if (domain_keeper->program) snprintf(shared_buffer, shared_buffer_len - 1, " ( " KEYWORD_KEEP_DOMAIN "%s from %s )", domain_keeper->program->name, domain_keeper->domainname->name);
				else snprintf(shared_buffer, shared_buffer_len - 1, " ( " KEYWORD_KEEP_DOMAIN "%s )", domain_keeper->domainname->name);
				printw("%s", eat(shared_buffer)); tmp_col += strlen(shared_buffer);
				put();
			} else if (IsInitializerSource(index)) {
				int redirect_index;
				get();
				memset(shared_buffer, 0, shared_buffer_len);
				snprintf(shared_buffer, shared_buffer_len - 1, ROOT_NAME "%s", strrchr(DomainName(index), ' '));
				redirect_index = FindDomain(shared_buffer, 0, 0, 0);
				if (redirect_index >= 0) snprintf(shared_buffer, shared_buffer_len - 1, " ( -> %d )", domain_list[0][redirect_index].number);
				else snprintf(shared_buffer, shared_buffer_len - 1, " ( -> Not Found )");
				printw("%s", eat(shared_buffer)); tmp_col += strlen(shared_buffer);
				put();
			}
		} else {
			const char *cp = generic_acl_list[index];
			mvprintw(header_lines + i, 0, "%c%4d: %s", generic_acl_list_selected[index] ? '&' : ' ', index, eat(cp)); tmp_col += strlen(cp) + 7;
		}
		clrtoeol();
		tmp_col -= window_width;
		if (tmp_col  > max_col) max_col = tmp_col;
	}
	ShowCurrent();
}

static void ResizeWindow(void) {
	getmaxyx(stdscr, window_height, window_width);
	body_lines = window_height - header_lines;
	if (body_lines <= current_y[current_screen]) current_y[current_screen] = body_lines - 1;
	if (current_y[current_screen] < 0) current_y[current_screen] = 0;
}

static void UpArrowKey(void) {
	if (current_y[current_screen] > 0) {
		current_y[current_screen]--;
		ShowCurrent();
	} else if (current_item_index[current_screen] > 0) {
		current_item_index[current_screen]--;
		ShowList();
	}
}

static void DownArrowKey(void) {
	if (current_y[current_screen] < body_lines - 1) {
		if (current_item_index[current_screen] + current_y[current_screen] < list_item_count[current_screen] - 1) {
			current_y[current_screen]++;
			ShowCurrent();
		}
	} else if (current_item_index[current_screen] + current_y[current_screen] < list_item_count[current_screen] - 1) {
		current_item_index[current_screen]++;
		ShowList();
	}
}

static void PageUpKey(void) {
	if (current_item_index[current_screen] + current_y[current_screen] > body_lines) {
		current_item_index[current_screen] -= body_lines;
		if (current_item_index[current_screen] < 0) current_item_index[current_screen] = 0;
		ShowList();
	} else if (current_item_index[current_screen] + current_y[current_screen] > 0) {
		current_item_index[current_screen] = 0;
		current_y[current_screen] = 0;
		ShowList();
	}
}

static void PageDownKey(void) {
	if (list_item_count[current_screen] - current_item_index[current_screen] > body_lines) {
		current_item_index[current_screen] += body_lines;
		if (current_item_index[current_screen] + current_y[current_screen] > list_item_count[current_screen] - 1) current_y[current_screen] = list_item_count[current_screen] - 1 - current_item_index[current_screen];
		ShowList();
	} else if (current_item_index[current_screen] + current_y[current_screen] < list_item_count[current_screen] - 1) {
		current_y[current_screen] = list_item_count[current_screen] - current_item_index[current_screen] - 1;
		ShowCurrent();
	}
}

static int GetCurrent(void) {
	if (list_item_count[current_screen] == 0) return EOF;
	if (current_item_index[current_screen] + current_y[current_screen] < 0 || current_item_index[current_screen] + current_y[current_screen] >= list_item_count[current_screen]) {
		fprintf(stderr, "ERROR: current_item_index=%d current_y=%d\n", current_item_index[current_screen], current_y[current_screen]);
		exit(127);
	}
	return current_item_index[current_screen] + current_y[current_screen];
}

static void ShowCurrent(void) {
	if (current_screen == SCREEN_DOMAIN_LIST) {
		get();
		memset(shared_buffer, 0, shared_buffer_len);
		eat_col = max_eat_col[current_screen];
		snprintf(shared_buffer, shared_buffer_len - 1, "%s", eat(DomainName(GetCurrent())));
		if (window_width < shared_buffer_len) shared_buffer[window_width] = '\0';
		move(2, 0);
		clrtoeol();
		printw("%s", shared_buffer);
		put();
	}
	move(header_lines + current_y[current_screen], 0);
	refresh();
}

static void AdjustCursorPos(const int item_count) {
	if (item_count == 0) {
		current_item_index[current_screen] = current_y[current_screen] = 0;
	} else {
		while (current_item_index[current_screen] + current_y[current_screen] >= item_count) {
			if (current_y[current_screen] > 0) current_y[current_screen]--;
			else if (current_item_index[current_screen] > 0) current_item_index[current_screen]--;
		}
	}
}

static void SetCursorPos(const int index) {
	while (index < current_y[current_screen] + current_item_index[current_screen]) {
		if (current_y[current_screen] > 0) current_y[current_screen]--;
		else current_item_index[current_screen]--;
	}
	while (index > current_y[current_screen] + current_item_index[current_screen]) {
		if (current_y[current_screen] < body_lines - 1) current_y[current_screen]++;
		else current_item_index[current_screen]++;
	}
}

static int count(const unsigned char *array, const int len) {
	int i, c = 0;
	for (i = 0; i < len; i++) if (array[i]) c++;
	return c;
}

static int GenericListLoop(void) {
	static char *last_error = NULL;
	static const int max_readline_history = 20;
	static const char **readline_history = NULL;
	static int readline_history_count = 0;
	static char *search_buffer[MAXSCREEN];
	static int saved_current_y[MAXSCREEN];
	static int saved_current_item_index[MAXSCREEN];
	static int first = 1;
	if (first) {
		readline_history = malloc(max_readline_history * sizeof(const char *));
		memset(saved_current_y, 0, sizeof(saved_current_y));
		memset(saved_current_item_index, 0, sizeof(saved_current_item_index));
		memset(search_buffer, 0, sizeof(search_buffer));
		first = 0;
	}
	if (current_screen == SCREEN_SYSTEM_LIST) {
		policy_file = SYSTEM_POLICY_FILE;
		list_caption = "System Policy";
	} else if (current_screen == SCREEN_EXCEPTION_LIST) {
		policy_file = EXCEPTION_POLICY_FILE;
		list_caption = "Exception Policy";
	} else if (current_screen == SCREEN_ACL_LIST) {
		policy_file = DOMAIN_POLICY_FILE;
		list_caption = "Domain Policy";
	} else {
		policy_file = DOMAIN_POLICY_FILE;
		//list_caption = "Domain Transition";
	}
	current_item_index[current_screen] = saved_current_item_index[current_screen];
	current_y[current_screen] = saved_current_y[current_screen];
 start:
	if (current_screen == SCREEN_DOMAIN_LIST) {
		ReadDomainAndExceptionPolicy();
		AdjustCursorPos(domain_list_count[0]);
	} else {
		ReadGenericPolicy();
		AdjustCursorPos(generic_acl_list_count);
	}
 start2:
	ShowList();
	if (last_error && current_screen == SCREEN_DOMAIN_LIST) {
		mvprintw(1, 0, "ERROR: %s", last_error); clrtoeol(); refresh();
		free(last_error); last_error = NULL;
	}	
	while (1) {
		const int current = GetCurrent();
		const int c = getch2();
		saved_current_item_index[current_screen] = current_item_index[current_screen];
		saved_current_y[current_screen] = current_y[current_screen];
		if (c == 'q' || c == 'Q') return MAXSCREEN;
		if (c == '\r' && current_screen == SCREEN_ACL_LIST) return SCREEN_DOMAIN_LIST;
		if (c == '\t') {
			if (current_screen == SCREEN_DOMAIN_LIST) return SCREEN_SYSTEM_LIST;
			else if (current_screen == SCREEN_SYSTEM_LIST) return SCREEN_EXCEPTION_LIST;
			else return SCREEN_DOMAIN_LIST;
		}
		if (c == ERR) continue; // Ignore invalid key.
		switch(c) {
		case KEY_RESIZE:
			ResizeWindow();
			ShowList();
			break;
		case KEY_UP:
			UpArrowKey();
			break;
		case KEY_DOWN:
			DownArrowKey();
			break;
		case KEY_PPAGE:
			PageUpKey();
			break;
		case KEY_NPAGE:
			PageDownKey();
			break;
		case ' ':
			if (current >= 0) {
				if (current_screen == SCREEN_DOMAIN_LIST) {
					if (IsDeletedDomain(current) || IsInitializerSource(current)) break;
					domain_list_selected[current] ^= 1;
				} else {
					generic_acl_list_selected[current] ^= 1;
				}
				ShowList();
			}
			break;
		case 'c':
		case 'C':
			if (current >= 0) {
				int index;
				if (current_screen == SCREEN_DOMAIN_LIST) {
					if (IsDeletedDomain(current) || IsInitializerSource(current)) break;
					for (index = current; index < domain_list_count[0]; index++) {
						if (IsDeletedDomain(index) || IsInitializerSource(index)) continue;
						domain_list_selected[index] = domain_list_selected[current];
					}
				} else {
					for (index = current; index < generic_acl_list_count; index++) {
						generic_acl_list_selected[index] = generic_acl_list_selected[current];
					}
				}
				ShowList();
			}
			break;
		case 'f':
		case 'F':
			if (current >= 0) {
				int index;
				char *line;
			input_path:
				line = simple_readline(window_height - 1, 0, "Search> ", readline_history, readline_history_count, 4000, 8);
				if (line && *line) {
					readline_history_count = simple_add_history(line, readline_history, readline_history_count, max_readline_history);
					free(search_buffer[current_screen]); search_buffer[current_screen] = line; line = NULL;
					for (index = 0; index < list_item_count[current_screen]; index++) {
						const char *cp = (current_screen == SCREEN_DOMAIN_LIST) ? GetLastName(index) : generic_acl_list[index];
						if (!strstr(cp, search_buffer[current_screen])) continue;
						SetCursorPos(index);
						break;
					}
				}
				free(line);
				ShowList();
			}
			break;
		case 'p':
		case 'P':
			if (current >= 0) {
				int index;
				if (!search_buffer[current_screen]) goto input_path;
				for (index = current - 1; index >= 0; index--) {
					const char *cp = (current_screen == SCREEN_DOMAIN_LIST) ? GetLastName(index) : generic_acl_list[index];
					if (!strstr(cp, search_buffer[current_screen])) continue;
					SetCursorPos(index);
					ShowList();
					break;
				}
			}
			break;
		case 'n':
		case 'N':
			if (current >= 0) {
				int index;
				if (!search_buffer[current_screen]) goto input_path;
				for (index = current + 1; index < list_item_count[current_screen]; index++) {
					const char *cp = (current_screen == SCREEN_DOMAIN_LIST) ? GetLastName(index) : generic_acl_list[index];
					if (!strstr(cp, search_buffer[current_screen])) continue;
					SetCursorPos(index);
					ShowList();
					break;
				}
			}
			break;
		case 'd':
		case 'D':
			{
				int c;
				move(1, 0);
				if (current_screen == SCREEN_DOMAIN_LIST) {
					if ((c = count(domain_list_selected, domain_list_count[0])) == 0) printw("Select domain using Space key first.");
					else printw("Delete selected domain%s? ('Y'es/'N'o)", c > 1 ? "s" : "");
				} else {
					if ((c = count(generic_acl_list_selected, generic_acl_list_count)) == 0) printw("Select entry using Space key first.");
					else printw("Delete selected entr%s? ('Y'es/'N'o)", c > 1 ? "ies" : "y");
				}
				clrtoeol();
				refresh();
				if (!c) break;
				do {
					c = getch2();
				} while (!(c == 'Y' || c == 'y' || c == 'N' || c == 'n' || c == EOF));
				ResizeWindow();
				if (c == 'Y' || c == 'y') {
					int index;
					if (current_screen == SCREEN_DOMAIN_LIST) {
						FILE *fp = open_write(DOMAIN_POLICY_FILE);
						if (fp) {
							for (index = 1; index < domain_list_count[0]; index++) {
								if (domain_list_selected[index]) fprintf(fp, "delete %s\n", DomainName(index));
							}
							fclose(fp);
						}
					} else {
						FILE *fp = open_write(policy_file);
						if (fp) {
							if (current_screen == SCREEN_ACL_LIST) fprintf(fp, "select %s\n", current_domain);
							for (index = 0; index < generic_acl_list_count; index++) {
								if (generic_acl_list_selected[index]) fprintf(fp, "delete %s\n", generic_acl_list[index]);
							}
							fclose(fp);
						}
					}
					goto start;
				}
				ShowList();
			}
			break;
		case 'a':
		case 'A':
			{
				char *line = simple_readline(window_height - 1, 0, "Enter new entry> ", readline_history, readline_history_count, 8192, 8);
				if (line && *line) {
					readline_history_count = simple_add_history(line, readline_history, readline_history_count, max_readline_history);
					if (current_screen == SCREEN_DOMAIN_LIST && !IsCorrectDomain(line)) {
						const int len = strlen(line) + 128;
						if ((last_error = (char *) realloc(last_error, len)) == NULL) OutOfMemory();
						memset(last_error, 0, len);
						snprintf(last_error, len - 1, "%s is a bad domainname.", line);
					} else {
						FILE *fp = open_write(policy_file);
						if (fp) {
							if (current_screen == SCREEN_ACL_LIST) fprintf(fp, "select %s\n", current_domain);
							fprintf(fp, "%s\n", line);
							fclose(fp);
						}
					}
				}
				free(line);
				goto start;
			}
			break;
		case '\r':
			if (current_screen == SCREEN_DOMAIN_LIST) {
				if (IsInitializerSource(current)) {
					int redirect_index;
					get();
					memset(shared_buffer, 0, shared_buffer_len);
					snprintf(shared_buffer, shared_buffer_len - 1, ROOT_NAME "%s", strrchr(DomainName(current), ' '));
					redirect_index = FindDomain(shared_buffer, 0, 0, 0);
					put();
					if (redirect_index != EOF) {
						current_item_index[current_screen] = redirect_index - current_y[current_screen];
						while (current_item_index[current_screen] < 0) {
							current_item_index[current_screen]++; current_y[current_screen]--;
						}
						ShowList();
					}
				} else if (!IsDeletedDomain(current)) {
					free(current_domain);
					if ((current_domain = strdup(DomainName(current))) == NULL) OutOfMemory();
					return SCREEN_ACL_LIST;
				}
			}
			break;
		case 's':
		case 'S':
			if (current_screen == SCREEN_DOMAIN_LIST) {
				if (!count(domain_list_selected, domain_list_count[0])) {
					mvprintw(1, 0, "Select domain using Space key first."); clrtoeol(); refresh();
				} else {
					char *line = simple_readline(window_height - 1, 0, "Enter profile number> ", NULL, 0, 8, 1);
					if (line && *line) {
						FILE *fp = open_write(DOMAIN_POLICY_FILE);
						if (fp) {
							int index;
							for (index = 0; index < domain_list_count[0]; index++) {
								if (domain_list_selected[index]) fprintf(fp, "select %s\nuse_profile %s\n", DomainName(index), line);
							}
							fclose(fp);
						}
					}
					free(line);
					goto start;
				}
			}
			break;
		case 'r':
		case 'R':
			goto start;
		case KEY_LEFT:
			if (!max_eat_col[current_screen]) break;
			max_eat_col[current_screen]--; 
			goto start2;
		case KEY_RIGHT:
			max_eat_col[current_screen]++;
			goto start2;
		case KEY_HOME:
			max_eat_col[current_screen] = 0;
			goto start2;
		case KEY_END:
			max_eat_col[current_screen] = max_col;
			goto start2;
		case KEY_IC:
			if (current >= 0) readline_history_count = simple_add_history(current_screen == SCREEN_DOMAIN_LIST ? DomainName(current) : generic_acl_list[current], readline_history, readline_history_count, max_readline_history);
			break;
		case '?':
			{
				int c;
				clear();
				printw("Commands available for this screen are:\n\n"
					   "Q/q        Quit this editor.\n"
					   "R/r        Refresh to the latest information.\n"
					   "F/f        Find first.\n"
					   "N/n        Find next.\n"
					   "P/p        Find previous.\n"
					   "Tab        Switch to next screen.\n"
					   "Insert     Copy an entry at the cursor position to history buffer.\n"
					   "Space      Invert selection state of an entry at the cursor position.\n"
					   "C/c        Copy selection state of an entry at the cursor position to all entries below the cursor position.\n");
				if (current_screen == SCREEN_DOMAIN_LIST) {
					printw("A/a        Add a new domain.\n"
						   "Enter      Edit ACLs of a domain at the cursor position.\n"
						   "D/d        Delete selected domains.\n"
						   "S/s        Set profile number of selected domains.\n");
				} else {
					printw("A/a        Add a new entry.\n"
						   "D/d        Delete selected entries.\n");
				}
				printw("Arrow-keys and PageUp/PageDown/Home/End keys for scroll.\n\n"
					   "Press '?' to escape from this help.\n"); refresh();
				while ((c = getch2()) != '?' && c != EOF);
				goto start;
			}
			break;
		}
	}
}

static void policy_daemon(void) {
	get();
	FindOrAssignNewDomain(ROOT_NAME, 0, 0, 0);
	while (1) {
		static const struct path_info **exception_list = NULL, **system_list = NULL;
		static int exception_list_count = 0, system_list_count = 0;
		FILE *fp;
		{
			struct msghdr msg;
			struct iovec iov = { shared_buffer, shared_buffer_len - 1 };
			char cmsg_buf[CMSG_SPACE(sizeof(int))];
			struct cmsghdr *cmsg = (struct cmsghdr *) cmsg_buf;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = &iov;
			msg.msg_iovlen = 1;
			msg.msg_control = cmsg_buf;
			msg.msg_controllen = sizeof(cmsg_buf);
			memset(shared_buffer, 0, shared_buffer_len);
			errno = 0;
			if (recvmsg(persistent_fd, &msg, 0) > 0 &&
				(cmsg = CMSG_FIRSTHDR(&msg)) != NULL &&
				cmsg->cmsg_level == SOL_SOCKET &&
				cmsg->cmsg_type == SCM_RIGHTS &&
				cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
				const int fd = * (int *) CMSG_DATA(cmsg);
				if ((fp = fdopen(fd, "w+")) == NULL) {
					close(fd);
					continue;
				}
			} else {
				break;
			}
		}
		if (strncmp(shared_buffer, "POST ", 5) == 0) {
			if (strcmp(shared_buffer + 5, "domain_policy") == 0) {
				int index = EOF;
				while (freadline(fp)) {
					int is_delete = 0, is_select = 0;
					if (strncmp(shared_buffer, "delete ", 7) == 0) {
						is_delete = 1;
						RemoveHeader(shared_buffer, 7);
					} else if (strncmp(shared_buffer, "select ", 7) == 0) {
						is_select = 1;
						RemoveHeader(shared_buffer, 7);
					}
					if (IsDomainDef(shared_buffer)) {
						if (is_delete) {
							index = FindDomain(shared_buffer, 0, 0, 0);
							if (index > 0) DeleteDomain(index, 0);
							index = EOF;
						} else if (is_select) {
							index = FindDomain(shared_buffer, 0, 0, 0);
						} else {
							index = FindOrAssignNewDomain(shared_buffer, 0, 0, 0);
						}
					} else if (index >= 0 && shared_buffer[0]) {
						unsigned int profile;
						if (sscanf(shared_buffer, "use_profile %u", &profile) == 1) {
							domain_list[0][index].profile = (unsigned char) profile;
						} else if (is_delete) {
							DelStringEntry(shared_buffer, index, 0);
						} else {
							AddStringEntry(shared_buffer, index, 0);
						}
					}
				}
			} else if (strcmp(shared_buffer + 5, "exception_policy") == 0) {
				while (freadline(fp)) {
					if (!shared_buffer[0]) continue;
					if (strncmp(shared_buffer, "delete ", 7) == 0) {
						int i;
						struct path_info path;
						RemoveHeader(shared_buffer, 7);
						path.name = shared_buffer;
						fill_path_info(&path);
						for (i = 0; i < exception_list_count; i++) {
							if (pathcmp(exception_list[i], &path)) continue;
							for (exception_list_count--; i < exception_list_count; i++) exception_list[i] = exception_list[i + 1];
							break;
						}
					} else {
						if ((exception_list = (const struct path_info **) realloc(exception_list, (exception_list_count + 1) * sizeof(const struct path_info *))) == NULL
							|| (exception_list[exception_list_count++] = SaveName(shared_buffer)) == NULL) OutOfMemory();
					}
				}
			} else if (strcmp(shared_buffer + 5, "system_policy") == 0) {
				while (freadline(fp)) {
					if (!shared_buffer[0]) continue;
					if (strncmp(shared_buffer, "delete ", 7) == 0) {
						int i;
						struct path_info path;
						RemoveHeader(shared_buffer, 7);
						path.name = shared_buffer;
						fill_path_info(&path);
						for (i = 0; i < system_list_count; i++) {
							if (pathcmp(system_list[i], &path)) continue;
							for (system_list_count--; i < system_list_count; i++) system_list[i] = system_list[i + 1];
							break;
						}
					} else {
						if ((system_list = (const struct path_info **) realloc(system_list, (system_list_count + 1) * sizeof(struct path_info *))) == NULL
							|| (system_list[system_list_count++] = SaveName(shared_buffer)) == NULL) OutOfMemory();
					}
				}
			}
		} else if (strncmp(shared_buffer, "GET ", 4) == 0) {
			if (strcmp(shared_buffer + 4, "domain_policy") == 0) {
				int i, j;
				for (i = 0; i < domain_list_count[0]; i++) {
					const struct path_info **string_ptr = domain_list[0][i].string_ptr;
					const int string_count = domain_list[0][i].string_count;
					fprintf(fp, "%s\nuse_profile %u\n\n", DomainName(i), domain_list[0][i].profile);
					for (j = 0; j < string_count; j++) {
						fprintf(fp, "%s\n", string_ptr[j]->name);
					}
					fprintf(fp, "\n");
				}
			} else if (strcmp(shared_buffer + 4, "exception_policy") == 0) {
				int i;
				for (i = 0; i < exception_list_count; i++) fprintf(fp, "%s\n", exception_list[i]->name);
			} else if (strcmp(shared_buffer + 4, "system_policy") == 0) {
				int i;
				for (i = 0; i < system_list_count; i++) fprintf(fp, "%s\n", system_list[i]->name);
			}
		}
		fclose(fp);
	}
	put();
	_exit(0);
}

static int editpolicy_main(int argc, char *argv[]) {
	memset(current_y, 0, sizeof(current_y));
	memset(current_item_index, 0, sizeof(current_item_index));
	memset(list_item_count, 0, sizeof(list_item_count));
	memset(max_eat_col, 0, sizeof(max_eat_col));
	if (argc > 1) {
		if (strcmp(argv[1], "s") == 0) current_screen = SCREEN_SYSTEM_LIST;
		else if (strcmp(argv[1], "e") == 0) current_screen = SCREEN_EXCEPTION_LIST;
		else if (strcmp(argv[1], "d") == 0) current_screen = SCREEN_DOMAIN_LIST;
		else {
			printf("Usage: %s [s|e|d]\n", argv[0]);
			return 1;
		}
	}
	{
		char *cp = strrchr(argv[0], '/');
		if (!cp) cp = argv[0];
		else cp++;
		if (strcmp(cp, "editpolicy_offline") == 0) offline_mode = 1;
	}
	if (offline_mode) {
		int fd[2];
		if (chdir("/etc/ccs/")) {
			fprintf(stderr, "/etc/ccs/ doesn't exist.\n");
			return 1;
		}
		if (socketpair(PF_UNIX, SOCK_STREAM, 0, fd)) {
			fprintf(stderr, "socketpair()\n");
			exit(1);
		}
		switch (fork()) {
		case 0:
			close(fd[0]);
			persistent_fd = fd[1];
			policy_daemon();
			_exit(0);
		case -1:
			fprintf(stderr, "fork()\n");
			exit(1);
		}
		close(fd[1]);
		persistent_fd = fd[0];
		{
			int fd, len;
			FILE *fp;
			get();
			if ((fd = open("system_policy.txt", O_RDONLY)) != EOF) {
				fp = open_write(SYSTEM_POLICY_FILE);
				while ((len = read(fd, shared_buffer, shared_buffer_len)) > 0) fwrite(shared_buffer, len, 1, fp);
				fclose(fp); close(fd);
			}
			if ((fd = open("exception_policy.txt", O_RDONLY)) != EOF) {
				fp = open_write(EXCEPTION_POLICY_FILE);
				while ((len = read(fd, shared_buffer, shared_buffer_len)) > 0) fwrite(shared_buffer, len, 1, fp);
				fclose(fp); close(fd);
			}
			if ((fd = open("domain_policy.txt", O_RDONLY)) != EOF) {
				fp = open_write(DOMAIN_POLICY_FILE);
				while ((len = read(fd, shared_buffer, shared_buffer_len)) > 0) fwrite(shared_buffer, len, 1, fp);
				fclose(fp); close(fd);
			}
			put();
		}
	} else {
		if (chdir("/proc/ccs/policy/")) {
			fprintf(stderr, "You can't use this editor for this kernel.\n");
			return 1;
		}
		{
			const int fd1 = open(SYSTEM_POLICY_FILE, O_RDWR), fd2 = open(EXCEPTION_POLICY_FILE, O_RDWR), fd3 = open(DOMAIN_POLICY_FILE, O_RDWR);
			if ((fd1 != EOF && write(fd1, "", 0) != 0) || (fd2 != EOF && write(fd2, "", 0) != 0) || (fd3 != EOF && write(fd3, "", 0) != 0)) {
				fprintf(stderr, "You need to register this program to /proc/ccs/policy/manager to run this program.\n");
				return 1;
			}
			close(fd1); close(fd2); close(fd3);
		}
	}
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, window_height, window_width);
	while (current_screen < MAXSCREEN) {
		if (!offline_mode) {
			if (current_screen == SCREEN_DOMAIN_LIST && access(DOMAIN_POLICY_FILE, F_OK)) current_screen = SCREEN_SYSTEM_LIST;
			else if (current_screen == SCREEN_SYSTEM_LIST && access(SYSTEM_POLICY_FILE, F_OK)) current_screen = SCREEN_EXCEPTION_LIST;
			else if (current_screen == SCREEN_EXCEPTION_LIST && access(EXCEPTION_POLICY_FILE, F_OK)) current_screen = SCREEN_DOMAIN_LIST;
		}
		ResizeWindow();
		current_screen = GenericListLoop();
	}
	clear();
	move(0, 0);
	refresh();
	endwin();
	if (offline_mode) {
		int fd, len;
		FILE *fp;
		time_t now = time(NULL);
		struct tm *tm = localtime(&now);
		char filename[1024], buffer[1024];
		memset(filename, 0, sizeof(filename));
		snprintf(filename, sizeof(filename) - 1, "system_policy.%02d-%02d-%02d.%02d:%02d:%02d.txt", tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		if ((fd = open(filename, O_WRONLY | O_CREAT, 0600)) != EOF) {
			if ((fp = open_read(SYSTEM_POLICY_FILE)) != NULL) {
				while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) write(fd, buffer, len);
				close(fd); fclose(fp);
				unlink("system_policy.txt");
				symlink(filename, "system_policy.txt");
			}
		}
		snprintf(filename, sizeof(filename) - 1, "exception_policy.%02d-%02d-%02d.%02d:%02d:%02d.txt", tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		if ((fd = open(filename, O_WRONLY | O_CREAT, 0600)) != EOF) {
			if ((fp = open_read(EXCEPTION_POLICY_FILE)) != NULL) {
				while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) write(fd, buffer, len);
				close(fd); fclose(fp);
				unlink("exception_policy.txt");
				symlink(filename, "exception_policy.txt");
			}
		}
		snprintf(filename, sizeof(filename) - 1, "domain_policy.%02d-%02d-%02d.%02d:%02d:%02d.txt", tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		if ((fd = open(filename, O_WRONLY | O_CREAT, 0600)) != EOF) {
			if ((fp = open_read(DOMAIN_POLICY_FILE)) != NULL) {
				while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) write(fd, buffer, len);
				close(fd); fclose(fp);
				unlink("domain_policy.txt");
				symlink(filename, "domain_policy.txt");
			}
		}
	}
	return 0;
}

/***** editpolicy end *****/

/***** checkpolicy start *****/

static unsigned int line = 0, errors = 0, warnings = 0;

static int CheckCondition(const char *condition) {
	static const struct {
		const char * const keyword;
		const int keyword_len; /* strlen(keyword) */
	} condition_control_keyword[] = {
		{ "task.uid",           8 },
		{ "task.euid",          9 },
		{ "task.suid",          9 },
		{ "task.fsuid",        10 },
		{ "task.gid",           8 },
		{ "task.egid",          9 },
		{ "task.sgid",          9 },
		{ "task.fsgid",        10 },
		{ "task.pid",           8 },
		{ "task.ppid",          9 },
		{ "path1.uid",          9 },
		{ "path1.gid",          9 },
		{ "path1.ino",          9 },
		{ "path1.parent.uid",  16 },
		{ "path1.parent.gid",  16 },
		{ "path1.parent.ino",  16 },
		{ "path2.parent.uid",  16 },
		{ "path2.parent.gid",  16 },
		{ "path2.parent.ino",  16 },
		{ NULL, 0 }
	};
	const char *start = condition;
	int left, right;
	unsigned long left_min = 0, left_max = 0, right_min = 0, right_max = 0;
	if (strncmp(condition, "if ", 3)) goto out;
	condition += 3;
	while (*condition) {
		if (*condition == ' ') condition++;
		for (left = 0; condition_control_keyword[left].keyword; left++) {
			if (strncmp(condition, condition_control_keyword[left].keyword, condition_control_keyword[left].keyword_len) == 0) {
				condition += condition_control_keyword[left].keyword_len;
				break;
			}
		}
		if (!condition_control_keyword[left].keyword) {
			if (!parse_ulong(&left_min, &condition)) goto out;
			if (*condition == '-') {
				condition++;
				if (!parse_ulong(&left_max, &condition) || left_min > left_max) goto out;
			}
		}
		if (strncmp(condition, "!=", 2) == 0) condition += 2;
		else if (*condition == '=') condition++;
		else goto out;
		for (right = 0; condition_control_keyword[right].keyword; right++) {
			if (strncmp(condition, condition_control_keyword[right].keyword, condition_control_keyword[right].keyword_len) == 0) {
				condition += condition_control_keyword[right].keyword_len;
				break;
			}
		}
		if (!condition_control_keyword[right].keyword) {
			if (!parse_ulong(&right_min, &condition)) goto out;
			if (*condition == '-') {
				condition++;
				if (!parse_ulong(&right_max, &condition) || right_min > right_max) goto out;
			}
		}
	}
	return 1;
 out:
	printf("%u: ERROR: '%s' is a illegal condition.\n", line, start); errors++;
	return 0;
}

static void CheckCapabilityPolicy(char *data) {
	static const char *capability_keywords[] = {
		"inet_tcp_create", "inet_tcp_listen", "inet_tcp_connect", "use_inet_udp", "use_inet_ip", "use_route", "use_packet",
		"SYS_MOUNT", "SYS_UMOUNT", "SYS_REBOOT", "SYS_CHROOT", "SYS_KILL", "SYS_VHANGUP", "SYS_TIME", "SYS_NICE", "SYS_SETHOSTNAME",
		"use_kernel_module", "create_fifo", "create_block_dev", "create_char_dev", "create_unix_socket",
		"SYS_LINK", "SYS_SYMLINK", "SYS_RENAME", "SYS_UNLINK", "SYS_CHMOD", "SYS_CHOWN", "SYS_IOCTL", "SYS_KEXEC_LOAD", NULL
	};
	int i;
	char *cp;
	if ((cp = FindConditionPart(data)) != NULL && !CheckCondition(cp)) return;
	for (i = 0; capability_keywords[i]; i++) {
		if (strcmp(data, capability_keywords[i]) == 0) return;
	}
	printf("%u: ERROR: '%s' is a bad capability name.\n", line, data); errors++;
}

static void CheckSignalPolicy(char *data) {
	int sig;
    char *cp;
	if ((cp = FindConditionPart(data)) != NULL && !CheckCondition(cp)) return;
	cp = strchr(data, ' ');
	if (!cp) {
		printf("%u: ERROR: Too few parameters.\n", line); errors++;
		return;
	}
	*cp++ = '\0';
	if (sscanf(data, "%d", &sig) != 1) {
		printf("%u: ERROR: '%s' is a bad signal number.\n", line, data); errors++;
	}
	if (!IsCorrectDomain(cp)) {
		printf("%u: ERROR: '%s' is a bad domainname.\n", line, cp); errors++;
	}
}

static void CheckArgv0Policy(char *data) {
	char *argv0 = strchr(data, ' ');
	char *cp;
	if (!argv0) {
		printf("%u: ERROR: Too few parameters.\n", line); errors++;
		return;
	}
	*argv0++ = '\0';
	if ((cp = FindConditionPart(argv0)) != NULL && !CheckCondition(cp)) return;
	if (!IsCorrectPath(data, 1, 0, -1)) {
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, data); errors++;
	}
	if (!IsCorrectPath(argv0, -1, 0, -1) || strchr(argv0, '/')) {
		printf("%u: ERROR: '%s' is a bad argv[0] name.\n", line, data); errors++;
	}
}

static void CheckNetworkPolicy(char *data) {
	int sock_type, operation, is_ipv6;
	u16 min_address[8], max_address[8];
	unsigned int min_port, max_port;
	int count;
	char *cp1 = NULL, *cp2 = NULL;
	if ((cp1 = FindConditionPart(data)) != NULL && !CheckCondition(cp1)) return;
	if ((cp1 = strchr(data, ' ')) == NULL) goto out; cp1++;
	if (strncmp(data, "TCP ", 4) == 0) sock_type = SOCK_STREAM;
	else if (strncmp(data, "UDP ", 4) == 0) sock_type = SOCK_DGRAM;
	else if (strncmp(data, "RAW ", 4) == 0) sock_type = SOCK_RAW;
	else goto out;
	if ((cp2 = strchr(cp1, ' ')) == NULL) goto out; cp2++;
	if (strncmp(cp1, "bind ", 5) == 0) {
		operation = (sock_type == SOCK_STREAM) ? NETWORK_ACL_TCP_BIND : (sock_type == SOCK_DGRAM) ? NETWORK_ACL_UDP_BIND : NETWORK_ACL_RAW_BIND;
	} else if (strncmp(cp1, "connect ", 8) == 0) {
		operation = (sock_type == SOCK_STREAM) ? NETWORK_ACL_TCP_CONNECT : (sock_type == SOCK_DGRAM) ? NETWORK_ACL_UDP_CONNECT : NETWORK_ACL_RAW_CONNECT;
	} else if (sock_type == SOCK_STREAM && strncmp(cp1, "listen ", 7) == 0) {
		operation = NETWORK_ACL_TCP_LISTEN;
	} else if (sock_type == SOCK_STREAM && strncmp(cp1, "accept ", 7) == 0) {
		operation = NETWORK_ACL_TCP_ACCEPT;
	} else {
		goto out;
	}
	if ((cp1 = strchr(cp2, ' ')) == NULL) goto out; cp1++;
	if ((count = sscanf(cp2, "%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx-%hx:%hx:%hx:%hx:%hx:%hx:%hx:%hx",
						&min_address[0], &min_address[1], &min_address[2], &min_address[3],
						&min_address[4], &min_address[5], &min_address[6], &min_address[7],
						&max_address[0], &max_address[1], &max_address[2], &max_address[3],
						&max_address[4], &max_address[5], &max_address[6], &max_address[7])) == 8 || count == 16) {
		int i;
		for (i = 0; i < 8; i++) {
			min_address[i] = htons(min_address[i]);
			max_address[i] = htons(max_address[i]);
		}
		if (count == 8) memmove(max_address, min_address, sizeof(min_address));
		is_ipv6 = 1;
	} else if ((count = sscanf(cp2, "%hu.%hu.%hu.%hu-%hu.%hu.%hu.%hu",
							   &min_address[0], &min_address[1], &min_address[2], &min_address[3],
 							   &max_address[0], &max_address[1], &max_address[2], &max_address[3])) == 4 || count == 8) {
		u32 ip = htonl((((u8) min_address[0]) << 24) + (((u8) min_address[1]) << 16) + (((u8) min_address[2]) << 8) + (u8) min_address[3]);
		* (u32 *) min_address = ip;
		if (count == 8) ip = htonl((((u8) max_address[0]) << 24) + (((u8) max_address[1]) << 16) + (((u8) max_address[2]) << 8) + (u8) max_address[3]);
		* (u32 *) max_address = ip;
		is_ipv6 = 0;
	} else {
		goto out;
	}
	if (strchr(cp1, ' ')) goto out;
	if ((count = sscanf(cp1, "%u-%u", &min_port, &max_port)) == 1 || count == 2) {
		if (count == 1) max_port = min_port;
		if (min_port <= max_port && max_port < 65536) return;
	}
 out: ;
	printf("%u: ERROR: Bad network address.\n", line); errors++;
}

static void CheckFilePolicy(char *data) {
	static const struct {
		const char * const keyword;
		const int paths;
	} acl_type_array[] = {
		{ "create",   1 },
		{ "unlink",   1 },
		{ "mkdir",    1 },
		{ "rmdir",    1 },
		{ "mkfifo",   1 },
		{ "mksock",   1 },
		{ "mkblock",  1 },
		{ "mkchar",   1 },
		{ "truncate", 1 },
		{ "symlink",  1 },
		{ "link",     2 },
		{ "rename",   2 },
		{ "rewrite",  1 },
		{ NULL, 0 }
	};
	char *filename = strchr(data, ' ');
	char *cp;
	unsigned int perm;
	if (!filename) {
		printf("%u: ERROR: Unknown command '%s'\n", line, data); errors++;
		return;
	}
	*filename++ = '\0';
	if ((cp = FindConditionPart(filename)) != NULL && !CheckCondition(cp)) return;
	if (sscanf(data, "%u", &perm) == 1 && perm > 0 && perm <= 7) {
		if (strendswith(filename, "/")) {
			if ((perm & 2) == 0) {
				printf("%u: WARNING: Directory '%s' without write permission will be ignored.\n", line, filename); warnings++;
			}
		}
		if (!IsCorrectPath(filename, 0, 0, 0)) goto out;
		return;
	}
	if (strncmp(data, "allow_", 6) == 0) {
		int type;
		for (type = 0; acl_type_array[type].keyword; type++) {
			if (strcmp(data + 6, acl_type_array[type].keyword)) continue;
			if (acl_type_array[type].paths == 2) {
				cp = strchr(filename, ' ');
				if (!cp || !IsCorrectPath(cp + 1, 0, 0, 0)) break;
				*cp = '\0';
			}
			if (!IsCorrectPath(filename, 0, 0, 0)) break;
			return;
		}
		if (!acl_type_array[type].keyword) goto out2;
	out:
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, filename); errors++;
		return;
	}
 out2:
	printf("%u: ERROR: Invalid permission '%s %s'\n", line, data, filename); errors++;
}

static void CheckMountPolicy(char *data) {
	char *cp, *cp2;
	const char *fs, *dev, *dir;
	unsigned int enable = 0, disable = 0;
	cp2 = data; if ((cp = strchr(cp2, ' ')) == NULL) goto out; *cp = '\0'; dev = cp2;
	cp2 = cp + 1; if ((cp = strchr(cp2, ' ')) == NULL) goto out; *cp = '\0'; dir = cp2;
	cp2 = cp + 1;
	if ((cp = strchr(cp2, ' ')) != NULL) {
		char *sp = cp + 1;
		*cp = '\0';
		while ((cp = strsep(&sp, " ,")) != NULL) {
			if (strcmp(cp, "rw") == 0)          disable |= 1;
			else if (strcmp(cp, "ro") == 0)     enable  |= 1;
			else if (strcmp(cp, "suid") == 0)   disable |= 2;
			else if (strcmp(cp, "nosuid") == 0) enable  |= 2;
			else if (strcmp(cp, "dev") == 0)    disable |= 4;
			else if (strcmp(cp, "nodev") == 0)  enable  |= 4;
			else if (strcmp(cp, "exec") == 0)   disable |= 8;
			else if (strcmp(cp, "noexec") == 0) enable  |= 8;
			else if (strcmp(cp, "atime") == 0)      disable |= 1024;
			else if (strcmp(cp, "noatime") == 0)    enable  |= 1024;
			else if (strcmp(cp, "diratime") == 0)   disable |= 2048;
			else if (strcmp(cp, "nodiratime") == 0) enable  |= 2048;
			else if (strcmp(cp, "norecurse") == 0)  disable |= 16384;
			else if (strcmp(cp, "recurse") == 0)    enable  |= 16384;
		}
	}
	fs = cp2;
	if (enable & disable) {
		printf("%u: ERROR: Conflicting mount options.\n", line); errors++;
	}
	if (!IsCorrectPath(dev, 0, 0, 0)) {
		printf("%u: ERROR: '%s' is a bad device name.\n", line, dir); errors++;
	}
	if (!IsCorrectPath(dir, 1, 0, 1)) {
		printf("%u: ERROR: '%s' is a bad mount point.\n", line, dir); errors++;
	}
	return;
 out:
	printf("%u: ERROR: Too few parameters.\n", line); errors++;
}

static void CheckPivotRootPolicy(char *data) {
	char *cp;
	if ((cp = strchr(data, ' ')) == NULL) goto out;
	*cp++ = '\0';
	if (!IsCorrectPath(data, 1, 0, 1)) {
		printf("%u: ERROR: '%s' is a bad directory.\n", line, data); errors++;
	}
	if (!IsCorrectPath(cp, 1, 0, 1)) {
		printf("%u: ERROR: '%s' is a bad directory.\n", line, cp); errors++;
	}
	return;
 out:
	printf("%u: ERROR: Too few parameters.\n", line); errors++;
}

static void CheckReservedPortPolicy(char *data) {
	unsigned int from, to;
	if (strchr(data, ' ')) goto out;
	if (sscanf(data, "%u-%u", &from, &to) == 2) {
		if (from <= to && to < 65536) return;
	} else if (sscanf(data, "%u", &from) == 1) {
		if (from < 65536) return;
	} else {
		printf("%u: ERROR: Too few parameters.\n", line); errors++;
		return;
	}
 out:
	printf("%u: ERROR: '%s' is a bad port number.\n", line, data); errors++;
}

static void CheckDomainInitializerEntry(const char *domainname, const char *program) {
	if (!IsCorrectPath(program, 1, 0, -1)) {
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, program); errors++;
	}
	if (domainname && !IsCorrectPath(domainname, 1, -1, -1) && !IsCorrectDomain(domainname)) {
		printf("%u: ERROR: '%s' is a bad domainname.\n", line, domainname); errors++;
	}
}

static void CheckDomainInitializerPolicy(char *data) {
	char *cp = strstr(data, " from ");
    if (cp) {
        *cp = '\0';
        CheckDomainInitializerEntry(cp + 6, data);
    } else {
        CheckDomainInitializerEntry(NULL, data);
    }
}

static void CheckDomainKeeperEntry(const char *domainname, const char *program) {
	if (!IsCorrectPath(domainname, 1, -1, -1) && !IsCorrectDomain(domainname)) {
		printf("%u: ERROR: '%s' is a bad domainname.\n", line, domainname); errors++;
	}
	if (program && !IsCorrectPath(program, 1, 0, -1)) {
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, program); errors++;
	}
}

static void CheckDomainKeeperPolicy(char *data) {
	char *cp = strstr(data, " from ");
    if (cp) {
        *cp = '\0';
        CheckDomainKeeperEntry(cp + 6, data);
    } else {
        CheckDomainKeeperEntry(data, NULL);
    }
}

static void CheckGroupPolicy(char *data) {
	char *cp = strchr(data, ' ');
	if (!cp) {
		printf("%u: ERROR: Too few parameters.\n", line); errors++;
		return;
	}
	*cp++ = '\0';
	if (!IsCorrectPath(data, 0, 0, 0)) {
		printf("%u: ERROR: '%s' is a bad group name.\n", line, data); errors++;
	}
	if (!IsCorrectPath(cp, 0, 0, 0)) {
		printf("%u: ERROR: '%s' is a bad pathname.\n", line, cp); errors++;
	}
}
		
static int checkpolicy_main(int argc, char *argv[]) {
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
	while (memset(shared_buffer, 0, shared_buffer_len), fgets(shared_buffer, shared_buffer_len - 1, stdin)) {
		static int domain = EOF;
		int is_select = 0, is_delete = 0, is_undelete = 0;
		char *cp = strchr(shared_buffer, '\n');
		line++;
		if (!cp) {
			printf("%u: ERROR: Line too long.\n", line); errors++;
			break;
		}
		*cp = '\0';
		{
			int c;
			for (c = 1; c < 256; c++) {
				if (c == '\t' || c == '\r' || (c >= ' ' && c < 127)) continue;
				if (strchr(shared_buffer, c)) {
					printf("%u: WARNING: Line contains illegal character (\\%03o).\n", line, c); warnings++;
					break;
				}
			}
		}
		NormalizeLine(shared_buffer);
		if (!shared_buffer[0]) continue;
		switch (policy_type) {
		case POLICY_TYPE_DOMAIN_POLICY:
			if (strncmp(shared_buffer, KEYWORD_DELETE, KEYWORD_DELETE_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_DELETE_LEN);
				is_delete = 1;
			} else if (strncmp(shared_buffer, KEYWORD_SELECT, KEYWORD_SELECT_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_SELECT_LEN);
				is_select = 1;
			} else if (strncmp(shared_buffer, KEYWORD_UNDELETE, KEYWORD_UNDELETE_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_UNDELETE_LEN);
				is_undelete = 1;
			}
			if (IsDomainDef(shared_buffer)) {
				if (!IsCorrectDomain(shared_buffer) || strlen(shared_buffer) >= CCS_MAX_PATHNAME_LEN) {
					printf("%u: ERROR: '%s' is a bad domainname.\n", line, shared_buffer); errors++;
				} else {
					if (is_delete) domain = EOF;
					else domain = 0;
				}
			} else if (is_select) {
				printf("%u: ERROR: Command 'select' is valid for selecting domains only.\n", line); errors++;
			} else if (is_undelete) {
				printf("%u: ERROR: Command 'undelete' is valid for undeleting domains only.\n", line); errors++;
			} else if (domain == EOF) {
				printf("%u: WARNING: '%s' is unprocessed because domain is not selected.\n", line, shared_buffer); warnings++;
			} else if (strncmp(shared_buffer, KEYWORD_USE_PROFILE, KEYWORD_USE_PROFILE_LEN) == 0) {
				unsigned int profile;
				RemoveHeader(shared_buffer, KEYWORD_USE_PROFILE_LEN);
				if (sscanf(shared_buffer, "%u", &profile) != 1 || profile >= 256) {
					printf("%u: ERROR: '%s' is a bad profile.\n", line, shared_buffer); errors++;
				}
			} else if (strncmp(shared_buffer, KEYWORD_ALLOW_CAPABILITY, KEYWORD_ALLOW_CAPABILITY_LEN) == 0) {
				CheckCapabilityPolicy(shared_buffer + KEYWORD_ALLOW_CAPABILITY_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_ALLOW_NETWORK, KEYWORD_ALLOW_NETWORK_LEN) == 0) {
				CheckNetworkPolicy(shared_buffer + KEYWORD_ALLOW_NETWORK_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_ALLOW_SIGNAL, KEYWORD_ALLOW_SIGNAL_LEN) == 0) {
				CheckSignalPolicy(shared_buffer + KEYWORD_ALLOW_SIGNAL_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_ALLOW_ARGV0, KEYWORD_ALLOW_ARGV0_LEN) == 0) {
				CheckArgv0Policy(shared_buffer + KEYWORD_ALLOW_ARGV0_LEN);
			} else {
				CheckFilePolicy(shared_buffer);
			}
			break;
		case POLICY_TYPE_EXCEPTION_POLICY:
			if (strncmp(shared_buffer, KEYWORD_DELETE, KEYWORD_DELETE_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_DELETE_LEN);
			}
			if (strncmp(shared_buffer, KEYWORD_DOMAIN_KEEPER, KEYWORD_DOMAIN_KEEPER_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_DOMAIN_KEEPER_LEN);
				if (!IsCorrectDomain(shared_buffer)) {
					printf("%u: ERROR: '%s' is a bad domainname.\n", line, shared_buffer); errors++;
				}
			} else if (strncmp(shared_buffer, KEYWORD_ALLOW_READ, KEYWORD_ALLOW_READ_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_ALLOW_READ_LEN);
				if (!IsCorrectPath(shared_buffer, 1, -1, -1)) {
					printf("%u: ERROR: '%s' is a bad pathname.\n", line, shared_buffer); errors++;
				}
			} else if (strncmp(shared_buffer, KEYWORD_INITIALIZER, KEYWORD_INITIALIZER_LEN) == 0) {
				CheckDomainInitializerPolicy(shared_buffer + KEYWORD_INITIALIZER_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_NO_INITIALIZER, KEYWORD_NO_INITIALIZER_LEN) == 0) {
				CheckDomainInitializerPolicy(shared_buffer + KEYWORD_NO_INITIALIZER_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_INITIALIZE_DOMAIN, KEYWORD_INITIALIZE_DOMAIN_LEN) == 0) {
				CheckDomainInitializerPolicy(shared_buffer + KEYWORD_INITIALIZE_DOMAIN_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_NO_INITIALIZE_DOMAIN, KEYWORD_NO_INITIALIZE_DOMAIN_LEN) == 0) {
				CheckDomainInitializerPolicy(shared_buffer + KEYWORD_NO_INITIALIZE_DOMAIN_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_KEEP_DOMAIN, KEYWORD_KEEP_DOMAIN_LEN) == 0) {
				CheckDomainKeeperPolicy(shared_buffer + KEYWORD_KEEP_DOMAIN_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_NO_KEEP_DOMAIN, KEYWORD_NO_KEEP_DOMAIN_LEN) == 0) {
				CheckDomainKeeperPolicy(shared_buffer + KEYWORD_NO_KEEP_DOMAIN_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_PATH_GROUP, KEYWORD_PATH_GROUP_LEN) == 0) {
				CheckGroupPolicy(shared_buffer + KEYWORD_PATH_GROUP_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_ALIAS, KEYWORD_ALIAS_LEN) == 0) {
				char *cp;
				RemoveHeader(shared_buffer, KEYWORD_ALIAS_LEN);
				if ((cp = strchr(shared_buffer, ' ')) == NULL) {
					printf("%u: ERROR: Too few parameters.\n", line); errors++;
				} else {
					*cp++ = '\0';
					if (!IsCorrectPath(shared_buffer, 1, -1, -1)) {
						printf("%u: ERROR: '%s' is a bad pathname.\n", line, shared_buffer); errors++;
					}
					if (!IsCorrectPath(cp, 1, -1, -1)) {
						printf("%u: ERROR: '%s' is a bad pathname.\n", line, cp); errors++;
					}
				}
			} else if (strncmp(shared_buffer, KEYWORD_AGGREGATOR, KEYWORD_AGGREGATOR_LEN) == 0) {
				char *cp;
				RemoveHeader(shared_buffer, KEYWORD_AGGREGATOR_LEN);
				if ((cp = strchr(shared_buffer, ' ')) == NULL) {
					printf("%u: ERROR: Too few parameters.\n", line); errors++;
				} else {
					*cp++ = '\0';
					if (!IsCorrectPath(shared_buffer, 1, 0, -1)) {
						printf("%u: ERROR: '%s' is a bad pattern.\n", line, shared_buffer); errors++;
					}
					if (!IsCorrectPath(cp, 1, -1, -1)) {
						printf("%u: ERROR: '%s' is a bad pathname.\n", line, cp); errors++;
					}
				}
			} else if (strncmp(shared_buffer, KEYWORD_FILE_PATTERN, KEYWORD_FILE_PATTERN_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_FILE_PATTERN_LEN);
				if (!IsCorrectPath(shared_buffer, 0, 1, 0)) {
					printf("%u: ERROR: '%s' is a bad pattern.\n", line, shared_buffer); errors++;
				}
			} else if (strncmp(shared_buffer, KEYWORD_DENY_REWRITE, KEYWORD_DENY_REWRITE_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_DENY_REWRITE_LEN);
				if (!IsCorrectPath(shared_buffer, 0, 0, 0)) {
					printf("%u: ERROR: '%s' is a bad pattern.\n", line, shared_buffer); errors++;
				}
			} else {
				printf("%u: ERROR: Unknown command '%s'.\n", line, shared_buffer); errors++;
			}
			break;
		case POLICY_TYPE_SYSTEM_POLICY:
			if (strncmp(shared_buffer, KEYWORD_DELETE, KEYWORD_DELETE_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_DELETE_LEN);
			}
			if (strncmp(shared_buffer, KEYWORD_ALLOW_MOUNT, KEYWORD_ALLOW_MOUNT_LEN) == 0) {
				CheckMountPolicy(shared_buffer + KEYWORD_ALLOW_MOUNT_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_DENY_UNMOUNT, KEYWORD_DENY_UNMOUNT_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_DENY_UNMOUNT_LEN);
				if (!IsCorrectPath(shared_buffer, 1, 0, 1)) {
					printf("%u: ERROR: '%s' is a bad pattern.\n", line, shared_buffer); errors++;
				}
			} else if (strncmp(shared_buffer, KEYWORD_ALLOW_CHROOT, KEYWORD_ALLOW_CHROOT_LEN) == 0) {
				RemoveHeader(shared_buffer, KEYWORD_ALLOW_CHROOT_LEN);
				if (!IsCorrectPath(shared_buffer, 1, 0, 1)) {
					printf("%u: ERROR: '%s' is a bad pattern.\n", line, shared_buffer); errors++;
				}
			} else if (strncmp(shared_buffer, KEYWORD_ALLOW_PIVOT_ROOT, KEYWORD_ALLOW_PIVOT_ROOT_LEN) == 0) {
				CheckPivotRootPolicy(shared_buffer + KEYWORD_ALLOW_PIVOT_ROOT_LEN);
			} else if (strncmp(shared_buffer, KEYWORD_DENY_AUTOBIND, KEYWORD_DENY_AUTOBIND_LEN) == 0) {
				CheckReservedPortPolicy(shared_buffer + KEYWORD_DENY_AUTOBIND_LEN);
			} else {
				printf("%u: ERROR: Unknown command '%s'.\n", line, shared_buffer); errors++;
			}
			break;
		}
	}
	put();
	printf("Total:   %u Line%s   %u Error%s   %u Warning%s\n", line, line > 1 ? "s" : "", errors, errors > 1 ? "s" : "", warnings, warnings > 1 ? "s" : "");
	return (errors ? 2 : (warnings ? 1 : 0));
}

/***** checkpolicy end *****/

/***** ccstree start *****/

static int status_fd = EOF;

static const char *ReadInfo(const pid_t pid, int *profile) {
	char *cp; /* caller must use get()/put(). */
	memset(shared_buffer, 0, shared_buffer_len);
	snprintf(shared_buffer, shared_buffer_len - 1, "%d\n", pid);
	write(status_fd, shared_buffer, strlen(shared_buffer));
	memset(shared_buffer, 0, shared_buffer_len);
	read(status_fd, shared_buffer, shared_buffer_len - 1);
	if ((cp = strchr(shared_buffer, ' ')) != NULL) {
		*profile = atoi(cp + 1);
		if ((cp = strchr(cp + 1, ' ')) != NULL) {
			return cp + 1;
		}
	}
	*profile = -1;
	return "<UNKNOWN>";
}

static TASK_ENTRY *task_list = NULL;
static int task_list_len = 0;

static void Dump(const pid_t pid, const int depth) {
	int i;
	for (i = 0; i < task_list_len; i++) {
		const char *info;
		char *name;
		int j, profile;
		if (pid != task_list[i].pid) continue;
		name = GetName(pid);
		get();
		info = ReadInfo(pid, &profile);
		printf("%3d", profile);
		for (j = 0; j < depth - 1; j++) printf("    ");
		for (; j < depth; j++) printf("  +-");
		printf(" %s (%u) %s\n", name, pid, info);
		put();
		free(name);
		task_list[i].done = 1;
	}
	for (i = 0; i < task_list_len; i++) {
		if (pid != task_list[i].ppid) continue;
		Dump(task_list[i].pid, depth + 1);
	}
}

static void DumpUnprocessed(void) {
	int i;
	for (i = 0; i < task_list_len; i++) {
		const char *info;
		char *name;
		int profile;
		const pid_t pid = task_list[i].pid;
		if (task_list[i].done) continue;
		name = GetName(task_list[i].pid);
		get();
		info = ReadInfo(pid, &profile);
		printf("%3d %s (%u) %s\n", profile, name, pid, info);
		put();
		free(name);
		task_list[i].done = 1;
	}
}
		
static int ccstree_main(int argc, char *argv[]) {
	static const char *policy_file = "/proc/ccs/info/.process_status";
	static int show_all = 0;
	if (access("/proc/ccs/", F_OK)) {
		fprintf(stderr, "You can't use this command for this kernel.\n");
		return 1;
	}
	if (argc > 1) {
		if (strcmp(argv[1], "-a") == 0) {
			show_all = 1;
		} else {
			fprintf(stderr, "Usage: %s [-a]\n", argv[0]);
			return 0;
		}
	}
	if ((status_fd = open(policy_file, O_RDWR)) == EOF) {
		fprintf(stderr, "Can't open %s\n", policy_file);
		return 1;
	}
	{
		struct dirent **namelist;
		int i, n = scandir("/proc/", &namelist, 0, 0);
		for (i = 0; i < n; i++) {
			pid_t pid;
			if (sscanf(namelist[i]->d_name, "%u", &pid) == 1) {
				char buffer[128], test[16];
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer) - 1, "/proc/%u/exe", pid);
				if (show_all || readlink(buffer, test, sizeof(test)) > 0) {
					task_list = (TASK_ENTRY *) realloc(task_list, (task_list_len + 1) * sizeof(TASK_ENTRY));
					task_list[task_list_len].pid = pid;
					task_list[task_list_len].ppid = GetPPID(pid);
					task_list[task_list_len].done = 0;
					task_list_len++;
				}
			}
			free((void *) namelist[i]);
		}
		if (n >= 0) free((void *) namelist);
	}
	Dump(1, 0);
	DumpUnprocessed();
	close(status_fd);
	return 0;
}

/***** ccstree end *****/

/***** ccs-queryd start *****/

static int ccsqueryd_main(int argc, char *argv[]) {
	const int domain_policy_fd = open("/proc/ccs/policy/domain_policy", O_WRONLY);
	static const int max_readline_history = 20;
	static const char **readline_history = NULL;
	static int readline_history_count = 0;
	if (argc > 1) {
		printf("Usage: %s\n\n", argv[0]);
		printf("This program is used for granting access requests manually.\n");
		printf("This program shows access requests that are about to rejected by the kernel's decision.\n");
		printf("If you answer before the kernel's decision taken effect, your decision will take effect.\n");
		printf("You can use this program to respond to accidental access requests triggered by non-routine tasks (such as restarting daemons after updating).\n");
		printf("To terminate this program, use 'Ctrl-C'.\n");
		return 0;
	}
	query_fd = open("/proc/ccs/policy/query", O_RDWR);
	if (query_fd == EOF) {
		fprintf(stderr, "You can't run this utility for this kernel.\n");
		return 1;
	} else if (write(query_fd, "", 0) != 0) {
		fprintf(stderr, "You need to register this program to /proc/ccs/policy/manager to run this program.\n");
		return 1;
	}
	readline_history = malloc(max_readline_history * sizeof(const char *));
	write(query_fd, "\n", 1);
	initscr();
	cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    clear(); refresh();
	while (1) {
		static int first = 1;
		static unsigned int prev_serial = 0;
		static const int buffer_len = 16384;
		static char *buffer = NULL, *prev_buffer = NULL;
		fd_set rfds;
		unsigned int serial;
		char *cp;
		if (!buffer && (buffer = malloc(buffer_len)) == NULL) break;
		if (!prev_buffer) {
			if ((prev_buffer = malloc(buffer_len)) == NULL) break;
			memset(prev_buffer, 0, buffer_len);
		}
		// Wait for query.
		FD_ZERO(&rfds);
		FD_SET(query_fd, &rfds);
		select(query_fd + 1, &rfds, NULL, NULL, NULL);
		if (!FD_ISSET(query_fd, &rfds)) continue;
		
		// Read query.
		memset(buffer, 0, buffer_len);
		if (read(query_fd, buffer, buffer_len - 1) <= 0) continue;
		if ((cp = strchr(buffer, '\n')) == NULL) continue;
		*cp = '\0';
		
		// Get query number.
		if (sscanf(buffer, "Q%u", &serial) != 1) continue;
		memmove(buffer, cp + 1, strlen(cp + 1) + 1);
		
		if (!first && prev_serial == serial) {
			sleep(1);
			write(query_fd, "\n", 1);
			continue;
		}
		first = 0;
		prev_serial = serial;
		timeout(1000);
			
		// Is this domain query?
		if (strncmp(buffer, "<kernel>", 8) == 0 && (buffer[8] == '\0' || buffer[8] == ' ') && (cp = strchr(buffer, '\n')) != NULL) {
			int c = 0;
			// Check for same domain.
			*cp++ = '\0';
			if (strcmp(buffer, prev_buffer)) {
				printw("----------------------------------------\n");
				memmove(prev_buffer, buffer, strlen(buffer) + 1);
			}
			printw("%s\n", buffer);
			printw("%s", cp);
			printw("Allow? ('Y'es/Yes and 'A'ppend to policy/'N'o):"); refresh();
			while (1) {
				c = getch2();
				if (c == 'Y' || c == 'y' || c == 'N' || c == 'n' || c == 'A' || c == 'a') break;
				write(query_fd, "\n", 1);
			}
			printw("%c\n", c); refresh();
			
			// Append to domain policy.
			if (c == 'A' || c == 'a') {
				int y, x;
				char *line;
				getyx(stdscr, y, x);
				if ((line = strchr(cp, '\n')) != NULL) *line = '\0';
				initial_readline_data = cp;
				readline_history_count = simple_add_history(cp, readline_history, readline_history_count, max_readline_history);
				line = simple_readline(y, 0, "Enter new entry> ", readline_history, readline_history_count, 4000, 8);
				printw("\n"); refresh();
				if (line && *line) {
					readline_history_count = simple_add_history(line, readline_history, readline_history_count, max_readline_history);
					write(domain_policy_fd, buffer, strlen(buffer));
					write(domain_policy_fd, "\n", 1);
					write(domain_policy_fd, line, strlen(line));
					write(domain_policy_fd, "\n", 1);
					printw("Added '%s'.\n", line);
				} else {
					printw("None added.\n", line);
				}
				refresh();
				free(line);
			}

			// Write answer.
			snprintf(buffer, buffer_len - 1, "A%u=%u\n", serial, c == 'Y' || c == 'y' || c == 'A' || c == 'a' ? 1 : 2);
			write(query_fd, buffer, strlen(buffer));
		} else {
			int c;
			printw("----------------------------------------\n");
			prev_buffer[0] = '\0';
			printw("%s", buffer);
			printw("Allow? ('Y'es/'N'o):"); refresh();
			while (1) {
				c = getch2();
				if (c == 'Y' || c == 'y' || c == 'N' || c == 'n') break;
				write(query_fd, "\n", 1);
			}
			printw("%c\n", c); refresh();
			
			// Write answer.
			snprintf(buffer, buffer_len - 1, "A%u=%u\n", serial, c == 'Y' || c == 'y' ? 1 : 2);
			write(query_fd, buffer, strlen(buffer));
		}
		printw("\n"); refresh();
	}
	endwin();
	return 0;
}

/***** ccs-queryd end *****/

/***** ccs-auditd start *****/

static int ccsauditd_main(int argc, char *argv[]) {
	static const char * const procfile_path[CCS_AUDITD_MAX_FILES] = {
		"/proc/ccs/info/grant_log",
		"/proc/ccs/info/reject_log"
	};	
	int i, fd_in[CCS_AUDITD_MAX_FILES], fd_out[CCS_AUDITD_MAX_FILES];
	const char *logfile_path[2] = { "/dev/null", "/dev/null" };
	if (access("/proc/ccs/policy/", F_OK)) {
		fprintf(stderr, "You can't run this daemon for this kernel.\n");
		return 0;
	}
	if (argc < 3) {
		fprintf(stderr, "%s grant_log_file reject_log_file\n" "  These files may /dev/null, if needn't to be saved.\n", argv[0]);
		return 0;
	}
	logfile_path[0] = argv[1]; logfile_path[1] = argv[2];
	{ // Get exclusive lock.
		int fd = open("/proc/self/exe", O_RDONLY); if (flock(fd, LOCK_EX | LOCK_NB) == EOF) return 0;
	}
	umask(0);
	for (i = 0; i < CCS_AUDITD_MAX_FILES; i++) {
		if ((fd_out[i] = open(logfile_path[i], O_WRONLY | O_CREAT | O_APPEND, 0600)) == EOF) {
			fprintf(stderr, "Can't open %s for writing.\n", logfile_path[i]);
			return 1;
		}
	}
	switch(fork()) {
	case 0:
		break;
	case -1:
		fprintf(stderr, "Can't fork()\n");
		return 1;
	default:
		return 0;
	}
	if (setsid() == EOF) {
		fprintf(stderr, "Can't setsid()\n");
		return 1;
	}
	switch(fork()) {
	case 0:
		break;
	case -1:
		fprintf(stderr, "Can't fork()\n");
		return 1;
	default:
		return 0;
	}
	if (chdir("/")) {
		fprintf(stderr, "Can't chdir()\n");
		return 1;
	}
	close(0); close(1); close(2);
	openlog("ccs-auditd", 0,  LOG_USER);
	for (i = 0; i < CCS_AUDITD_MAX_FILES; i++) {
		if ((fd_in[i] = open(procfile_path[i], O_RDONLY)) == EOF) {
			syslog(LOG_WARNING, "Can't open %s for reading.\n", procfile_path[i]);
			return 1;
		}
	}
	syslog(LOG_WARNING, "Started.\n");
	while (1) {
		static const int buffer_len = 16384;
		static char *buffer = NULL;
		char timestamp[128];
		fd_set rfds;
		if (!buffer && (buffer = malloc(buffer_len)) == NULL) break;
		FD_ZERO(&rfds);
		for (i = 0; i < CCS_AUDITD_MAX_FILES; i++) FD_SET(fd_in[i], &rfds);
		// Wait for data.
		if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) == EOF) break;
		for (i = 0; i < CCS_AUDITD_MAX_FILES; i++) {
			time_t stamp;
			char *cp;
			int len;
			if (!FD_ISSET(fd_in[i], &rfds)) continue;
			memset(buffer, 0, buffer_len);
			if (read(fd_in[i], buffer, buffer_len - 1) < 0) continue;
			memset(timestamp, 0, sizeof(timestamp));
			if (sscanf(buffer, "#timestamp=%lu", &stamp) == 1 && (cp = strchr(buffer, ' ')) != NULL) {
				struct tm *tm = localtime(&stamp);
				snprintf(timestamp, sizeof(timestamp) - 1, "#%04d-%02d-%02d %02d:%02d:%02d#", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
				memmove(buffer, cp, strlen(cp) + 1);
			}
			// Open destination file.
			if (access(logfile_path[i], F_OK)) {
				close(fd_out[i]);
				if ((fd_out[i] = open(logfile_path[i], O_WRONLY | O_CREAT | O_APPEND, 0600)) == EOF) {
					syslog(LOG_WARNING, "Can't open %s for writing.\n", logfile_path[i]);
					goto out;
				}
			}
			len = strlen(timestamp);
			write(fd_out[i], timestamp, len);
			len = strlen(buffer);
			write(fd_out[i], buffer, len);
			write(fd_out[i], "\n", 1);
			fsync(fd_out[i]);
		}
	}
 out: ;
	syslog(LOG_WARNING, "Terminated.\n");
	closelog();
	return 0;
}

/***** ccs-auditd end *****/

/***** patternize start *****/

static int patternize_main(int argc, char *argv[]) {
	struct path_info *pattern_list = malloc(argc * sizeof(struct path_info));
	if (!pattern_list) OutOfMemory();
	int i;
	for (i = 0; i < argc; i++) {
		pattern_list[i].name = argv[i];
		fill_path_info(&pattern_list[i]);
	}
	get();
	while (freadline(stdin)) {
		char *sp = shared_buffer, *cp;
		int first = 1;
		int check_second = 0;
		int disabled = 0;
		while ((cp = strsep(&sp, " ")) != NULL) {
		check:
			if (first) {
				unsigned int perm;
				if (sscanf(cp, "%u", &perm) == 1 && (perm & 1) == 1) {
					/* Is this entry for a program? */
					check_second = 1;
				} else if (strcmp(cp, "<kernel>") == 0 || strcmp(cp, "use_profile") == 0
						   || strcmp(cp, "allow_capability") == 0 || strcmp(cp, "allow_signal") == 0 ||
						   strcmp(cp, "allow_network") == 0) {
					/* This entry is not pathname related permission. I don't convert. */
					disabled = 1;
				}
			} else if (disabled) {
				// Nothing to do.
			} else if (check_second) {
				check_second = 0;
				if (*cp == '/' && * (strchr(cp, '\0') - 1) != '/') { /* Don't convert @path_group . */
					/* This entry is for a program. I don't convert. */
					disabled = 1;
				}
				goto check;
			} else if (strcmp(cp, "if") == 0) {
				/* Don't convert after condition part. */
				disabled = 1;
			} else if (!PathContainsPattern(cp)) {
				int i;
				struct path_info cp2;
				cp2.name = cp;
				fill_path_info(&cp2);
				for (i = 1; i < argc; i++) {
					if (PathMatchesToPattern(&cp2, &pattern_list[i])) {
						cp = argv[i]; break;
					}
				}
			}
			if (!first) putchar(' ');
			first = 0;
			printf("%s", cp);
		}
		putchar('\n');
	}
	put();
	free(pattern_list);
	return 0;
}

/***** patternize end *****/

/***** MAIN END *****/

int main(int argc, char *argv[]) {
	const char *argv0 = argv[0];
	if (!argv0) {
		fprintf(stderr, "Function not specified.\n");
		return 1;
	}
	if (strrchr(argv0, '/')) argv0 = strrchr(argv0, '/') + 1;
#ifdef NEED_SORTPOLICY
	if (strcmp(argv0, "sortpolicy") == 0) return sortpolicy_main(argc, argv);
#endif
#ifdef NEED_SETPROFILE
	if (strcmp(argv0, "setprofile") == 0) return setprofile_main(argc, argv);
#endif
#ifdef NEED_SETLEVEL
	if (strcmp(argv0, "setlevel") == 0) return setlevel_main(argc, argv);
#endif
#ifdef NEED_SAVEPOLICY
	if (strcmp(argv0, "savepolicy") == 0) return savepolicy_main(argc, argv);
#endif
#ifdef NEED_PATHMATCH
	if (strcmp(argv0, "pathmatch") == 0) return pathmatch_main(argc, argv);
#endif
#ifdef NEED_LOADPOLICY
	if (strcmp(argv0, "loadpolicy") == 0) return loadpolicy_main(argc, argv);
#endif
#ifdef NEED_LDWATCH
	if (strcmp(argv0, "ld-watch") == 0) return ldwatch_main(argc, argv);
#endif
#ifdef NEED_FINDTEMP
	if (strcmp(argv0, "findtemp") == 0) return findtemp_main(argc, argv);
#endif
#ifdef NEED_EDITPOLICY
	if (strcmp(argv0, "editpolicy") == 0 || strcmp(argv0, "editpolicy_offline") == 0) return editpolicy_main(argc, argv);
#endif
#ifdef NEED_CHECKPOLICY
	if (strcmp(argv0, "checkpolicy") == 0) return checkpolicy_main(argc, argv);
#endif
#ifdef NEED_CCSTREE
	if (strcmp(argv0, "ccstree") == 0) return ccstree_main(argc, argv);
#endif
#ifdef NEED_CCSQUERYD
	if (strcmp(argv0, "ccs-queryd") == 0) return ccsqueryd_main(argc, argv);
#endif
#ifdef NEED_CCSAUDITD
	if (strcmp(argv0, "ccs-auditd") == 0) return ccsauditd_main(argc, argv);
#endif
#ifdef NEED_PATTERNIZE
	if (strcmp(argv0, "patternize") == 0) return patternize_main(argc, argv);
#endif
	/*
	 * Unlike busybox, I don't use argv[1] if argv[0] is the name of this program
	 * because it is dangerous to allow updating policies via unchecked argv[1].
	 * You should use either "symbolic links with 'alias' directive" or "hard links".
	 */
	printf("ccstools version 1.4-rc1 build 2007/03/25\n");
	fprintf(stderr, "Function %s not implemented.\n", argv0);
	return 1;
}
