/*
 * editpolicy.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2007  NTT DATA CORPORATION
 *
 * Version: 1.5.0-pre   2007/08/12
 *
 */
#include "ccstools.h"

/// add color start
#ifdef COLOR_ON
#define OFF 0
#define ON !OFF
enum color_pair {	NORMAL,
			DOMAIN_HEAD, DOMAIN_CURSOR,
			SYSTEM_HEAD, SYSTEM_CURSOR,
			EXCEPTION_HEAD, EXCEPTION_CURSOR,
			ACL_HEAD, ACL_CURSOR,
			DISP_ERR }; 

static void ColorInit(void){
	start_color();
	init_pair(DOMAIN_HEAD, COLOR_BLACK, COLOR_GREEN);
	init_pair(DOMAIN_CURSOR, COLOR_BLACK, COLOR_GREEN);
	
	init_pair(SYSTEM_HEAD, COLOR_WHITE, COLOR_BLUE);
	init_pair(SYSTEM_CURSOR, COLOR_WHITE, COLOR_BLUE);

	init_pair(EXCEPTION_HEAD, COLOR_BLACK, COLOR_CYAN);
	init_pair(EXCEPTION_CURSOR, COLOR_BLACK, COLOR_CYAN);
	
	init_pair(ACL_HEAD, COLOR_BLACK, COLOR_YELLOW);
	init_pair(ACL_CURSOR, COLOR_BLACK, COLOR_YELLOW);
	
	init_pair(DISP_ERR, COLOR_RED, COLOR_BLACK);
}

static void ColorSave(int flg) {
	static int save_color = NORMAL;
	if (flg == ON)
		save_color = getattrs(stdscr);
	else
		attrset(save_color);
}

#define colorChange(attr, flg)	{flg ? attron(COLOR_PAIR(attr)) : attroff(COLOR_PAIR(attr));}
#define attrChange(attr, flg)	{flg ? attron(attr) : attroff(attr);}

#define sttrSave()		ColorSave(ON)
#define sttrRestore()	ColorSave(OFF)

#define colorHead()	( \
	(current_screen == SCREEN_DOMAIN_LIST) ? DOMAIN_HEAD \
			: (current_screen == SCREEN_SYSTEM_LIST) ? SYSTEM_HEAD \
			: (current_screen == SCREEN_EXCEPTION_LIST) ? EXCEPTION_HEAD \
			: ACL_HEAD )

#define colorCursor()	( \
	(current_screen == SCREEN_DOMAIN_LIST) ? DOMAIN_CURSOR \
			: (current_screen == SCREEN_SYSTEM_LIST) ? SYSTEM_CURSOR \
			: (current_screen == SCREEN_EXCEPTION_LIST) ? EXCEPTION_CURSOR \
			: ACL_CURSOR )



#else	// no color

#define ColorInit()
#define colorChange(attr, flg)
#define attrChange(attr, flg)
#define sttrSave()
#define sttrRestore()
#define colorHead()
#define colorCursor()

#endif
/// add color end

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

static struct group_entry *group_list = NULL;
static int group_list_len = 0;

static struct domain_info *domain_list = NULL, *shadow_domain_list = NULL;
static int domain_list_count = 0, shadow_domain_list_count = 0;
static unsigned char *domain_list_selected = NULL;

static void SwapDomainList(void) {
	struct domain_info *tmp_list = domain_list;
	int tmp_list_count = domain_list_count;
	domain_list = shadow_domain_list;
	domain_list_count = shadow_domain_list_count;
	shadow_domain_list = tmp_list;
	shadow_domain_list_count = tmp_list_count;
}

static const char *DomainName(const int index) {
	return domain_list[index].domainname->name;
}

static const char *GetLastName(const int index) {
	const char *cp0 = DomainName(index), *cp1;
	if ((cp1 = strrchr(cp0, ' ')) != NULL) return cp1 + 1;
	return cp0;
}

static int AddStringEntry(const char *entry, const int index) {
	const struct path_info **acl_ptr;
	int acl_count;
	const struct path_info *cp;
	int i;
	if (index < 0 || index >= domain_list_count) {
		fprintf(stderr, "AddStringEntry: ERROR: domain is out of range.\n");
		return -EINVAL;
	}
	if (!entry || !*entry) return -EINVAL;
	if ((cp = SaveName(entry)) == NULL) OutOfMemory();

	acl_ptr = domain_list[index].string_ptr;
	acl_count = domain_list[index].string_count;

	// Check for the same entry.
	for (i = 0; i < acl_count; i++) {
		// Faster comparison, for they are SaveName'd.
		if (cp == acl_ptr[i]) return 0;
	}

	if ((acl_ptr = (const struct path_info **) realloc(acl_ptr, (acl_count + 1) * sizeof(const struct path_info *))) == NULL) OutOfMemory();
	acl_ptr[acl_count++] = cp;
	domain_list[index].string_ptr = acl_ptr;
	domain_list[index].string_count = acl_count;
	return 0;
}

static int DelStringEntry(const char *entry, const int index) {
	const struct path_info **acl_ptr;
	int acl_count;
	const struct path_info *cp;
	int i;
	if (index < 0 || index >= domain_list_count) {
		fprintf(stderr, "DelStringEntry: ERROR: domain is out of range.\n");
		return -EINVAL;
	}
	if (!entry || !*entry) return -EINVAL;
	if ((cp = SaveName(entry)) == NULL) OutOfMemory();

	acl_ptr = domain_list[index].string_ptr;
	acl_count = domain_list[index].string_count;

	for (i = 0; i < acl_count; i++) {
		// Faster comparison, for they are SaveName'd.
		if (cp != acl_ptr[i]) continue;
		domain_list[index].string_count--;
		for (; i < acl_count - 1; i++) acl_ptr[i] = acl_ptr[i + 1];
		return 0;
	}
	return -ENOENT;
}

static void ClearDomainPolicy(void) {
	int index;
	for (index = 0; index < domain_list_count; index++) {
		free(domain_list[index].string_ptr);
		domain_list[index].string_ptr = NULL;
		domain_list[index].string_count = 0;
	}
	free(domain_list);
	domain_list = NULL;
	domain_list_count = 0;
}

static int FindDomain(const char *domainname0, const int is_domain_initializer_source, const int is_domain_deleted) {
	int i;
	struct path_info domainname;
	domainname.name = domainname0;
	fill_path_info(&domainname);
	for (i = 0; i < domain_list_count; i++) {
		if (domain_list[i].is_domain_initializer_source == is_domain_initializer_source && domain_list[i].is_domain_deleted == is_domain_deleted && !pathcmp(&domainname, domain_list[i].domainname)) return i;
	}
	return EOF;
}

static int FindOrAssignNewDomain(const char *domainname, const int is_domain_initializer_source, const int is_domain_deleted) {
	const struct path_info *saved_domainname;
	int index;
	if ((index = FindDomain(domainname, is_domain_initializer_source, is_domain_deleted)) == EOF) {
		if (IsCorrectDomain(domainname)) {
			if ((domain_list = (struct domain_info *) realloc(domain_list, (domain_list_count + 1) * sizeof(struct domain_info))) == NULL) OutOfMemory();
			memset(&domain_list[domain_list_count], 0, sizeof(struct domain_info));
			if ((saved_domainname = SaveName(domainname)) == NULL) OutOfMemory();
			domain_list[domain_list_count].domainname = saved_domainname;
			domain_list[domain_list_count].is_domain_initializer_source = is_domain_initializer_source;
			domain_list[domain_list_count].is_domain_deleted = is_domain_deleted;
			index = domain_list_count++;
		} else {
			fprintf(stderr, "FindOrAssignNewDomain: Invalid domainname '%s'\n", domainname);
		}
	}
	return index;
}

static void DeleteDomain(const int index) {
	if (index > 0 && index < domain_list_count) {
		int i;
		free(domain_list[index].string_ptr);
		for (i = index; i < domain_list_count - 1; i++) domain_list[i] = domain_list[i + 1];
		domain_list_count--;
	}
}

static int domainname_compare(const void *a, const void *b) {
	return strcmp(((struct domain_info *) a)->domainname->name, ((struct domain_info *) b)->domainname->name);
}

static int path_info_compare(const void *a, const void *b) {
	const char *a0 = (* (struct path_info **) a)->name;
	const char *b0 = (* (struct path_info **) b)->name;
	if (*a0 && *b0) return strcmp(a0 + 1, b0 + 1);
	return 0;
}

static void SortPolicy(void) {
	int i;
	qsort(domain_list, domain_list_count, sizeof(struct domain_info), domainname_compare);
	for (i = 0; i < domain_list_count; i++) qsort(domain_list[i].string_ptr, domain_list[i].string_count, sizeof(struct path_info *), path_info_compare);
}

static int WriteDomainPolicy(const int fd) {
	int i, j;
	for (i = 0; i < domain_list_count; i++) {
		const struct path_info **string_ptr = domain_list[i].string_ptr;
		const int string_count = domain_list[i].string_count;
		write(fd, domain_list[i].domainname->name, domain_list[i].domainname->total_len);
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
	if (domain_list_count == shadow_domain_list_count) {
		int i, j;
		for (i = 0; i < domain_list_count; i++) {
			const struct path_info **string_ptr = domain_list[i].string_ptr;
			const int string_count = domain_list[i].string_count;
			if (string_count == shadow_domain_list[i].string_count) {
				const struct path_info **ptr = shadow_domain_list[i].string_ptr;
				for (j = 0; j < string_count; j++) {
					/* Faster comparison, for they are SaveName'd and sorted pointers. */
					if (string_ptr[j] != ptr[j]) break;
				}
				if (j == string_count) continue;
			}
			break;
		}
		if (i == domain_list_count) return 1;
	}
	return 0;
}

static int IsKeeperDomain(const int index) {
	return domain_list[index].is_domain_keeper;
}

static int IsInitializerSource(const int index) {
	return domain_list[index].is_domain_initializer_source;
}

static int IsInitializerTarget(const int index) {
	return domain_list[index].is_domain_initializer_target;
}

static int IsDomainUnreachable(const int index) {
	return domain_list[index].is_domain_unreachable;
}

static int IsDeletedDomain(const int index) {
	return domain_list[index].is_domain_deleted;
}

static void ReadDomainPolicy(const char *filename) {
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
			index = FindOrAssignNewDomain(shared_buffer, 0, 0);
		} else if (index >= 0 && shared_buffer[0]) {
			AddStringEntry(shared_buffer, index);
		}
	}
	put();
	if (fp != stdin) fclose(fp);
	SortPolicy();
}

/***** sortpolicy start *****/

int sortpolicy_main(int argc, char *argv[]) {
	ReadDomainPolicy(NULL);
	WriteDomainPolicy(1);
	return 0;
}

/***** sortpolicy end *****/

/***** savepolicy start *****/

int savepolicy_main(int argc, char *argv[]) {
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
		ReadDomainPolicy("/proc/ccs/policy/domain_policy");
		for (repeat = 0; repeat < 10; repeat++) {
			//if (repeat) printf("Domain policy has changed while saving domain policy. Retrying.\n");
			if (access("domain_policy.txt", R_OK) == 0) {
				SwapDomainList();
				ReadDomainPolicy("domain_policy.txt");
				SwapDomainList();
			}
			/* Need to save domain policy? */
			if (force_save || !IsSameDomainList()) {
				int fd;
				snprintf(filename, sizeof(filename) - 1, "domain_policy.%02d-%02d-%02d.%02d:%02d:%02d.txt", tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
				if ((fd = open(filename, O_WRONLY | O_CREAT, 0600)) != EOF) {
					ftruncate(fd, 0);
					WriteDomainPolicy(fd);
					close(fd);
					unlink("domain_policy.txt");
					symlink(filename, "domain_policy.txt");
				} else {
					printf("Can't create %s\n", filename);
				}
			}
			/* Has domain policy changed while saving domain policy? */
			ClearDomainPolicy();
			ReadDomainPolicy("/proc/ccs/policy/domain_policy");
			if (IsSameDomainList()) break;
			SwapDomainList(); ClearDomainPolicy(); SwapDomainList();
		}
		ClearDomainPolicy();
		SwapDomainList(); ClearDomainPolicy(); SwapDomainList();
	}
	if (remount_root) mount("/", "/", "rootfs", MS_REMOUNT | MS_RDONLY, NULL);
	return 0;
}

/***** savepolicy end *****/

/***** loadpolicy start *****/

int loadpolicy_main(int argc, char *argv[]) {
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
		ReadDomainPolicy("domain_policy.txt");
		SwapDomainList();
		ReadDomainPolicy("/proc/ccs/policy/domain_policy");
		SwapDomainList();
		if (domain_list_count == 0) {
			fprintf(stderr, "Can't open domain_policy.txt\n");
			fclose(proc_fp);
			goto out_domain;
		}
		for (new_index = 0; new_index < domain_list_count; new_index++) {
			const char *domainname = DomainName(new_index);
			const struct path_info **new_string_ptr = domain_list[new_index].string_ptr;
			const int new_string_count = domain_list[new_index].string_count;
			int old_index;
			int i, j;
			SwapDomainList(); old_index = FindDomain(domainname, 0, 0); SwapDomainList();
			if (refresh_policy && old_index >= 0) {
				/* Old policy for this domain found. */
				const struct path_info **old_string_ptr = shadow_domain_list[old_index].string_ptr;
				const int old_string_count = shadow_domain_list[old_index].string_count;
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
			if (old_index >= 0) shadow_domain_list[old_index].domainname = &reserved; /* Don't delete this domain later. */
		}
		if (refresh_policy) {
			int old_index;
			/* Delete all domains that are not defined in new policy. */
			for (old_index = 0; old_index < shadow_domain_list_count; old_index++) {
				if (shadow_domain_list[old_index].domainname != &reserved) fprintf(proc_fp, "delete %s\n", shadow_domain_list[old_index].domainname->name);
			}
		}
		fclose(proc_fp);
	}
 out_domain: ;

	return 0;
}

/***** loadpolicy end *****/

/***** editpolicy start *****/

static const char *policy_file = DOMAIN_POLICY_FILE;
static const char *list_caption = NULL;
static char *current_domain = NULL;

static int current_screen = SCREEN_DOMAIN_LIST;

// List for generic policy.
static char **generic_acl_list = NULL;
static int generic_acl_list_count = 0;
static unsigned char *generic_acl_list_selected = NULL;

static struct domain_keeper_entry *domain_keeper_list = NULL;
static int domain_keeper_list_len = 0;
static struct domain_initializer_entry *domain_initializer_list = NULL;
static int domain_initializer_list_len = 0;

///////////////////////////  ACL HANDLER  //////////////////////////////

static const struct domain_keeper_entry *IsDomainKeeper(const struct path_info *domainname, const char *program) {
	int i;
	const struct domain_keeper_entry *flag = NULL;
	struct path_info last_name;
	if ((last_name.name = strrchr(domainname->name, ' ')) != NULL) last_name.name++;
	else last_name.name = domainname->name;
	fill_path_info(&last_name);
	for (i = 0; i < domain_keeper_list_len; i++) {
		struct domain_keeper_entry *ptr = &domain_keeper_list[i];
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

static const struct domain_initializer_entry *IsDomainInitializer(const struct path_info *domainname, const char *program) {
	int i;
	const struct domain_initializer_entry *flag = NULL;
	struct path_info last_name;
	if ((last_name.name = strrchr(domainname->name, ' ')) != NULL) last_name.name++;
	else last_name.name = domainname->name;
	fill_path_info(&last_name);
	for (i = 0; i < domain_initializer_list_len; i++) {
		struct domain_initializer_entry *ptr = &domain_initializer_list[i];
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
	struct domain_initializer_entry *ptr;
	int is_last_name = 0;
	if (!IsCorrectPath(program, 1, 0, -1)) return -EINVAL;
	if (domainname) {
		if (IsCorrectPath(domainname, 1, -1, -1)) {
			is_last_name = 1;
		} else if (!IsCorrectDomain(domainname)) {
			return -EINVAL;
		}
	}
	if ((domain_initializer_list = (struct domain_initializer_entry *) realloc(domain_initializer_list, (domain_initializer_list_len + 1) * sizeof(struct domain_initializer_entry))) == NULL) OutOfMemory();
	ptr = &domain_initializer_list[domain_initializer_list_len++];
	memset(ptr, 0, sizeof(struct domain_initializer_entry));
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
	struct domain_keeper_entry *ptr;
	int is_last_name = 0;
	if (IsCorrectPath(domainname, 1, -1, -1)) {
		is_last_name = 1;
	} else if (!IsCorrectDomain(domainname)) {
		return -EINVAL;
	}
	if (program && !IsCorrectPath(program, 1, 0, -1)) return -EINVAL;
	if ((domain_keeper_list = (struct domain_keeper_entry *) realloc(domain_keeper_list, (domain_keeper_list_len + 1) * sizeof(struct domain_keeper_entry))) == NULL) OutOfMemory();
	ptr = &domain_keeper_list[domain_keeper_list_len++];
	memset(ptr, 0, sizeof(struct domain_keeper_entry));
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
	struct group_entry *group = NULL;
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
		if ((group_list = (struct group_entry *) realloc(group_list, (group_list_len + 1) * sizeof(struct group_entry))) == NULL) OutOfMemory();
		group = &group_list[group_list_len++];
		memset(group, 0, sizeof(struct group_entry));
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

static struct group_entry *FindGroup(const char *group_name) {
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
		if (FindOrAssignNewDomain(shared_buffer, 1, 0) == EOF) OutOfMemory();
		put();
	}
}

static int domainname_attribute_compare(const void *a, const void *b) {
	const int k = strcmp(((struct domain_info *) a)->domainname->name, ((struct domain_info *) b)->domainname->name);
	if (k > 0 || (k == 0 && ((struct domain_info *) a)->is_domain_initializer_source < ((struct domain_info *) b)->is_domain_initializer_source)) return 1;
	return k;
}

static int unnumbered_domain_count = 0;

static void ReadDomainAndExceptionPolicy(void) {
	FILE *fp;
	int i, j;
	ClearDomainPolicy();
	domain_keeper_list_len = 0;
	domain_initializer_list_len = 0;
	group_list_len = 0;
	FindOrAssignNewDomain(ROOT_NAME, 0, 0);

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
				index = FindOrAssignNewDomain(shared_buffer, 0, 0);
			} else if (index >= 0 && (atoi(shared_buffer) & 1) == 1 && (cp = strchr(shared_buffer, ' ')) != NULL) {
				cp++;
				if ((cp2 = strchr(cp, ' ')) != NULL) *cp2 = '\0';
				if (*cp == '@' || IsCorrectPath(cp, 1, 0, -1)) AddStringEntry(cp, index);
			} else if (index >= 0 && sscanf(shared_buffer, "use_profile %u", &profile) == 1) {
				domain_list[index].profile = (unsigned char) profile;
			}
		}
		put();
		fclose(fp);
	}
	
	{
		int index, max_index = domain_list_count;
		
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
					if (parent.total_len == ROOT_NAME_LEN) break; /* Initializer under <kernel> is reachable. */
					domain_list[index].domain_initializer = domain_initializer;
					domain_list[index].domain_keeper = NULL;
				} else if ((domain_keeper = IsDomainKeeper(&parent, cp)) != NULL) {
					domain_list[index].domain_initializer = NULL;
					domain_list[index].domain_keeper = domain_keeper;
				}
			}
			put();
			if (domain_list[index].domain_initializer || domain_list[index].domain_keeper) domain_list[index].is_domain_unreachable = 1;
		}
		
		// Find domain initializer target domains.
		for (index = 0; index < max_index; index++) {
			char *cp;
			if ((cp = strchr(DomainName(index), ' ')) != NULL && strchr(cp + 1, ' ') == NULL) {
				for (i = 0; i < domain_initializer_list_len; i++) {
					struct domain_initializer_entry *ptr = &domain_initializer_list[i];
					if (ptr->is_not) continue;
					if (strcmp(ptr->program->name, cp + 1)) continue;
					domain_list[index].is_domain_initializer_target = 1;
				}
			}
		}

		// Find domain keeper domains.
		for (index = 0; index < max_index; index++) {
			for (i = 0; i < domain_keeper_list_len; i++) {
				struct domain_keeper_entry *ptr = &domain_keeper_list[i];
				if (ptr->is_not) continue;
				if (!ptr->is_last_name) {
					if (pathcmp(ptr->domainname, domain_list[index].domainname)) continue;
				} else {
					char *cp = strrchr(domain_list[index].domainname->name, ' ');
					if (!cp || strcmp(ptr->domainname->name, cp + 1)) continue;
				}
				domain_list[index].is_domain_keeper = 1;
			}
		}

		// Create domain initializer source domains.
		for (index = 0; index < max_index; index++) {
			const struct path_info *domainname = domain_list[index].domainname;
			const struct path_info **string_ptr = domain_list[index].string_ptr;
			const int max_count = domain_list[index].string_count;
			if (domainname->total_len == ROOT_NAME_LEN) continue; // Don't create source domain under <kernel> because they will become target domains. 
			for (i = 0; i < max_count; i++) {
				const struct path_info *cp = string_ptr[i];
				if (cp->name[0] == '@') {
					struct group_entry *group = FindGroup(cp->name + 1);
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
				if (FindDomain(shared_buffer, 0, 0) != EOF) continue;
				if (FindOrAssignNewDomain(shared_buffer, 0, 1) == EOF) OutOfMemory();
			}
			put();
		}

	}
	// Sort by domain name.
	qsort(domain_list, domain_list_count, sizeof(struct domain_info), domainname_attribute_compare);

	// Assign domain numbers.
	{
		int number = 0, index;
		unnumbered_domain_count= 0;
		for (index = 0; index < domain_list_count; index++) {
			if (IsDeletedDomain(index) || IsInitializerSource(index)) {
				domain_list[index].number = -1;
				unnumbered_domain_count++;
			} else {
				domain_list[index].number = number++;
			}
		}
	}

	domain_list_selected = realloc(domain_list_selected, domain_list_count);
	if (domain_list_count && !domain_list_selected) OutOfMemory();
	memset(domain_list_selected, 0, domain_list_count);
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
	if (current_screen == SCREEN_DOMAIN_LIST) list_item_count[SCREEN_DOMAIN_LIST] = domain_list_count;
	else list_item_count[current_screen] = generic_acl_list_count;
	clear();
	if (window_height < header_lines + 1) {
		mvprintw(0, 0, "Please resize window. This program needs at least %d lines.\n", header_lines + 1);
		refresh();
		return;
	}
	colorChange(colorHead(), ON);  // add color
	if (current_screen == SCREEN_DOMAIN_LIST) mvprintw(0, 0, "<<< Domain Transition Editor >>>      %d domain%c    '?' for help", list_item_count[SCREEN_DOMAIN_LIST] - unnumbered_domain_count, list_item_count[SCREEN_DOMAIN_LIST] - unnumbered_domain_count > 1 ? 's' : ' ');
	else mvprintw(0, 0, "<<< %s Editor >>>      %d entr%s    '?' for help", list_caption, list_item_count[current_screen], list_item_count[current_screen] > 1 ? "ies" : "y");
	colorChange(colorHead(), OFF);  // add color
	eat_col = max_eat_col[current_screen];
	max_col = 0;
	if (current_screen == SCREEN_ACL_LIST) {
		get();
		memset(shared_buffer, 0, shared_buffer_len);
		snprintf(shared_buffer, shared_buffer_len - 1, "%s", eat(current_domain));
		colorChange(colorHead(), ON);  // add color
		mvprintw(2, 0, "%s", shared_buffer);
		colorChange(colorHead(), OFF);  // add color
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
			const int number = domain_list[index].number;
			if (number >= 0) mvprintw(header_lines + i, 0, "%c%4d:%3u %c%c%c ", domain_list_selected[index] ? '&' : ' ', number, domain_list[index].profile, IsKeeperDomain(index) ? '#' : ' ', IsInitializerTarget(index) ? '*' : ' ', IsDomainUnreachable(index) ? '!' : ' ');
			else mvprintw(header_lines + i, 0, "              ");
			tmp_col += 14;
			sp = DomainName(index);
			while ((cp = strchr(sp, ' ')) != NULL) { printw("%s", eat("    ")); tmp_col += 4; sp = cp + 1; }
			if (IsDeletedDomain(index)) { printw("%s", eat("( ")); tmp_col += 2; }
			printw("%s", eat(sp)); tmp_col += strlen(sp);
			if (IsDeletedDomain(index)) { printw("%s", eat(" )")); tmp_col += 2; }
			if ((domain_initializer = domain_list[index].domain_initializer) != NULL) {
				get();
				memset(shared_buffer, 0, shared_buffer_len);
				if (domain_initializer->domainname) snprintf(shared_buffer, shared_buffer_len - 1, " ( %s%s from %s )", domain_initializer->is_oldstyle ? KEYWORD_INITIALIZER : KEYWORD_INITIALIZE_DOMAIN, domain_initializer->program->name, domain_initializer->domainname->name);
				else snprintf(shared_buffer, shared_buffer_len - 1, " ( %s%s )", domain_initializer->is_oldstyle ? KEYWORD_INITIALIZER : KEYWORD_INITIALIZE_DOMAIN, domain_initializer->program->name);
				printw("%s", eat(shared_buffer)); tmp_col += strlen(shared_buffer);
				put();
			} else if ((domain_keeper = domain_list[index].domain_keeper) != NULL) {
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
				redirect_index = FindDomain(shared_buffer, 0, 0);
				if (redirect_index >= 0) snprintf(shared_buffer, shared_buffer_len - 1, " ( -> %d )", domain_list[redirect_index].number);
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

/// add color start
#ifdef COLOR_ON
static int before_current[MAXSCREEN] = {-1, -1, -1, -1};
static int before_y[MAXSCREEN] = {-1, -1, -1, -1};

static void LineDraw(void) {
	int current = GetCurrent();
	int y, x;

	if (current == EOF) return;

	getyx(stdscr, y, x);
	if (-1 < before_current[current_screen] && current != before_current[current_screen]){
		move(header_lines + before_y[current_screen], 0);
		chgat(-1, A_NORMAL, NORMAL, NULL);
	}

	move(y, x);
	chgat(-1, A_NORMAL, colorCursor(), NULL);
	touchwin(stdscr);

	before_current[current_screen] = current;
	before_y[current_screen] = current_y[current_screen];
}
#else
#define LineDraw()
#endif
/// add color end

static void ShowCurrent(void) {
	if (current_screen == SCREEN_DOMAIN_LIST) {
		get();
		memset(shared_buffer, 0, shared_buffer_len);
		eat_col = max_eat_col[current_screen];
		snprintf(shared_buffer, shared_buffer_len - 1, "%s", eat(DomainName(GetCurrent())));
		if (window_width < shared_buffer_len) shared_buffer[window_width] = '\0';
		move(2, 0);
		clrtoeol();
		attrChange(A_REVERSE, ON);  // add color
		printw("%s", shared_buffer);
		attrChange(A_REVERSE, OFF);  // add color
		put();
	}
	move(header_lines + current_y[current_screen], 0);
	LineDraw();     // add color
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

static int SelectItem(const int current) {
	if (current >= 0) {
		int x, y;
		if (current_screen == SCREEN_DOMAIN_LIST) {
			if (IsDeletedDomain(current) || IsInitializerSource(current)) return 0;
			domain_list_selected[current] ^= 1;
		} else {
			generic_acl_list_selected[current] ^= 1;
		}
		getyx(stdscr, y, x);
		sttrSave();		// add color
		ShowList();
		sttrRestore();	// add color
		move(y, x);
		return 1;
	}
	return 0;
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

static void split_acl(char *data, struct path_info *arg1, struct path_info *arg2, struct path_info *arg3) {
	/* data = word[0] word[1] ... word[n-1] word[n] if cond[0] cond[1] ... cond[m] */
	/*                                        */
	/* arg1 = word[0]                         */
	/* arg2 = word[1] ... word[n-1] word[n]   */
	/* arg3 = if cond[0] cond[1] ... cond[m]  */
	char *cp;
	arg1->name = data;
	cp = strstr(data, " if ");
	if (cp) *cp++ = '\0';
	else cp = "";
	arg3->name = cp;
	cp = strchr(data, ' ');
	if (cp) *cp++ = '\0';
	else cp = "";
	arg2->name = cp;
	fill_path_info(arg1);
	fill_path_info(arg2);
	fill_path_info(arg3);
}

static void try_optimize(const int current) {
	char *cp;
	const char *directive;
	int directive_index, directive_len, index;
	struct path_info sarg1, sarg2, sarg3;
	struct path_info darg1, darg2, darg3;
	static const char *directive_list[30] = {
		[0]  = "1 ",
		[1]  = "2 ",
		[2]  = "3 ",
		[3]  = "4 ",
		[4]  = "5 ",
		[5]  = "6 ",
		[6]  = "7 ",
		[7]  = "allow_create ",
		[8]  = "allow_unlink ",
		[9]  = "allow_mkdir ",
		[10] = "allow_rmdir ",
		[11] = "allow_mkfifo ",
		[12] = "allow_mksock ",
		[13] = "allow_mkblock ",
		[14] = "allow_mkchar ",
		[15] = "allow_truncate ",
		[16] = "allow_symlink ",
		[17] = "allow_link ",
		[18] = "allow_rename ",
		[19] = "allow_rewrite ",
		[20] = "allow_argv0 ",
		[21] = "allow_signal ",
		[22] = "allow_network UDP bind ",
		[23] = "allow_network UDP connect ",
		[24] = "allow_network TCP bind ",
		[25] = "allow_network TCP listen ",
		[26] = "allow_network TCP connect ",
		[27] = "allow_network TCP accept ",
		[28] = "allow_network RAW bind ",
		[29] = "allow_network RAW connect ",
	};
	if (current < 0) return;
	cp = generic_acl_list[current];
	for (directive_index = 0; directive_index < 30; directive_index++) {
		if (strncmp(cp, directive_list[directive_index], strlen(directive_list[directive_index])) == 0) break;
	}
	if (directive_index == 30) return;
	cp = strdup(cp);
	if (!cp) return;
	
	directive = directive_list[directive_index];
	directive_len = strlen(directive);

	split_acl(cp + directive_len, &sarg1, &sarg2, &sarg3);
	
	get();
	for (index = 0; index < list_item_count[current_screen]; index++) {
		const char *cp = generic_acl_list[index];
		if (index == current) continue;
		if (generic_acl_list_selected[index]) continue;
		if (strncmp(cp, directive, directive_len)) continue;
		memmove(shared_buffer, cp, shared_buffer_len - 1);
		
		split_acl(shared_buffer + directive_len, &darg1, &darg2, &darg3);
	
		/* Compare condition part. */
		if (pathcmp(&sarg3, &darg3)) continue;

		/* Compare first word. */
		if (directive_index < 21) {
			if (pathcmp(&sarg1, &darg1)) {
				const int may_use_pattern = !darg1.is_patterned;
				if (darg1.name[0] == '@') continue;
				if (sarg1.name[0] == '@') {
					/* path_group component. */
					int i;
					struct group_entry *group = FindGroup(sarg1.name + 1);
					if (!group) continue;
					for (i = 0; i < group->member_name_len; i++) {
						const struct path_info *member_name = group->member_name[i];
						if (!pathcmp(member_name, &darg1)) break;
						if (may_use_pattern && PathMatchesToPattern(&darg1, member_name)) break;
					}
					if (i == group->member_name_len) continue;
				} else {
					/* Pathname component. */
					if (!may_use_pattern || !PathMatchesToPattern(&darg1, &sarg1)) continue;
				}
			}
		} else if (directive_index == 21) {
			/* Signal number component. */
			if (strcmp(sarg1.name, darg1.name)) continue;
		} else {
			if (sarg1.name[0] == '@') {
				/* IP address group component. */
			} else {
				/* IP address component. */
			}
		}

		/* Compare rest words. */
		if (directive_index == 18 || directive_index == 19) {
			if (pathcmp(&sarg2, &darg2)) {
				const int may_use_pattern = !darg2.is_patterned;
				if (darg2.name[0] == '@') continue;
				if (sarg2.name[0] == '@') {
					/* path_group component. */
					int i;
					struct group_entry *group = FindGroup(sarg2.name + 1);
					if (!group) continue;
					for (i = 0; i < group->member_name_len; i++) {
						const struct path_info *member_name = group->member_name[i];
						if (!pathcmp(member_name, &darg2)) break;
						if (may_use_pattern && PathMatchesToPattern(&darg2, member_name)) break;
					}
					if (i == group->member_name_len) continue;
				} else {
					/* Pathname component. */
					if (!may_use_pattern || !PathMatchesToPattern(&darg2, &sarg2)) continue;
				}
			}
		} else if (directive_index == 21) {
			/* Domainname component. */
			char c;
			if (strncmp(sarg2.name, darg2.name, sarg2.total_len)) continue;
			c = darg2.name[sarg2.total_len];
			if (c && c != ' ') continue;
		} else if (directive_index >= 22) {
			/* Port number component. */
			unsigned int smin, smax, dmin, dmax;
			switch (sscanf(sarg2.name, "%u-%u", &smin, &smax)) {
			case 1:
				smax = smin;
			case 2:
				break;
			default:
				continue;
			}
			switch (sscanf(darg2.name, "%u-%u", &dmin, &dmax)) {
			case 1:
				dmax = dmin;
			case 2:
				break;
			default:
				continue;
			}
			if (smin > dmin || smax < dmax) continue;
		} else {
			/* This must be empty. */
			if (sarg2.total_len || darg2.total_len) continue;
		}
		generic_acl_list_selected[index] = 1;
	}
	put();
	free(cp);
	ShowList();
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
		AdjustCursorPos(domain_list_count);
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
		if ((c == '\r' || c == '\n') && current_screen == SCREEN_ACL_LIST) return SCREEN_DOMAIN_LIST;
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
			SelectItem(current);
			break;
		case 'c':
		case 'C':
			if (current >= 0) {
				int index;
				if (current_screen == SCREEN_DOMAIN_LIST) {
					if (IsDeletedDomain(current) || IsInitializerSource(current)) break;
					for (index = current; index < domain_list_count; index++) {
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
				attrChange(A_BOLD, ON);	// add color
				line = simple_readline(window_height - 1, 0, "Search> ", readline_history, readline_history_count, 4000, 8);
				attrChange(A_BOLD, OFF);	// add color
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
				colorChange(DISP_ERR, ON);	// add color
				if (current_screen == SCREEN_DOMAIN_LIST) {
					if ((c = count(domain_list_selected, domain_list_count)) == 0 && (c = SelectItem(current)) == 0) printw("Select domain using Space key first.");
					else printw("Delete selected domain%s? ('Y'es/'N'o)", c > 1 ? "s" : "");
				} else {
					if ((c = count(generic_acl_list_selected, generic_acl_list_count)) == 0 && (c = SelectItem(current)) == 0) printw("Select entry using Space key first.");
					else printw("Delete selected entr%s? ('Y'es/'N'o)", c > 1 ? "ies" : "y");
				}
				colorChange(DISP_ERR, OFF);	// add color
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
							for (index = 1; index < domain_list_count; index++) {
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
				attrChange(A_BOLD, ON);	// add color
				char *line = simple_readline(window_height - 1, 0, "Enter new entry> ", readline_history, readline_history_count, 8192, 8);
				attrChange(A_BOLD, OFF);	// add color
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
		case '\n':
			if (current_screen == SCREEN_DOMAIN_LIST) {
				if (IsInitializerSource(current)) {
					int redirect_index;
					get();
					memset(shared_buffer, 0, shared_buffer_len);
					snprintf(shared_buffer, shared_buffer_len - 1, ROOT_NAME "%s", strrchr(DomainName(current), ' '));
					redirect_index = FindDomain(shared_buffer, 0, 0);
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
				if (!count(domain_list_selected, domain_list_count) && !SelectItem(current)) {
					mvprintw(1, 0, "Select domain using Space key first."); clrtoeol(); refresh();
				} else {
					attrChange(A_BOLD, ON);	// add color
					char *line = simple_readline(window_height - 1, 0, "Enter profile number> ", NULL, 0, 8, 1);
					attrChange(A_BOLD, OFF);	// add color
					if (line && *line) {
						FILE *fp = open_write(DOMAIN_POLICY_FILE);
						if (fp) {
							int index;
							for (index = 0; index < domain_list_count; index++) {
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
		case 'o':
		case 'O':
			try_optimize(current);
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
	FindOrAssignNewDomain(ROOT_NAME, 0, 0);
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
							index = FindDomain(shared_buffer, 0, 0);
							if (index > 0) DeleteDomain(index);
							index = EOF;
						} else if (is_select) {
							index = FindDomain(shared_buffer, 0, 0);
						} else {
							index = FindOrAssignNewDomain(shared_buffer, 0, 0);
						}
					} else if (index >= 0 && shared_buffer[0]) {
						unsigned int profile;
						if (sscanf(shared_buffer, "use_profile %u", &profile) == 1) {
							domain_list[index].profile = (unsigned char) profile;
						} else if (is_delete) {
							DelStringEntry(shared_buffer, index);
						} else {
							AddStringEntry(shared_buffer, index);
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
				for (i = 0; i < domain_list_count; i++) {
					const struct path_info **string_ptr = domain_list[i].string_ptr;
					const int string_count = domain_list[i].string_count;
					fprintf(fp, "%s\nuse_profile %u\n\n", DomainName(i), domain_list[i].profile);
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

int editpolicy_main(int argc, char *argv[]) {
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
	ColorInit();	// add color
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
