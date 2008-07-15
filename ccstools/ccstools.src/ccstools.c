/*
 * ccstools.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.3   2008/07/15
 *
 */
#include "ccstools.h"

/***** UTILITY FUNCTIONS START *****/

void OutOfMemory(void) {
	fprintf(stderr, "Out of memory. Aborted.\n");
	exit(1);
}

void NormalizeLine(unsigned char *line) {
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

int IsDomainDef(const unsigned char *domainname) {
	return strncmp(domainname, ROOT_NAME, ROOT_NAME_LEN) == 0 && (domainname[ROOT_NAME_LEN] == '\0' || domainname[ROOT_NAME_LEN] == ' ');
}

int IsCorrectDomain(const unsigned char *domainname) {
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

void fprintf_encoded(FILE *fp, const char *pathname) {
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

int decode(const char *ascii, char *bin) {
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

void RemoveHeader(char *line, const int len) {
	memmove(line, line + len, strlen(line + len) + 1); 
}

int IsCorrectPath(const char *filename, const int start_type, const int pattern_type, const int end_type) {
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
			case '-':   /* "\-" */
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

static int FileMatchesToPattern2(const char *filename, const char *filename_end, const char *pattern, const char *pattern_end) {
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
						if (FileMatchesToPattern2(filename + i, filename_end, pattern + 1, pattern_end)) return 1;
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
						if (FileMatchesToPattern2(filename + i, filename_end, pattern + 1, pattern_end)) return 1;
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

int FileMatchesToPattern(const char *filename, const char *filename_end, const char *pattern, const char *pattern_end) {
	const char *pattern_start = pattern;
	int first = 1;
	int result;
	while (pattern < pattern_end - 1) {
		if (*pattern++ != '\\' || *pattern++ != '-') continue;
		result = FileMatchesToPattern2(filename, filename_end, pattern_start, pattern - 2);
		if (first) result = !result;
		if (result) return 0;
		first = 0;
		pattern_start = pattern;
	}
	result = FileMatchesToPattern2(filename, filename_end, pattern_start, pattern_end);
	return first ? result : !result;
}

int string_compare(const void *a, const void *b) {
	return strcmp(* (char **) a, * (char **) b);
}

int pathcmp(const struct path_info *a, const struct path_info *b) {
	return a->hash != b->hash || strcmp(a->name, b->name);
}

void fill_path_info(struct path_info *ptr) {
	const char *name = ptr->name;
	const int len = strlen(name);
	ptr->total_len = len;
	ptr->const_len = const_part_length(name);
	ptr->is_dir = len && (name[len - 1] == '/');
	ptr->is_patterned = (ptr->const_len < len);
	ptr->hash = full_name_hash(name, len);
	ptr->depth = PathDepth(name);
}

const struct path_info *SaveName(const char *name) {
	static struct free_memory_block_list fmb_list = { NULL, NULL, 0 };
	static struct savename_entry name_list[SAVENAME_MAX_HASH]; /* The list of names. */
	struct savename_entry *ptr, *prev = NULL;
	unsigned int hash;
	struct free_memory_block_list *fmb = &fmb_list;
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
			if ((cp = (char *) malloc(PAGE_SIZE)) == NULL || (fmb->next = (struct free_memory_block_list *) alloc_element(sizeof(struct free_memory_block_list))) == NULL) OutOfMemory();
			memset(cp, 0, PAGE_SIZE);
			fmb = fmb->next;
			fmb->ptr = cp;
			fmb->len = PAGE_SIZE;
		}
	}
	if ((ptr = (struct savename_entry *) alloc_element(sizeof(struct savename_entry))) == NULL) OutOfMemory();
	memset(ptr, 0, sizeof(struct savename_entry));
	ptr->entry.name = fmb->ptr;
	memmove(fmb->ptr, name, len);
	fill_path_info(&ptr->entry);
	fmb->ptr += len;
	fmb->len -= len;
	prev->next = ptr; /* prev != NULL because name_list is not empty. */
	if (fmb->len == 0) {
		struct free_memory_block_list *ptr = &fmb_list;
		while (ptr->next != fmb) ptr = ptr->next; ptr->next = fmb->next;
	}
 out:
	return ptr ? &ptr->entry : NULL;
}

char *shared_buffer = NULL;
static int buffer_lock = 0;
void get(void) {
	if (buffer_lock) OutOfMemory();
	if (!shared_buffer && (shared_buffer = malloc(shared_buffer_len)) == NULL) OutOfMemory();
	buffer_lock++;
}
void put(void) {
	if (buffer_lock != 1) OutOfMemory();
	buffer_lock--;
}
int freadline(FILE *fp) {
	char *cp;
	memset(shared_buffer, 0, shared_buffer_len);
	if (fgets(shared_buffer, shared_buffer_len - 1, fp) == NULL ||
		(cp = strchr(shared_buffer, '\n')) == NULL) return 0;
	*cp = '\0';
	NormalizeLine(shared_buffer);
	return 1;
}

/***** UTILITY FUNCTIONS END *****/

extern int sortpolicy_main(int argc, char *argv[]);
extern int setprofile_main(int argc, char *argv[]);
extern int setlevel_main(int argc, char *argv[]);
extern int savepolicy_main(int argc, char *argv[]);
extern int pathmatch_main(int argc, char *argv[]);
extern int loadpolicy_main(int argc, char *argv[]);
extern int ldwatch_main(int argc, char *argv[]);
extern int findtemp_main(int argc, char *argv[]);
extern int editpolicy_main(int argc, char *argv[]);
extern int checkpolicy_main(int argc, char *argv[]);
extern int ccstree_main(int argc, char *argv[]);
extern int ccsqueryd_main(int argc, char *argv[]);
extern int ccsauditd_main(int argc, char *argv[]);
extern int patternize_main(int argc, char *argv[]);

const char *proc_policy_dir           = "/proc/ccs/",
	*disk_policy_dir              = "/etc/ccs/",
	*proc_policy_domain_policy    = "/proc/ccs/domain_policy",
	*disk_policy_domain_policy    = "/etc/ccs/domain_policy.conf",
	*proc_policy_exception_policy = "/proc/ccs/exception_policy",
	*disk_policy_exception_policy = "/etc/ccs/exception_policy.conf",
	*proc_policy_system_policy    = "/proc/ccs/system_policy",
	*disk_policy_system_policy    = "/etc/ccs/system_policy.conf",
	*proc_policy_profile          = "/proc/ccs/profile",
	*disk_policy_profile          = "/etc/ccs/profile.conf",
	*proc_policy_manager          = "/proc/ccs/manager",
	*disk_policy_manager          = "/etc/ccs/manager.conf",
	*proc_policy_query            = "/proc/ccs/query",
	*proc_policy_grant_log        = "/proc/ccs/grant_log",
	*proc_policy_reject_log       = "/proc/ccs/reject_log",
	*proc_policy_domain_status    = "/proc/ccs/.domain_status",
	*proc_policy_process_status   = "/proc/ccs/.process_status";

int main(int argc, char *argv[]) {
	const char *argv0 = argv[0];
	if (!argv0) {
		fprintf(stderr, "Function not specified.\n");
		return 1;
	}
	if (access("/sys/kernel/security/tomoyo/", F_OK) == 0) {
		proc_policy_dir              = "/sys/kernel/security/tomoyo/";
		disk_policy_dir              = "/etc/tomoyo/";
		proc_policy_domain_policy    = "/sys/kernel/security/tomoyo/domain_policy";
		disk_policy_domain_policy    = "/etc/tomoyo/domain_policy.conf";
		proc_policy_exception_policy = "/sys/kernel/security/tomoyo/exception_policy";
		disk_policy_exception_policy = "/etc/tomoyo/exception_policy.conf";
		proc_policy_system_policy    = "/sys/kernel/security/tomoyo/system_policy";
		disk_policy_system_policy    = "/etc/tomoyo/system_policy.conf";
		proc_policy_profile          = "/sys/kernel/security/tomoyo/profile";
		disk_policy_profile          = "/etc/tomoyo/profile.conf";
		proc_policy_manager          = "/sys/kernel/security/tomoyo/manager";
		disk_policy_manager          = "/etc/tomoyo/manager.conf";
		proc_policy_query            = "/sys/kernel/security/tomoyo/query";
		proc_policy_grant_log        = "/sys/kernel/security/tomoyo/grant_log";
		proc_policy_reject_log       = "/sys/kernel/security/tomoyo/reject_log";
		proc_policy_domain_status    = "/sys/kernel/security/tomoyo/.domain_status";
		proc_policy_process_status   = "/sys/kernel/security/tomoyo/.process_status";
	} else if (access("/proc/tomoyo/", F_OK) == 0) {
		proc_policy_dir              = "/proc/tomoyo/";
		disk_policy_dir              = "/etc/tomoyo/";
		proc_policy_domain_policy    = "/proc/tomoyo/domain_policy";
		disk_policy_domain_policy    = "/etc/tomoyo/domain_policy.conf";
		proc_policy_exception_policy = "/proc/tomoyo/exception_policy";
		disk_policy_exception_policy = "/etc/tomoyo/exception_policy.conf";
		proc_policy_system_policy    = "/proc/tomoyo/system_policy";
		disk_policy_system_policy    = "/etc/tomoyo/system_policy.conf";
		proc_policy_profile          = "/proc/tomoyo/profile";
		disk_policy_profile          = "/etc/tomoyo/profile.conf";
		proc_policy_manager          = "/proc/tomoyo/manager";
		disk_policy_manager          = "/etc/tomoyo/manager.conf";
		proc_policy_query            = "/proc/tomoyo/query";
		proc_policy_grant_log        = "/proc/tomoyo/grant_log";
		proc_policy_reject_log       = "/proc/tomoyo/reject_log";
		proc_policy_domain_status    = "/proc/tomoyo/.domain_status";
		proc_policy_process_status   = "/proc/tomoyo/.process_status";
	}
	if (strrchr(argv0, '/')) argv0 = strrchr(argv0, '/') + 1;
retry:
	if (strcmp(argv0, "sortpolicy") == 0) return sortpolicy_main(argc, argv);
	if (strcmp(argv0, "setprofile") == 0) return setprofile_main(argc, argv);
	if (strcmp(argv0, "setlevel") == 0) return setlevel_main(argc, argv);
	if (strcmp(argv0, "savepolicy") == 0) return savepolicy_main(argc, argv);
	if (strcmp(argv0, "pathmatch") == 0) return pathmatch_main(argc, argv);
	if (strcmp(argv0, "loadpolicy") == 0) return loadpolicy_main(argc, argv);
	if (strcmp(argv0, "ld-watch") == 0) return ldwatch_main(argc, argv);
	if (strcmp(argv0, "findtemp") == 0) return findtemp_main(argc, argv);
	if (strcmp(argv0, "editpolicy") == 0 || strcmp(argv0, "editpolicy_offline") == 0) return editpolicy_main(argc, argv);
	if (strcmp(argv0, "checkpolicy") == 0) return checkpolicy_main(argc, argv);
	if (strcmp(argv0, "ccstree") == 0) return ccstree_main(argc, argv);
	if (strcmp(argv0, "ccs-queryd") == 0) return ccsqueryd_main(argc, argv);
	if (strcmp(argv0, "ccs-auditd") == 0) return ccsauditd_main(argc, argv);
	if (strcmp(argv0, "patternize") == 0) return patternize_main(argc, argv);
	if (strncmp(argv0, "ccs-", 4) == 0) {
		argv0 += 4;
		goto retry;
	}
	/*
	 * Unlike busybox, I don't use argv[1] if argv[0] is the name of this program
	 * because it is dangerous to allow updating policies via unchecked argv[1].
	 * You should use either "symbolic links with 'alias' directive" or "hard links".
	 */
	printf("ccstools version 1.6.3 build 2008/07/15\n");
	fprintf(stderr, "Function %s not implemented.\n", argv0);
	return 1;
}
