/*
 * editpolicy.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.6-pre   2008/12/16
 *
 */
#include "ccstools.h"

/* add color start */
#ifdef COLOR_ON

enum color_pair {
	NORMAL, DOMAIN_HEAD, DOMAIN_CURSOR, SYSTEM_HEAD, SYSTEM_CURSOR,
	EXCEPTION_HEAD, EXCEPTION_CURSOR, ACL_HEAD, ACL_CURSOR, DISP_ERR
};

static void editpolicy_color_init(void)
{
	static struct color_env_t {
		enum color_pair	tag;
		short int fore;
		short int back;
		const char *name;
	} color_env[] = {
		{ DOMAIN_HEAD,      COLOR_BLACK,
		  COLOR_GREEN,      "DOMAIN_HEAD" },
		{ DOMAIN_CURSOR,    COLOR_BLACK,
		  COLOR_GREEN,      "DOMAIN_CURSOR" },
		{ SYSTEM_HEAD,      COLOR_WHITE,
		  COLOR_BLUE,       "SYSTEM_HEAD" },
		{ SYSTEM_CURSOR,    COLOR_WHITE,
		  COLOR_BLUE,       "SYSTEM_CURSOR" },
		{ EXCEPTION_HEAD,   COLOR_BLACK,
		  COLOR_CYAN,       "EXCEPTION_HEAD" },
		{ EXCEPTION_CURSOR, COLOR_BLACK,
		  COLOR_CYAN,       "EXCEPTION_CURSOR" },
		{ ACL_HEAD,         COLOR_BLACK,
		  COLOR_YELLOW,     "ACL_HEAD" },
		{ ACL_CURSOR,       COLOR_BLACK,
		  COLOR_YELLOW,     "ACL_CURSOR" },
		{ NORMAL,           COLOR_WHITE,
		  COLOR_BLACK,      NULL }
	};
	FILE *fp = fopen(CCSTOOLS_CONFIG_FILE, "r");
	int i;
	if (!fp)
		goto use_default;
	get();
	while (freadline(fp)) {
		char *cp;
		if (!str_starts(shared_buffer, "editpolicy.line_color "))
			continue;
		cp = strchr(shared_buffer, '=');
		if (!cp)
			continue;
		*cp++ = '\0';
		normalize_line(shared_buffer);
		normalize_line(cp);
		if (!*shared_buffer || !*cp)
			continue;
		for (i = 0; color_env[i].name; i++) {
			short int fore;
			short int back;
			if (strcmp(shared_buffer, color_env[i].name))
				continue;
			if (strlen(cp) != 2)
				break;
			fore = (*cp++) - '0'; /* foreground color */
			back = (*cp) - '0';   /* background color */
			if (fore < 0 || fore > 7 || back < 0 || back > 7)
				break;
			color_env[i].fore = fore;
			color_env[i].back = back;
			break;
		}
	}
	put();
	fclose(fp);
use_default:
	start_color();
	for (i = 0; color_env[i].name; i++) {
		struct color_env_t *colorp = &color_env[i];
		init_pair(colorp->tag, colorp->fore, colorp->back);
	}
	init_pair(DISP_ERR, COLOR_RED, COLOR_BLACK); /* error message */
}

static void editpolicy_color_save(const _Bool flg)
{
	static attr_t save_color = NORMAL;
	if (flg)
		save_color = getattrs(stdscr);
	else
		attrset(save_color);
}

static inline void editpolicy_color_change(const attr_t attr, const _Bool flg)
{
	if (flg)
		attron(COLOR_PAIR(attr));
	else
		attroff(COLOR_PAIR(attr));
}

static inline void editpolicy_attr_change(const attr_t attr, const _Bool flg)
{
	if (flg)
		attron(attr);
	else
		attroff(attr);
}

static inline void editpolicy_sttr_save(void)
{
	editpolicy_color_save(true);
}

static inline void editpolicy_sttr_restore(void)
{
	editpolicy_color_save(false);
}

static inline int editpolicy_color_head(const int screen)
{
	if (screen == SCREEN_DOMAIN_LIST)
		return DOMAIN_HEAD;
	if (screen == SCREEN_SYSTEM_LIST)
		return SYSTEM_HEAD;
	if (screen == SCREEN_EXCEPTION_LIST)
		return EXCEPTION_HEAD;
	return ACL_HEAD;
}

static inline int editpolicy_color_cursor(const int screen)
{
	if (screen == SCREEN_DOMAIN_LIST)
		return DOMAIN_CURSOR;
	if (screen == SCREEN_SYSTEM_LIST)
		return SYSTEM_CURSOR;
	if (screen == SCREEN_EXCEPTION_LIST)
		return EXCEPTION_CURSOR;
	return ACL_CURSOR;
}

#else /* no color */

#define editpolicy_color_init()
#define editpolicy_color_change(attr, flg)
#define editpolicy_attr_change(attr, flg)
#define editpolicy_sttr_save()
#define editpolicy_sttr_restore()
#define editpolicy_color_head()
#define editpolicy_color_cursor()

#endif
/* add color end */

static FILE *open_read(const char *filename);
static FILE *open_write(const char *filename);
static _Bool offline_mode = false;

static struct path_group_entry *path_group_list = NULL;
static int path_group_list_len = 0;
static struct address_group_entry *address_group_list = NULL;
static int address_group_list_len = 0;

struct domain_policy {
	struct domain_info *list;
	int list_len;
	unsigned char *list_selected;
};

static const char *domain_name(const struct domain_policy *dp, const int index)
{
	return dp->list[index].domainname->name;
}

static const char *get_last_name(const struct domain_policy *dp,
				 const int index)
{
	const char *cp0 = domain_name(dp, index);
	const char *cp1 = strrchr(cp0, ' ');
	if (cp1)
		return cp1 + 1;
	return cp0;
}

static int add_string_entry(struct domain_policy *dp, const char *entry,
			    const int index)
{
	const struct path_info **acl_ptr;
	int acl_count;
	const struct path_info *cp;
	int i;
	if (index < 0 || index >= dp->list_len) {
		fprintf(stderr, "%s: ERROR: domain is out of range.\n",
			__func__);
		return -EINVAL;
	}
	if (!entry || !*entry)
		return -EINVAL;
	cp = savename(entry);
	if (!cp)
		out_of_memory();

	acl_ptr = dp->list[index].string_ptr;
	acl_count = dp->list[index].string_count;

	/* Check for the same entry. */
	for (i = 0; i < acl_count; i++) {
		/* Faster comparison, for they are savename'd. */
		if (cp == acl_ptr[i])
			return 0;
	}

	acl_ptr = realloc(acl_ptr, (acl_count + 1)
			  * sizeof(const struct path_info *));
	if (!acl_ptr)
		out_of_memory();
	acl_ptr[acl_count++] = cp;
	dp->list[index].string_ptr = acl_ptr;
	dp->list[index].string_count = acl_count;
	return 0;
}

static int del_string_entry(struct domain_policy *dp, const char *entry,
			    const int index)
{
	const struct path_info **acl_ptr;
	int acl_count;
	const struct path_info *cp;
	int i;
	if (index < 0 || index >= dp->list_len) {
		fprintf(stderr, "%s: ERROR: domain is out of range.\n",
			__func__);
		return -EINVAL;
	}
	if (!entry || !*entry)
		return -EINVAL;
	cp = savename(entry);
	if (!cp)
		out_of_memory();

	acl_ptr = dp->list[index].string_ptr;
	acl_count = dp->list[index].string_count;

	for (i = 0; i < acl_count; i++) {
		/* Faster comparison, for they are savename'd. */
		if (cp != acl_ptr[i])
			continue;
		dp->list[index].string_count--;
		for (; i < acl_count - 1; i++)
			acl_ptr[i] = acl_ptr[i + 1];
		return 0;
	}
	return -ENOENT;
}

static void clear_domain_policy(struct domain_policy *dp)
{
	int index;
	for (index = 0; index < dp->list_len; index++) {
		free(dp->list[index].string_ptr);
		dp->list[index].string_ptr = NULL;
		dp->list[index].string_count = 0;
	}
	free(dp->list);
	dp->list = NULL;
	dp->list_len = 0;
}

static int find_domain(struct domain_policy *dp, const char *domainname0,
		       const _Bool is_dis, const _Bool is_dd)
{
	int i;
	struct path_info domainname;
	domainname.name = domainname0;
	fill_path_info(&domainname);
	for (i = 0; i < dp->list_len; i++) {
		if (dp->list[i].is_dis == is_dis &&
		    dp->list[i].is_dd == is_dd &&
		    !pathcmp(&domainname, dp->list[i].domainname))
			return i;
	}
	return EOF;
}

static int find_or_assign_new_domain(struct domain_policy *dp,
				     const char *domainname, const _Bool is_dis,
				     const _Bool is_dd)
{
	const struct path_info *saved_domainname;
	int index = find_domain(dp, domainname, is_dis, is_dd);
	if (index >= 0)
		goto found;
	if (!is_correct_domain(domainname)) {
		fprintf(stderr, "%s: Invalid domainname '%s'\n",
			__func__, domainname);
		return EOF;
	}
	dp->list = realloc(dp->list, (dp->list_len + 1) *
			   sizeof(struct domain_info));
	if (!dp->list)
		out_of_memory();
	memset(&dp->list[dp->list_len], 0,
	       sizeof(struct domain_info));
	saved_domainname = savename(domainname);
	if (!saved_domainname)
		out_of_memory();
	dp->list[dp->list_len].domainname = saved_domainname;
	dp->list[dp->list_len].is_dis = is_dis;
	dp->list[dp->list_len].is_dd = is_dd;
	index = dp->list_len++;
found:
	return index;
}

static void delete_domain(struct domain_policy *dp, const int index)
{
	if (index >= 0 && index < dp->list_len) {
		int i;
		free(dp->list[index].string_ptr);
		for (i = index; i < dp->list_len - 1; i++)
			dp->list[i] = dp->list[i + 1];
		dp->list_len--;
	}
}

static int domainname_compare(const void *a, const void *b)
{
	return strcmp(((struct domain_info *) a)->domainname->name,
		      ((struct domain_info *) b)->domainname->name);
}

static int path_info_compare(const void *a, const void *b)
{
	const char *a0 = (*(struct path_info **) a)->name;
	const char *b0 = (*(struct path_info **) b)->name;
	return strcmp(a0, b0);
}

static void sort_domain_policy(struct domain_policy *dp)
{
	int i;
	qsort(dp->list, dp->list_len, sizeof(struct domain_info),
	      domainname_compare);
	for (i = 0; i < dp->list_len; i++)
		qsort(dp->list[i].string_ptr, dp->list[i].string_count,
		      sizeof(struct path_info *), path_info_compare);
}

static int write_domain_policy(struct domain_policy *dp, const int fd)
{
	int i;
	int j;
	for (i = 0; i < dp->list_len; i++) {
		const struct path_info **string_ptr
			= dp->list[i].string_ptr;
		const int string_count = dp->list[i].string_count;
		write(fd, dp->list[i].domainname->name,
		      dp->list[i].domainname->total_len);
		write(fd, "\n\n", 2);
		for (j = 0; j < string_count; j++) {
			write(fd, string_ptr[j]->name,
			      string_ptr[j]->total_len);
			write(fd, "\n", 1);
		}
		write(fd, "\n", 1);
	}
	return 0;
}

static _Bool is_keeper_domain(struct domain_policy *dp, const int index)
{
	return dp->list[index].is_dk;
}

static _Bool is_initializer_source(struct domain_policy *dp, const int index)
{
	return dp->list[index].is_dis;
}

static _Bool is_initializer_target(struct domain_policy *dp, const int index)
{
	return dp->list[index].is_dit;
}

static _Bool is_domain_unreachable(struct domain_policy *dp, const int index)
{
	return dp->list[index].is_du;
}

static _Bool is_deleted_domain(struct domain_policy *dp, const int index)
{
	return dp->list[index].is_dd;
}

static void handle_domain_policy(struct domain_policy *dp, FILE *fp,
				 _Bool is_write);

static void read_domain_policy(struct domain_policy *dp, const char *filename)
{
	FILE *fp = stdin;
	if (filename) {
		fp = open_read(filename);
		if (!fp) {
			fprintf(stderr, "Can't open %s\n", filename);
			return;
		}
	}
	get();
	handle_domain_policy(dp, fp, true);
	put();
	if (fp != stdin)
		fclose(fp);
	sort_domain_policy(dp);
}

/***** sortpolicy start *****/

int sortpolicy_main(int argc, char *argv[])
{
	struct domain_policy dp = { NULL, 0, NULL };
	read_domain_policy(&dp, NULL);
	write_domain_policy(&dp, 1);
	clear_domain_policy(&dp);
	return 0;
}

/***** sortpolicy end *****/

/***** diffpolicy start *****/

static int find_domain_by_ptr(struct domain_policy *dp,
			      const struct path_info *domainname)
{
	int i;
	for (i = 0; i < dp->list_len; i++) {
		if (dp->list[i].domainname == domainname)
			return i;
	}
	return EOF;
}

static _Bool save_domain_policy_with_diff(struct domain_policy *dp,
					 struct domain_policy *bp,
					 const char *proc, const char *base,
					 const char *diff)
{
	const struct path_info **proc_string_ptr;
	const struct path_info **base_string_ptr;
	int proc_string_count;
	int base_string_count;
	int proc_index;
	int base_index;
	const struct path_info *domainname;
	int i;
	int j;
	FILE *diff_fp = stdout;
	if (diff) {
		diff_fp = fopen(diff, "w");
		if (!diff_fp) {
			fprintf(stderr, "Can't open %s\n", diff);
			return false;
		}
	}
	read_domain_policy(dp, proc);
	if (!access(base, R_OK)) {
		_Bool om = offline_mode;
		offline_mode = false;
		read_domain_policy(bp, base);
		offline_mode = om;
	}

	for (base_index = 0; base_index < bp->list_len; base_index++) {
		domainname = bp->list[base_index].domainname;
		proc_index = find_domain_by_ptr(dp, domainname);
		if (proc_index >= 0)
			continue;
		/* This domain was deleted by diff policy. */
		fprintf(diff_fp, "delete %s\n\n", domainname->name);
	}

	for (proc_index = 0; proc_index < dp->list_len; proc_index++) {
		domainname = dp->list[proc_index].domainname;
		base_index = find_domain_by_ptr(bp, domainname);
		if (base_index >= 0)
			continue;
		/* This domain was added by diff policy. */
		fprintf(diff_fp, "%s\n\n", domainname->name);
		fprintf(diff_fp, KEYWORD_USE_PROFILE "%u\n",
			dp->list[proc_index].profile);
		proc_string_ptr = dp->list[proc_index].string_ptr;
		proc_string_count = dp->list[proc_index].string_count;
		for (i = 0; i < proc_string_count; i++)
			fprintf(diff_fp, "%s\n", proc_string_ptr[i]->name);
		fprintf(diff_fp, "\n");
	}

	for (proc_index = 0; proc_index < dp->list_len; proc_index++) {
		_Bool first = true;
		domainname = dp->list[proc_index].domainname;
		base_index = find_domain_by_ptr(bp, domainname);
		if (base_index == EOF)
			continue;
		/* This domain exists in both base policy and proc policy. */
		proc_string_ptr = dp->list[proc_index].string_ptr;
		proc_string_count = dp->list[proc_index].string_count;
		base_string_ptr = bp->list[base_index].string_ptr;
		base_string_count = bp->list[base_index].string_count;
		for (i = 0; i < proc_string_count; i++) {
			for (j = 0; j < base_string_count; j++) {
				if (proc_string_ptr[i] != base_string_ptr[j])
					continue;
				proc_string_ptr[i] = NULL;
				base_string_ptr[j] = NULL;
			}
		}

		for (i = 0; i < base_string_count; i++) {
			if (!base_string_ptr[i])
				continue;
			if (first)
				fprintf(diff_fp, "%s\n\n", domainname->name);
			first = false;
			fprintf(diff_fp, "delete %s\n",
				base_string_ptr[i]->name);
		}
		for (i = 0; i < proc_string_count; i++) {
			if (!proc_string_ptr[i])
				continue;
			if (first)
				fprintf(diff_fp, "%s\n\n", domainname->name);
			first = false;
			fprintf(diff_fp, "%s\n", proc_string_ptr[i]->name);
		}
		if (dp->list[proc_index].profile !=
		    bp->list[base_index].profile) {
			if (first)
				fprintf(diff_fp, "%s\n\n", domainname->name);
			first = false;
			fprintf(diff_fp, KEYWORD_USE_PROFILE "%u\n",
				dp->list[proc_index].profile);
		}
		if (!first)
			fprintf(diff_fp, "\n");
	}

	if (diff_fp != stdout)
		fclose(diff_fp);
	return true;
}

int diffpolicy_main(int argc, char *argv[])
{
	struct domain_policy dp = { NULL, 0, NULL };
	struct domain_policy bp = { NULL, 0, NULL };
	const char *original = argc > 1 ? argv[1] : proc_policy_domain_policy;
	const char *base = argc > 2 ? argv[2] : base_policy_domain_policy;
	const char *diff = argc > 3 ? argv[3] : NULL;
	if (access(original, R_OK)) {
		fprintf(stderr, "%s not found.\n", original);
		return 1;
	}
	if (base == argv[2] && access(base, R_OK)) {
		fprintf(stderr, "%s not found.\n", base);
		return 1;
	}
	return !save_domain_policy_with_diff(&dp, &bp, original, base, diff);
}

/***** diffpolicy end *****/

/***** savepolicy start *****/

static _Bool cat_file(const char *path)
{
	FILE *fp = fopen(path, "r");
	if (!fp) {
		fprintf(stderr, "Can't open %s\n", path);
		return false;
	}
	while (true) {
		int c = fgetc(fp);
		if (c == EOF)
			break;
		putchar(c);
	}
	fclose(fp);
	return true;
}

static _Bool move_proc_to_file(const char *src, const char *base,
			      const char *dest)
{
	FILE *proc_fp;
	FILE *base_fp;
	FILE *file_fp = stdout;
	char **proc_list = NULL;
	char **base_list = NULL;
	int proc_list_len = 0;
	int base_list_len = 0;
	int i;
	proc_fp = open_read(src);
	if (!proc_fp) {
		fprintf(stderr, "Can't open %s\n", src);
		return false;
	}
	if (dest) {
		file_fp = fopen(dest, "w");
		if (!file_fp) {
			fprintf(stderr, "Can't open %s\n", dest);
			fclose(proc_fp);
			return false;
		}
	}
	get();
	base_fp = fopen(base, "r");
	if (base_fp) {
		while (freadline(base_fp)) {
			char *cp;
			if (!shared_buffer[0])
				continue;
			base_list = realloc(base_list, sizeof(char *) *
					    (base_list_len + 1));
			if (!base_list)
				out_of_memory();
			cp = strdup(shared_buffer);
			if (!cp)
				out_of_memory();
			base_list[base_list_len++] = cp;
		}
		fclose(base_fp);
	}
	while (freadline(proc_fp)) {
		char *cp;
		if (!shared_buffer[0])
			continue;
		proc_list = realloc(proc_list, sizeof(char *) *
				    (proc_list_len + 1));
		if (!proc_list)
			out_of_memory();
		cp = strdup(shared_buffer);
		if (!cp)
			out_of_memory();
		proc_list[proc_list_len++] = cp;
	}
	put();
	fclose(proc_fp);

	for (i = 0; i < proc_list_len; i++) {
		int j;
		for (j = 0; j < base_list_len; j++) {
			if (!proc_list[i] || !base_list[j] ||
			    strcmp(proc_list[i], base_list[j]))
				continue;
			free(proc_list[i]);
			proc_list[i] = NULL;
			free(base_list[j]);
			base_list[j] = NULL;
			break;
		}
	}
	for (i = 0; i < base_list_len; i++) {
		if (base_list[i])
			fprintf(file_fp, "delete %s\n", base_list[i]);
	}
	for (i = 0; i < proc_list_len; i++) {
		if (proc_list[i])
			fprintf(file_fp, "%s\n", proc_list[i]);
	}

	if (file_fp != stdout)
		fclose(file_fp);
	while (proc_list_len)
		free(proc_list[--proc_list_len]);
	free(proc_list);
	while (base_list_len)
		free(base_list[--base_list_len]);
	free(base_list);
	return true;
}

static _Bool is_identical_file(const char *file1, const char *file2)
{
	char buffer1[4096];
	char buffer2[4096];
	struct stat sb1;
	struct stat sb2;
	const int fd1 = open(file1, O_RDONLY);
	const int fd2 = open(file2, O_RDONLY);
	int len1;
	int len2;
	/* Don't compare if file1 is a symlink to file2. */
	if (fstat(fd1, &sb1) || fstat(fd2, &sb2) || sb1.st_ino == sb2.st_ino)
		goto out;
	do {
		len1 = read(fd1, buffer1, sizeof(buffer1));
		len2 = read(fd2, buffer2, sizeof(buffer2));
		if (len1 < 0 || len1 != len2)
			goto out;
		if (memcmp(buffer1, buffer2, len1))
			goto out;
	} while (len1);
	close(fd1);
	close(fd2);
	return true;
out:
	close(fd1);
	close(fd2);
	return false;
}

int savepolicy_main(int argc, char *argv[])
{
	struct domain_policy dp = { NULL, 0, NULL };
	struct domain_policy bp = { NULL, 0, NULL };
	_Bool remount_root = false;
	char *filename;
	_Bool write_to_stdout = false;
	int save_profile = 0;
	int save_manager = 0;
	int save_system_policy = 0;
	int save_exception_policy = 0;
	int save_domain_policy = 0;
	_Bool force_save = false;
	time_t now = time(NULL);
	if (access("/proc/self/", F_OK))
		mount("/proc", "/proc", "proc", 0, NULL);
	if (access(proc_policy_dir, F_OK)) {
		fprintf(stderr,
			"You can't run this program for this kernel.\n");
		return 0;
	}
	if (argc == 1) {
		force_save = true;
		save_system_policy = 1;
		save_exception_policy = 1;
		save_domain_policy = 1;
	} else {
		int i;
		for (i = 1; i < argc; i++) {
			char *ptr = argv[i];
			char *s = strchr(ptr, 's');
			char *e = strchr(ptr, 'e');
			char *d = strchr(ptr, 'd');
			char *a = strchr(ptr, 'a');
			char *f = strchr(ptr, 'f');
			char *p = strchr(ptr, 'p');
			char *m = strchr(ptr, 'm');
			char *i = strchr(ptr, '-');
			if (s || a)
				save_system_policy = 1;
			if (e || a)
				save_exception_policy = 1;
			if (d || a)
				save_domain_policy = 1;
			if (p)
				save_profile = 1;
			if (m)
				save_manager = 1;
			if (f)
				force_save = true;
			if (i)
				write_to_stdout = true;
			if (strcspn(ptr, "sedafpm-"))
				goto usage;
			if (write_to_stdout && save_system_policy +
			    save_exception_policy + save_domain_policy +
			    save_profile + save_manager != 1)
				goto usage;
		}
	}
	if (chdir(disk_policy_dir)) {
		printf("Directory %s doesn't exist.\n", disk_policy_dir);
		return 1;
	}
	if (access(".", W_OK) == EOF) {
		if (errno != EROFS ||
		    mount("/", "/", "rootfs", MS_REMOUNT, NULL) == EOF) {
			printf("Can't remount for read-write. (%s)\n",
			       strerror(errno));
			return 1;
		}
		remount_root = true;
	}

	/* Exclude nonexistent policy. */
	if (access(proc_policy_system_policy, R_OK))
		save_system_policy = 0;
	if (access(proc_policy_exception_policy, R_OK))
		save_exception_policy = 0;
	if (access(proc_policy_domain_policy, R_OK))
		save_domain_policy = 0;

	if (write_to_stdout) {
		if (save_profile)
			cat_file(proc_policy_profile);
		else if (save_manager)
			cat_file(proc_policy_manager);
		else if (save_system_policy)
			cat_file(proc_policy_system_policy);
		else if (save_exception_policy)
			cat_file(proc_policy_exception_policy);
		else if (save_domain_policy)
			cat_file(proc_policy_domain_policy);
		goto done;
	}
	if (save_profile)
		move_proc_to_file(proc_policy_profile, base_policy_profile,
				  disk_policy_profile);
	if (save_manager)
		move_proc_to_file(proc_policy_manager, base_policy_manager,
				  disk_policy_manager);

	if (save_system_policy) {
		filename = make_filename("system_policy", now);
		if (move_proc_to_file(proc_policy_system_policy,
				      base_policy_system_policy, filename)
		    && !write_to_stdout) {
			if (!force_save &&
			    is_identical_file("system_policy.conf", filename)) {
				unlink(filename);
			} else {
				unlink("system_policy.conf");
				symlink(filename, "system_policy.conf");
			}
		}
	}

	if (save_exception_policy) {
		filename = make_filename("exception_policy", now);
		if (move_proc_to_file(proc_policy_exception_policy,
				      base_policy_exception_policy, filename)
		    && !write_to_stdout) {
			if (!force_save &&
			    is_identical_file("exception_policy.conf",
					      filename)) {
				unlink(filename);
			} else {
				unlink("exception_policy.conf");
				symlink(filename, "exception_policy.conf");
			}
		}
	}

	if (save_domain_policy) {
		filename = make_filename("domain_policy", now);
		if (save_domain_policy_with_diff(&dp, &bp,
						 proc_policy_domain_policy,
						 base_policy_domain_policy,
						 filename)
		    && !write_to_stdout) {
			if (!force_save &&
			    is_identical_file("domain_policy.conf", filename)) {
				unlink(filename);
			} else {
				unlink("domain_policy.conf");
				symlink(filename, "domain_policy.conf");
			}
		}
	}
done:
	if (remount_root)
		mount("/", "/", "rootfs", MS_REMOUNT | MS_RDONLY, NULL);
	return 0;
usage:
	printf("%s [s][e][d][a][f][p][m][-]\n"
	       "s : Save system_policy.\n"
	       "e : Save exception_policy.\n"
	       "d : Save domain_policy.\n"
	       "a : Save system_policy,exception_policy,domain_policy.\n"
	       "p : Save profile.\n"
	       "m : Save manager.\n"
	       "- : Write policy to stdout. "
	       "(Only one of 'sedpm' is possible when using '-'.)\n"
	       "f : Save even if on-disk policy and on-memory policy "
	       "are the same. (Valid for 'sed'.)\n\n"
	       "If no options given, this program assumes 'a' and 'f' "
	       "are given.\n", argv[0]);
	return 0;
}

/***** savepolicy end *****/

/***** loadpolicy start *****/

static void move_file_to_proc(const char *base, const char *src,
			      const char *dest)
{
	FILE *file_fp = stdin;
	FILE *base_fp;
	FILE *proc_fp = fopen(dest, "w");
	if (!proc_fp) {
		fprintf(stderr, "Can't open %s\n", dest);
		return;
	}
	if (src) {
		file_fp = fopen(src, "r");
		if (!file_fp) {
			fprintf(stderr, "Can't open %s\n", src);
			fclose(proc_fp);
			return;
		}
	}
	get();
	base_fp = fopen(base, "r");
	if (base_fp) {
		while (freadline(base_fp)) {
			if (shared_buffer[0])
				fprintf(proc_fp, "%s\n", shared_buffer);
		}
		fclose(base_fp);
	}
	while (freadline(file_fp)) {
		if (shared_buffer[0])
			fprintf(proc_fp, "%s\n", shared_buffer);
	}
	put();
	fclose(proc_fp);
	if (file_fp != stdin)
		fclose(file_fp);
}

static void delete_proc_policy(const char *name)
{
	FILE *proc_write_fp = fopen(name, "w");
	FILE *proc_read_fp = fopen(name, "r");
	if (!proc_write_fp || !proc_read_fp) {
		fprintf(stderr, "Can't open %s\n", name);
		if (proc_write_fp)
			fclose(proc_write_fp);
		if (proc_read_fp)
			fclose(proc_read_fp);
		return;
	}
	get();
	while (freadline(proc_read_fp)) {
		if (shared_buffer[0])
			fprintf(proc_write_fp, "delete %s\n", shared_buffer);
	}
	put();
	fclose(proc_read_fp);
	fclose(proc_write_fp);
}

static void update_domain_policy(struct domain_policy *dp,
				 struct domain_policy *bp, const char *base,
				 const char *src, const char *dest)
{
	int base_index;
	int proc_index;
	FILE *proc_fp = fopen(dest, "w");
	if (!proc_fp) {
		fprintf(stderr, "Can't open %s\n", dest);
		return;
	}
	/* Load base and diff policy to bp->list. */
	if (!access(base, R_OK))
		read_domain_policy(bp, base);
	read_domain_policy(bp, src);
	/* Load proc policy to dp->list. */
	read_domain_policy(dp, dest);
	for (base_index = 0; base_index < bp->list_len; base_index++) {
		int i;
		int j;
		const struct path_info *domainname
			= bp->list[base_index].domainname;
		const struct path_info **base_string_ptr
			= bp->list[base_index].string_ptr;
		const int base_string_count
			= bp->list[base_index].string_count;
		const struct path_info **proc_string_ptr;
		int proc_string_count;
		proc_index = find_domain_by_ptr(bp, domainname);
		fprintf(proc_fp, "%s\n", domainname->name);
		if (proc_index == EOF)
			goto not_found;

		/* Proc policy for this domain found. */
		proc_string_ptr = dp->list[proc_index].string_ptr;
		proc_string_count = dp->list[proc_index].string_count;
		for (j = 0; j < proc_string_count; j++) {
			for (i = 0; i < base_string_count; i++) {
				if (base_string_ptr[i] == proc_string_ptr[j])
					break;
			}
			/* Delete this entry from proc policy if not found
			   in base policy. */
			if (i == base_string_count)
				fprintf(proc_fp, "delete %s\n",
					proc_string_ptr[j]->name);
		}
		delete_domain(dp, proc_index);
not_found:
		/* Append entries defined in base policy. */
		for (i = 0; i < base_string_count; i++)
			fprintf(proc_fp, "%s\n", base_string_ptr[i]->name);
	}
	/* Delete all domains that are not defined in base policy. */
	for (proc_index = 0; proc_index < dp->list_len; proc_index++) {
		fprintf(proc_fp, "delete %s\n",
			dp->list[proc_index].domainname->name);
	}
	fclose(proc_fp);
}

int loadpolicy_main(int argc, char *argv[])
{
	struct domain_policy dp = { NULL, 0, NULL };
	struct domain_policy bp = { NULL, 0, NULL };
	_Bool read_from_stdin = false;
	int load_profile = 0;
	int load_manager = 0;
	int load_system_policy = 0;
	int load_exception_policy = 0;
	int load_domain_policy = 0;
	_Bool refresh_policy = false;
	if (access(proc_policy_dir, F_OK)) {
		fprintf(stderr,
			"You can't run this program for this kernel.\n");
		return 0;
	}
	if (argc == 1) {
		goto usage;
	} else {
		int i;
		for (i = 1; i < argc; i++) {
			char *ptr = argv[i];
			char *s = strchr(ptr, 's');
			char *e = strchr(ptr, 'e');
			char *d = strchr(ptr, 'd');
			char *a = strchr(ptr, 'a');
			char *f = strchr(ptr, 'f');
			char *p = strchr(ptr, 'p');
			char *m = strchr(ptr, 'm');
			char *i = strchr(ptr, '-');
			if (s || a)
				load_system_policy = 1;
			if (e || a)
				load_exception_policy = 1;
			if (d || a)
				load_domain_policy = 1;
			if (p)
				load_profile = 1;
			if (m)
				load_manager = 1;
			if (f)
				refresh_policy = true;
			if (i)
				read_from_stdin = true;
			if (strcspn(ptr, "sedafpm-"))
				goto usage;
			if (read_from_stdin && load_system_policy +
			    load_exception_policy + load_domain_policy +
			    load_profile + load_manager != 1)
				goto usage;
		}
	}
	if (chdir(disk_policy_dir)) {
		printf("Directory %s doesn't exist.\n", disk_policy_dir);
		return 1;
	}

	if (load_profile) {
		if (read_from_stdin)
			move_file_to_proc(NULL, NULL, proc_policy_profile);
		else
			move_file_to_proc(base_policy_profile,
					  disk_policy_profile,
					  proc_policy_profile);
	}

	if (load_manager) {
		if (read_from_stdin)
			move_file_to_proc(NULL, NULL, proc_policy_manager);
		else
			move_file_to_proc(base_policy_manager,
					  disk_policy_manager,
					  proc_policy_manager);
	}

	if (load_system_policy) {
		if (refresh_policy)
			delete_proc_policy(proc_policy_system_policy);
		if (read_from_stdin)
			move_file_to_proc(NULL, NULL,
					  proc_policy_system_policy);
		else
			move_file_to_proc(base_policy_system_policy,
					  disk_policy_system_policy,
					  proc_policy_system_policy);
	}

	if (load_exception_policy) {
		if (refresh_policy)
			delete_proc_policy(proc_policy_exception_policy);
		if (read_from_stdin)
			move_file_to_proc(NULL, NULL,
					  proc_policy_exception_policy);
		else
			move_file_to_proc(base_policy_exception_policy,
					  disk_policy_exception_policy,
					  proc_policy_exception_policy);
	}

	if (load_domain_policy) {
		if (refresh_policy) {
			if (read_from_stdin)
				update_domain_policy(&dp, &bp, NULL, NULL,
						     proc_policy_domain_policy);
			else
				update_domain_policy(&dp, &bp,
						     base_policy_domain_policy,
						     disk_policy_domain_policy,
						     proc_policy_domain_policy);
		} else {
			if (read_from_stdin)
				move_file_to_proc(NULL, NULL,
						  proc_policy_domain_policy);
			else
				move_file_to_proc(base_policy_domain_policy,
						  disk_policy_domain_policy,
						  proc_policy_domain_policy);
		}
	}
	return 0;
usage:
	printf("%s [s][e][d][a][f][p][m][-]\n"
	       "s : Load system_policy.\n"
	       "e : Load exception_policy.\n"
	       "d : Load domain_policy.\n"
	       "a : Load system_policy,exception_policy,domain_policy.\n"
	       "p : Load profile.\n"
	       "m : Load manager.\n"
	       "- : Read policy from stdin. "
	       "(Only one of 'sedpm' is possible when using '-'.)\n"
	       "f : Delete on-memory policy before loading on-disk policy. "
	       "(Valid for 'sed'.)\n\n", argv[0]);
	return 0;
}

/***** loadpolicy end *****/

/***** editpolicy start *****/

#define DIRECTIVE_NONE                              0
#define DIRECTIVE_1                                 1
#define DIRECTIVE_2                                 2
#define DIRECTIVE_3                                 3
#define DIRECTIVE_4                                 4
#define DIRECTIVE_5                                 5
#define DIRECTIVE_6                                 6
#define DIRECTIVE_7                                 7
#define DIRECTIVE_ALLOW_EXECUTE                     8
#define DIRECTIVE_ALLOW_READ                        9
#define DIRECTIVE_ALLOW_WRITE                      10
#define DIRECTIVE_ALLOW_READ_WRITE                 11
#define DIRECTIVE_ALLOW_CREATE                     12
#define DIRECTIVE_ALLOW_UNLINK                     13
#define DIRECTIVE_ALLOW_MKDIR                      14
#define DIRECTIVE_ALLOW_RMDIR                      15
#define DIRECTIVE_ALLOW_MKFIFO                     16
#define DIRECTIVE_ALLOW_MKSOCK                     17
#define DIRECTIVE_ALLOW_MKBLOCK                    18
#define DIRECTIVE_ALLOW_MKCHAR                     19
#define DIRECTIVE_ALLOW_TRUNCATE                   20
#define DIRECTIVE_ALLOW_SYMLINK                    21
#define DIRECTIVE_ALLOW_LINK                       22
#define DIRECTIVE_ALLOW_RENAME                     23
#define DIRECTIVE_ALLOW_REWRITE                    24
#define DIRECTIVE_ALLOW_ARGV0                      25
#define DIRECTIVE_ALLOW_SIGNAL                     26
#define DIRECTIVE_ALLOW_NETWORK                    27
#define DIRECTIVE_ALLOW_ENV                        28
#define DIRECTIVE_ADDRESS_GROUP                    29
#define DIRECTIVE_AGGREGATOR                       30
#define DIRECTIVE_ALIAS                            31
#define DIRECTIVE_ALLOW_CAPABILITY                 32
#define DIRECTIVE_ALLOW_CHROOT                     33
#define DIRECTIVE_ALLOW_MOUNT                      34
#define DIRECTIVE_ALLOW_PIVOT_ROOT                 35
#define DIRECTIVE_DENY_AUTOBIND                    36
#define DIRECTIVE_DENY_REWRITE                     37
#define DIRECTIVE_DENY_UNMOUNT                     38
#define DIRECTIVE_FILE_PATTERN                     39
#define DIRECTIVE_EXECUTE_HANDLER                  40
#define DIRECTIVE_DENIED_EXECUTE_HANDLER           41
#define DIRECTIVE_IGNORE_GLOBAL_ALLOW_ENV          42
#define DIRECTIVE_IGNORE_GLOBAL_ALLOW_READ         43
#define DIRECTIVE_INITIALIZE_DOMAIN                44
#define DIRECTIVE_KEEP_DOMAIN                      45
#define DIRECTIVE_NO_INITIALIZE_DOMAIN             46
#define DIRECTIVE_NO_KEEP_DOMAIN                   47
#define DIRECTIVE_PATH_GROUP                       48
#define DIRECTIVE_QUOTA_EXCEEDED                   49
#define DIRECTIVE_USE_PROFILE                      50
#define MAX_DIRECTIVE_INDEX                        51

static struct {
	const char *original;
	const char *alias;
	int original_len;
	int alias_len;
} directives[MAX_DIRECTIVE_INDEX] = {
	[DIRECTIVE_NONE] = { "", NULL, 0, 0 },
	[DIRECTIVE_1]  = { "1", NULL, 0, 0 },
	[DIRECTIVE_2]  = { "2", NULL, 0, 0 },
	[DIRECTIVE_3]  = { "3", NULL, 0, 0 },
	[DIRECTIVE_4]  = { "4", NULL, 0, 0 },
	[DIRECTIVE_5]  = { "5", NULL, 0, 0 },
	[DIRECTIVE_6]  = { "6", NULL, 0, 0 },
	[DIRECTIVE_7]  = { "7", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_EXECUTE]    = { "allow_execute", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_READ]       = { "allow_read", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_WRITE]      = { "allow_write", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_READ_WRITE] = { "allow_read/write", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_CREATE]     = { "allow_create", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_UNLINK]     = { "allow_unlink", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_MKDIR]      = { "allow_mkdir", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_RMDIR]      = { "allow_rmdir", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_MKFIFO]     = { "allow_mkfifo", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_MKSOCK]     = { "allow_mksock", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_MKBLOCK]    = { "allow_mkblock", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_MKCHAR]     = { "allow_mkchar", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_TRUNCATE]   = { "allow_truncate", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_SYMLINK]    = { "allow_symlink", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_LINK]       = { "allow_link", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_RENAME]     = { "allow_rename", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_REWRITE]    = { "allow_rewrite", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_ARGV0]      = { "allow_argv0", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_SIGNAL]     = { "allow_signal", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_NETWORK]    = { "allow_network", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_ENV]        = { "allow_env", NULL, 0, 0 },
	[DIRECTIVE_ADDRESS_GROUP]    = { "address_group", NULL, 0, 0 },
	[DIRECTIVE_AGGREGATOR]       = { "aggregator", NULL, 0, 0 },
	[DIRECTIVE_ALIAS]            = { "alias", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_CAPABILITY] = { "allow_capability", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_CHROOT]     = { "allow_chroot", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_MOUNT]      = { "allow_mount", NULL, 0, 0 },
	[DIRECTIVE_ALLOW_PIVOT_ROOT] = { "allow_pivot_root", NULL, 0, 0 },
	[DIRECTIVE_DENY_AUTOBIND]    = { "deny_autobind", NULL, 0, 0 },
	[DIRECTIVE_DENY_REWRITE]     = { "deny_rewrite", NULL, 0, 0 },
	[DIRECTIVE_DENY_UNMOUNT]     = { "deny_unmount", NULL, 0, 0 },
	[DIRECTIVE_FILE_PATTERN]     = { "file_pattern", NULL, 0, 0 },
	[DIRECTIVE_EXECUTE_HANDLER]  = { "execute_handler", NULL, 0, 0 },
	[DIRECTIVE_DENIED_EXECUTE_HANDLER] = {
		"denied_execute_handler", NULL, 0, 0 },
	[DIRECTIVE_IGNORE_GLOBAL_ALLOW_ENV] = {
		"ignore_global_allow_env", NULL, 0, 0 },
	[DIRECTIVE_IGNORE_GLOBAL_ALLOW_READ] = {
		"ignore_global_allow_read", NULL, 0, 0 },
	[DIRECTIVE_INITIALIZE_DOMAIN]    = { "initialize_domain", NULL, 0, 0 },
	[DIRECTIVE_KEEP_DOMAIN]          = { "keep_domain", NULL, 0, 0 },
	[DIRECTIVE_NO_INITIALIZE_DOMAIN] = {
		"no_initialize_domain", NULL, 0, 0 },
	[DIRECTIVE_NO_KEEP_DOMAIN]       = { "no_keep_domain", NULL, 0, 0 },
	[DIRECTIVE_PATH_GROUP]       = { "path_group", NULL, 0, 0 },
	[DIRECTIVE_QUOTA_EXCEEDED]   = { "quota_exceeded", NULL, 0, 0 },
	[DIRECTIVE_USE_PROFILE]      = { "use_profile", NULL, 0, 0 },
};

static const char *policy_file = NULL;
static const char *list_caption = NULL;
static char *current_domain = NULL;

static int current_screen = SCREEN_DOMAIN_LIST;

/* List for generic policy. */
static struct generic_acl {
	u8 directive;
	u8 selected;
	const char *operand;
} *generic_acl_list = NULL;
static int generic_acl_list_count = 0;

static struct domain_keeper_entry *domain_keeper_list = NULL;
static int domain_keeper_list_len = 0;
static struct domain_initializer_entry *domain_initializer_list = NULL;
static int domain_initializer_list_len = 0;

/* ACL HANDLER  */

static const struct domain_keeper_entry *
is_domain_keeper(const struct path_info *domainname, const char *program)
{
	int i;
	const struct domain_keeper_entry *flag = NULL;
	struct path_info last_name;
	last_name.name = strrchr(domainname->name, ' ');
	if (last_name.name)
		last_name.name++;
	else
		last_name.name = domainname->name;
	fill_path_info(&last_name);
	for (i = 0; i < domain_keeper_list_len; i++) {
		struct domain_keeper_entry *ptr = &domain_keeper_list[i];
		if (!ptr->is_last_name) {
			if (pathcmp(ptr->domainname, domainname))
				continue;
		} else {
			if (pathcmp(ptr->domainname, &last_name))
				continue;
		}
		if (ptr->program && strcmp(ptr->program->name, program))
			continue;
		if (ptr->is_not)
			return NULL;
		flag = ptr;
	}
	return flag;
}

static const struct domain_initializer_entry *
is_domain_initializer(const struct path_info *domainname, const char *program)
{
	int i;
	const struct domain_initializer_entry *flag = NULL;
	struct path_info last_name;
	last_name.name = strrchr(domainname->name, ' ');
	if (last_name.name)
		last_name.name++;
	else
		last_name.name = domainname->name;
	fill_path_info(&last_name);
	for (i = 0; i < domain_initializer_list_len; i++) {
		struct domain_initializer_entry *ptr
			= &domain_initializer_list[i];
		if (ptr->domainname) {
			if (!ptr->is_last_name) {
				if (pathcmp(ptr->domainname, domainname))
					continue;
			} else {
				if (pathcmp(ptr->domainname, &last_name))
					continue;
			}
		}
		if (strcmp(ptr->program->name, program))
			continue;
		if (ptr->is_not)
			return NULL;
		flag = ptr;
	}
	return flag;
}

/* UTILITY FUNCTIONS */

static int persistent_fd = EOF;

static void send_fd(char *data, int *fd)
{
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
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	msg.msg_controllen = cmsg->cmsg_len;
	memmove(CMSG_DATA(cmsg), fd, sizeof(int));
	sendmsg(persistent_fd, &msg, 0);
	close(*fd);
}

static FILE *open_read(const char *filename)
{
	if (offline_mode) {
		char request[1024];
		int fd[2];
		FILE *fp;
		if (socketpair(PF_UNIX, SOCK_STREAM, 0, fd)) {
			fprintf(stderr, "socketpair()\n");
			exit(1);
		}
		if (shutdown(fd[0], SHUT_WR))
			goto out;
		fp = fdopen(fd[0], "r");
		if (!fp)
			goto out;
		memset(request, 0, sizeof(request));
		snprintf(request, sizeof(request) - 1, "GET %s", filename);
		send_fd(request, &fd[1]);
		return fp;
out:
		close(fd[1]);
		close(fd[0]);
		exit(1);
	} else {
		return fopen(filename, "r");
	}
}

static FILE *open_write(const char *filename)
{
	if (offline_mode) {
		char request[1024];
		int fd[2];
		if (socketpair(PF_UNIX, SOCK_STREAM, 0, fd)) {
			fprintf(stderr, "socketpair()\n");
			exit(1);
		}
		if (shutdown(fd[0], SHUT_RD))
			goto out;
		memset(request, 0, sizeof(request));
		snprintf(request, sizeof(request) - 1, "POST %s", filename);
		send_fd(request, &fd[1]);
		return fdopen(fd[0], "w");
out:
		close(fd[1]);
		close(fd[0]);
		exit(1);
	} else {
		return fdopen(open(filename, O_WRONLY), "w");
	}
}

static u8 find_directive(const _Bool forward, char *line);
static int generic_acl_compare(const void *a, const void *b);

static int generic_acl_compare0(const void *a, const void *b)
{
	const struct generic_acl *a0 = (struct generic_acl *) a;
	const struct generic_acl *b0 = (struct generic_acl *) b;
	const char *a1 = directives[a0->directive].alias;
	const char *b1 = directives[b0->directive].alias;
	const char *a2 = a0->operand;
	const char *b2 = b0->operand;
	const int ret = strcmp(a1, b1);
	if (ret)
		return ret;
	return strcmp(a2, b2);
}

static int string_acl_compare(const void *a, const void *b)
{
	const struct generic_acl *a0 = (struct generic_acl *) a;
	const struct generic_acl *b0 = (struct generic_acl *) b;
	const char *a2 = a0->operand;
	const char *b2 = b0->operand;
	return strcmp(a2, b2);
}

static void read_generic_policy(void)
{
	FILE *fp = NULL;
	_Bool flag = false;
	while (generic_acl_list_count)
		free((void *)
		     generic_acl_list[--generic_acl_list_count].operand);
	if (!offline_mode && current_screen == SCREEN_ACL_LIST) {
		fp = fopen(policy_file, "r+");
		if (fp) {
			fprintf(fp, "select domain=%s\n", current_domain);
			fflush(fp);
		}
	}
	if (!fp)
		fp = open_read(policy_file);
	if (!fp)
		return;
	get();
	while (freadline(fp)) {
		u8 directive;
		char *cp;
		if (current_screen == SCREEN_ACL_LIST) {
			if (is_domain_def(shared_buffer)) {
				flag = !strcmp(shared_buffer, current_domain);
				continue;
			}
			if (!flag || !shared_buffer[0] ||
			    !strncmp(shared_buffer, KEYWORD_USE_PROFILE,
				     KEYWORD_USE_PROFILE_LEN))
				continue;
		} else {
			if (!shared_buffer[0])
				continue;
		}
		switch (current_screen) {
		case SCREEN_SYSTEM_LIST:
		case SCREEN_EXCEPTION_LIST:
		case SCREEN_ACL_LIST:
			directive = find_directive(true, shared_buffer);
			if (directive == DIRECTIVE_NONE)
				continue;
			break;
		default:
			directive = DIRECTIVE_NONE;
			break;
		}
		generic_acl_list = realloc(generic_acl_list,
					   (generic_acl_list_count + 1) *
					   sizeof(struct generic_acl));
		if (!generic_acl_list)
			out_of_memory();
		cp = strdup(shared_buffer);
		if (!cp)
			out_of_memory();
		generic_acl_list[generic_acl_list_count].directive = directive;
		generic_acl_list[generic_acl_list_count].selected = 0;
		generic_acl_list[generic_acl_list_count++].operand = cp;
	}
	put();
	fclose(fp);
	switch (current_screen) {
	case SCREEN_ACL_LIST:
		qsort(generic_acl_list, generic_acl_list_count,
		      sizeof(struct generic_acl), generic_acl_compare);
		break;
	case SCREEN_SYSTEM_LIST:
	case SCREEN_EXCEPTION_LIST:
		qsort(generic_acl_list, generic_acl_list_count,
		      sizeof(struct generic_acl), generic_acl_compare0);
		break;
	default:
		qsort(generic_acl_list, generic_acl_list_count,
		      sizeof(struct generic_acl), string_acl_compare);
	}
}

static int add_domain_initializer_entry(const char *domainname,
					const char *program, const _Bool is_not)
{
	void *vp;
	struct domain_initializer_entry *ptr;
	_Bool is_last_name = false;
	if (!is_correct_path(program, 1, 0, -1))
		return -EINVAL;
	if (domainname) {
		if (is_correct_path(domainname, 1, -1, -1))
			is_last_name = true;
		else if (!is_correct_domain(domainname))
			return -EINVAL;
	}
	vp = realloc(domain_initializer_list,
		     (domain_initializer_list_len + 1) *
		     sizeof(struct domain_initializer_entry));
	if (!vp)
		out_of_memory();
	domain_initializer_list = vp;
	ptr = &domain_initializer_list[domain_initializer_list_len++];
	memset(ptr, 0, sizeof(struct domain_initializer_entry));
	ptr->program = savename(program);
	if (!ptr->program)
		out_of_memory();
	if (domainname) {
		ptr->domainname = savename(domainname);
		if (!ptr->domainname)
			out_of_memory();
	}
	ptr->is_not = is_not;
	ptr->is_last_name = is_last_name;
	return 0;
}

static int add_domain_initializer_policy(char *data, const _Bool is_not)
{
	char *cp = strstr(data, " from ");
	if (cp) {
		*cp = '\0';
		return add_domain_initializer_entry(cp + 6, data, is_not);
	} else {
		return add_domain_initializer_entry(NULL, data, is_not);
	}
}

static int add_domain_keeper_entry(const char *domainname, const char *program,
				   const _Bool is_not)
{
	struct domain_keeper_entry *ptr;
	_Bool is_last_name = false;
	if (is_correct_path(domainname, 1, -1, -1))
		is_last_name = true;
	else if (!is_correct_domain(domainname))
		return -EINVAL;
	if (program && !is_correct_path(program, 1, 0, -1))
		return -EINVAL;
	domain_keeper_list = realloc(domain_keeper_list,
				     (domain_keeper_list_len + 1) *
				     sizeof(struct domain_keeper_entry));
	if (!domain_keeper_list)
		out_of_memory();
	ptr = &domain_keeper_list[domain_keeper_list_len++];
	memset(ptr, 0, sizeof(struct domain_keeper_entry));
	ptr->domainname = savename(domainname);
	if (!ptr->domainname)
		out_of_memory();
	if (program) {
		ptr->program = savename(program);
		if (!ptr->program)
			out_of_memory();
	}
	ptr->is_not = is_not;
	ptr->is_last_name = is_last_name;
	return 0;
}

static int add_domain_keeper_policy(char *data, const _Bool is_not)
{
	char *cp = strstr(data, " from ");
	if (cp) {
		*cp = '\0';
		return add_domain_keeper_entry(cp + 6, data, is_not);
	} else {
		return add_domain_keeper_entry(data, NULL, is_not);
	}
}

static int add_path_group_entry(const char *group_name, const char *member_name,
				const _Bool is_delete)
{
	const struct path_info *saved_group_name;
	const struct path_info *saved_member_name;
	int i;
	int j;
	struct path_group_entry *group = NULL;
	if (!is_correct_path(group_name, 0, 0, 0) ||
	    !is_correct_path(member_name, 0, 0, 0))
		return -EINVAL;
	saved_group_name = savename(group_name);
	saved_member_name = savename(member_name);
	if (!saved_group_name || !saved_member_name)
		return -ENOMEM;
	for (i = 0; i < path_group_list_len; i++) {
		group = &path_group_list[i];
		if (saved_group_name != group->group_name)
			continue;
		for (j = 0; j < group->member_name_len; j++) {
			if (group->member_name[j] != saved_member_name)
				continue;
			if (!is_delete)
				return 0;
			while (j < group->member_name_len - 1)
				group->member_name[j] =
					group->member_name[j + 1];
			group->member_name_len--;
			return 0;
		}
		break;
	}
	if (is_delete)
		return -ENOENT;
	if (i == path_group_list_len) {
		path_group_list = realloc(path_group_list,
					  (path_group_list_len + 1) *
					  sizeof(struct path_group_entry));
		if (!path_group_list)
			out_of_memory();
		group = &path_group_list[path_group_list_len++];
		memset(group, 0, sizeof(struct path_group_entry));
		group->group_name = saved_group_name;
	}
	group->member_name = realloc(group->member_name,
				     (group->member_name_len + 1)
				     * sizeof(const struct path_info *));
	if (!group->member_name)
		out_of_memory();
	group->member_name[group->member_name_len++] = saved_member_name;
	return 0;
}

static int add_path_group_policy(char *data, const _Bool is_delete)
{
	char *cp = strchr(data, ' ');
	if (!cp)
		return -EINVAL;
	*cp++ = '\0';
	return add_path_group_entry(data, cp, is_delete);
}

static struct path_group_entry *find_path_group(const char *group_name)
{
	int i;
	for (i = 0; i < path_group_list_len; i++) {
		if (!strcmp(group_name, path_group_list[i].group_name->name))
			return &path_group_list[i];
	}
	return NULL;
}

static int parse_ip(const char *address, struct ip_address_entry *entry)
{
	unsigned int min[8];
	unsigned int max[8];
	int i;
	int j;
	memset(entry, 0, sizeof(*entry));
	i = sscanf(address, "%u.%u.%u.%u-%u.%u.%u.%u",
		   &min[0], &min[1], &min[2], &min[3],
		   &max[0], &max[1], &max[2], &max[3]);
	if (i == 4)
		for (j = 0; j < 4; j++)
			max[j] = min[j];
	if (i == 4 || i == 8) {
		for (j = 0; j < 4; j++) {
			entry->min[j] = (u8) min[j];
			entry->max[j] = (u8) max[j];
		}
		return 0;
	}
	i = sscanf(address, "%X:%X:%X:%X:%X:%X:%X:%X-%X:%X:%X:%X:%X:%X:%X:%X",
		   &min[0], &min[1], &min[2], &min[3],
		   &min[4], &min[5], &min[6], &min[7],
		   &max[0], &max[1], &max[2], &max[3],
		   &max[4], &max[5], &max[6], &max[7]);
	if (i == 8)
		for (j = 0; j < 8; j++)
			max[j] = min[j];
	if (i == 8 || i == 16) {
		for (j = 0; j < 8; j++) {
			entry->min[j * 2] = (u8) (min[j] >> 8);
			entry->min[j * 2 + 1] = (u8) min[j];
			entry->max[j * 2] = (u8) (max[j] >> 8);
			entry->max[j * 2 + 1] = (u8) max[j];
		}
		entry->is_ipv6 = true;
		return 0;
	}
	return -EINVAL;
}

static int add_address_group_entry(const char *group_name,
				   const char *member_name,
				   const _Bool is_delete)
{
	const struct path_info *saved_group_name;
	int i;
	int j;
	struct ip_address_entry entry;
	struct address_group_entry *group = NULL;
	if (parse_ip(member_name, &entry))
		return -EINVAL;
	if (!is_correct_path(group_name, 0, 0, 0))
		return -EINVAL;
	saved_group_name = savename(group_name);
	if (!saved_group_name)
		return -ENOMEM;
	for (i = 0; i < address_group_list_len; i++) {
		group = &address_group_list[i];
		if (saved_group_name != group->group_name)
			continue;
		for (j = 0; j < group->member_name_len; j++) {
			if (memcmp(&group->member_name[j], &entry,
				   sizeof(entry)))
				continue;
			if (!is_delete)
				return 0;
			while (j < group->member_name_len - 1)
				group->member_name[j]
					= group->member_name[j + 1];
			group->member_name_len--;
			return 0;
		}
		break;
	}
	if (is_delete)
		return -ENOENT;
	if (i == address_group_list_len) {
		void *vp;
		vp = realloc(address_group_list,
			     (address_group_list_len + 1) *
			     sizeof(struct address_group_entry));
		if (!vp)
			out_of_memory();
		address_group_list = vp;
		group = &address_group_list[address_group_list_len++];
		memset(group, 0, sizeof(struct address_group_entry));
		group->group_name = saved_group_name;
	}
	group->member_name = realloc(group->member_name,
				     (group->member_name_len + 1) *
				     sizeof(const struct ip_address_entry));
	if (!group->member_name)
		out_of_memory();
	group->member_name[group->member_name_len++] = entry;
	return 0;
}

static int add_address_group_policy(char *data, const _Bool is_delete)
{
	char *cp = strchr(data, ' ');
	if (!cp)
		return -EINVAL;
	*cp++ = '\0';
	return add_address_group_entry(data, cp, is_delete);
}

static struct address_group_entry *find_address_group(const char *group_name)
{
	int i;
	for (i = 0; i < address_group_list_len; i++) {
		if (!strcmp(group_name, address_group_list[i].group_name->name))
			return &address_group_list[i];
	}
	return NULL;
}

static void assign_domain_initializer_source(struct domain_policy *dp,
					     const struct path_info *domainname,
					     const char *program)
{
	if (is_domain_initializer(domainname, program)) {
		get();
		shprintf("%s %s", domainname->name, program);
		normalize_line(shared_buffer);
		if (find_or_assign_new_domain(dp, shared_buffer, true, false)
		    == EOF)
			out_of_memory();
		put();
	}
}

static int domainname_attribute_compare(const void *a, const void *b)
{
	const struct domain_info *a0 = a;
	const struct domain_info *b0 = b;
	const int k = strcmp(a0->domainname->name, b0->domainname->name);
	if ((k > 0) || (!k && !a0->is_dis && b0->is_dis))
		return 1;
	return k;
}

static int unnumbered_domain_count = 0;

static void read_domain_and_exception_policy(struct domain_policy *dp)
{
	FILE *fp;
	int i;
	int j;
	int index;
	int max_index;
	clear_domain_policy(dp);
	domain_keeper_list_len = 0;
	domain_initializer_list_len = 0;
	while (path_group_list_len)
		free(path_group_list[--path_group_list_len].member_name);
	/*
	while (address_group_list_len)
		free(address_group_list[--address_group_list_len].member_name);
	*/
	address_group_list_len = 0;
	find_or_assign_new_domain(dp, ROOT_NAME, false, false);

	/* Load domain_initializer list, domain_keeper list. */
	fp = open_read(proc_policy_exception_policy);
	if (!fp)
		goto no_exception;
	get();
	while (freadline(fp)) {
		if (str_starts(shared_buffer, KEYWORD_INITIALIZE_DOMAIN))
			add_domain_initializer_policy(shared_buffer, false);
		else if (str_starts(shared_buffer,
				    KEYWORD_NO_INITIALIZE_DOMAIN))
			add_domain_initializer_policy(shared_buffer, true);
		else if (str_starts(shared_buffer, KEYWORD_KEEP_DOMAIN))
			add_domain_keeper_policy(shared_buffer, false);
		else if (str_starts(shared_buffer, KEYWORD_NO_KEEP_DOMAIN))
			add_domain_keeper_policy(shared_buffer, true);
		else if (str_starts(shared_buffer, KEYWORD_PATH_GROUP))
			add_path_group_policy(shared_buffer, false);
		else if (str_starts(shared_buffer, KEYWORD_ADDRESS_GROUP))
			add_address_group_policy(shared_buffer, false);
	}
	put();
	fclose(fp);
no_exception:

	/* Load all domain list. */
	fp = NULL;
	if (!offline_mode) {
		fp = fopen(policy_file, "r+");
		if (fp) {
			fprintf(fp, "select allow_execute\n");
			fflush(fp);
		}
	}
	if (!fp)
		fp = open_read(proc_policy_domain_policy);
	if (!fp)
		goto no_domain;
	index = EOF;
	get();
	while (freadline(fp)) {
		char *cp;
		char *cp2;
		unsigned int profile;
		if (is_domain_def(shared_buffer)) {
			index = find_or_assign_new_domain(dp, shared_buffer,
							  false, false);
			continue;
		} else if (index == EOF) {
			continue;
		}
		if (str_starts(shared_buffer, KEYWORD_EXECUTE_HANDLER)) {
			add_string_entry(dp, shared_buffer, index);
		} else if (str_starts(shared_buffer,
				      KEYWORD_DENIED_EXECUTE_HANDLER)) {
			add_string_entry(dp, shared_buffer, index);
		} else if (str_starts(shared_buffer, KEYWORD_ALLOW_EXECUTE) ||
			   str_starts(shared_buffer, "1 ") ||
			   str_starts(shared_buffer, "3 ") ||
			   str_starts(shared_buffer, "5 ") ||
			   str_starts(shared_buffer, "7 ")) {
			cp = shared_buffer;
			cp2 = strchr(cp, ' ');
			if (cp2)
				*cp2 = '\0';
			if (*cp == '@' || is_correct_path(cp, 1, 0, -1))
				add_string_entry(dp, cp, index);
		} else if (sscanf(shared_buffer,
				  KEYWORD_USE_PROFILE "%u", &profile) == 1) {
			dp->list[index].profile = (u8) profile;
		}
	}
	put();
	fclose(fp);
no_domain:

	max_index = dp->list_len;

	/* Find unreachable domains. */
	for (index = 0; index < max_index; index++) {
		get();
		shprintf("%s", domain_name(dp, index));
		while (true) {
			const struct domain_initializer_entry *d_i;
			const struct domain_keeper_entry *d_k;
			struct path_info parent;
			char *cp = strrchr(shared_buffer, ' ');
			if (!cp)
				break;
			*cp++ = '\0';
			parent.name = shared_buffer;
			fill_path_info(&parent);
			d_i = is_domain_initializer(&parent, cp);
			if (d_i) {
				/* Initializer under <kernel> is reachable. */
				if (parent.total_len == ROOT_NAME_LEN)
					break;
				dp->list[index].d_i = d_i;
				dp->list[index].d_k = NULL;
				continue;
			}
			d_k = is_domain_keeper(&parent, cp);
			if (d_k) {
				dp->list[index].d_i = NULL;
				dp->list[index].d_k = d_k;
			}
		}
		put();
		if (dp->list[index].d_i || dp->list[index].d_k)
			dp->list[index].is_du = true;
	}

	/* Find domain initializer target domains. */
	for (index = 0; index < max_index; index++) {
		char *cp = strchr(domain_name(dp, index), ' ');
		if (!cp || strchr(cp + 1, ' '))
			continue;
		for (i = 0; i < domain_initializer_list_len; i++) {
			struct domain_initializer_entry *ptr
				= &domain_initializer_list[i];
			if (ptr->is_not)
				continue;
			if (strcmp(ptr->program->name, cp + 1))
				continue;
			dp->list[index].is_dit = true;
		}
	}

	/* Find domain keeper domains. */
	for (index = 0; index < max_index; index++) {
		for (i = 0; i < domain_keeper_list_len; i++) {
			struct domain_keeper_entry *ptr
				= &domain_keeper_list[i];
			char *cp;
			if (ptr->is_not)
				continue;
			if (!ptr->is_last_name) {
				if (pathcmp(ptr->domainname,
					    dp->list[index].domainname))
					continue;
				dp->list[index].is_dk = true;
				continue;
			}
			cp = strrchr(dp->list[index].domainname->name,
				     ' ');
			if (!cp || strcmp(ptr->domainname->name, cp + 1))
				continue;
			dp->list[index].is_dk = true;
		}
	}

	/* Create domain initializer source domains. */
	for (index = 0; index < max_index; index++) {
		const struct path_info *domainname
			= dp->list[index].domainname;
		const struct path_info **string_ptr
			= dp->list[index].string_ptr;
		const int max_count = dp->list[index].string_count;
		/* Don't create source domain under <kernel> because
		   they will become target domains. */
		if (domainname->total_len == ROOT_NAME_LEN)
			continue;
		for (i = 0; i < max_count; i++) {
			const struct path_info *cp = string_ptr[i];
			struct path_group_entry *group;
			if (cp->name[0] != '@') {
				assign_domain_initializer_source(dp, domainname,
								 cp->name);
				continue;
			}
			group = find_path_group(cp->name + 1);
			if (!group)
				continue;
			for (j = 0; j < group->member_name_len; j++) {
				cp = group->member_name[j];
				assign_domain_initializer_source(dp, domainname,
								 cp->name);
			}
		}
	}

	/* Create missing parent domains. */
	for (index = 0; index < max_index; index++) {
		get();
		shprintf("%s", domain_name(dp, index));
		while (true) {
			char *cp = strrchr(shared_buffer, ' ');
			if (!cp)
				break;
			*cp = '\0';
			if (find_domain(dp, shared_buffer, false, false) != EOF)
				continue;
			if (find_or_assign_new_domain(dp, shared_buffer, false,
						      true)
			    == EOF)
				out_of_memory();
		}
		put();
	}

	/* Sort by domain name. */
	qsort(dp->list, dp->list_len, sizeof(struct domain_info),
	      domainname_attribute_compare);

	/* Assign domain numbers. */
	{
		int number = 0;
		int index;
		unnumbered_domain_count = 0;
		for (index = 0; index < dp->list_len; index++) {
			if (is_deleted_domain(dp, index) ||
			    is_initializer_source(dp, index)) {
				dp->list[index].number = -1;
				unnumbered_domain_count++;
			} else {
				dp->list[index].number = number++;
			}
		}
	}

	dp->list_selected = realloc(dp->list_selected, dp->list_len);
	if (dp->list_len && !dp->list_selected)
		out_of_memory();
	memset(dp->list_selected, 0, dp->list_len);
}

static void show_current(struct domain_policy *dp);

static int window_width = 0;
static int window_height = 0;
static int current_y[MAXSCREEN];
static int current_item_index[MAXSCREEN];
static int list_item_count[MAXSCREEN];

static const int header_lines = 3;
static int body_lines = 0;

static int max_eat_col[MAXSCREEN];
static int eat_col = 0;
static int max_col = 0;
static int list_indent = 0;

static const char *eat(const char *str)
{
	while (*str && eat_col) {
		str++;
		eat_col--;
	}
	return str;
}

static int show_domain_line(struct domain_policy *dp, int index)
{
	int tmp_col = 0;
	const struct domain_initializer_entry *domain_initializer;
	const struct domain_keeper_entry *domain_keeper;
	const char *sp;
	const int number = dp->list[index].number;
	int redirect_index;
	if (number >= 0)
		printw("%c%4d:%3u %c%c%c ",
			 dp->list_selected[index] ? '&' : ' ',
			 number, dp->list[index].profile,
			 is_keeper_domain(dp, index) ? '#' : ' ',
			 is_initializer_target(dp, index) ? '*' : ' ',
			 is_domain_unreachable(dp, index) ? '!' : ' ');
	else
		printw("              ");
	tmp_col += 14;
	sp = domain_name(dp, index);
	while (true) {
		const char *cp = strchr(sp, ' ');
		if (!cp)
			break;
		printw("%s", eat("    "));
		tmp_col += 4;
		sp = cp + 1;
	}
	if (is_deleted_domain(dp, index)) {
		printw("%s", eat("( "));
		tmp_col += 2;
	}
	printw("%s", eat(sp));
	tmp_col += strlen(sp);
	if (is_deleted_domain(dp, index)) {
		printw("%s", eat(" )"));
		tmp_col += 2;
	}
	domain_initializer = dp->list[index].d_i;
	if (!domain_initializer)
		goto not_domain_initializer;
	get();
	if (domain_initializer->domainname)
		shprintf(" ( " KEYWORD_INITIALIZE_DOMAIN "%s from %s )",
			 domain_initializer->program->name,
			 domain_initializer->domainname->name);
	else
		shprintf(" ( " KEYWORD_INITIALIZE_DOMAIN "%s )",
			 domain_initializer->program->name);
	printw("%s", eat(shared_buffer));
	tmp_col += strlen(shared_buffer);
	put();
	goto done;
not_domain_initializer:
	domain_keeper = dp->list[index].d_k;
	if (!domain_keeper)
		goto not_domain_keeper;
	get();
	if (domain_keeper->program)
		shprintf(" ( " KEYWORD_KEEP_DOMAIN "%s from %s )",
			 domain_keeper->program->name,
			 domain_keeper->domainname->name);
	else
		shprintf(" ( " KEYWORD_KEEP_DOMAIN "%s )",
			 domain_keeper->domainname->name);
	printw("%s", eat(shared_buffer));
	tmp_col += strlen(shared_buffer);
	put();
	goto done;
not_domain_keeper:
	if (!is_initializer_source(dp, index))
		goto done;
	get();
	shprintf(ROOT_NAME "%s", strrchr(domain_name(dp, index), ' '));
	redirect_index = find_domain(dp, shared_buffer, false, false);
	if (redirect_index >= 0)
		shprintf(" ( -> %d )", dp->list[redirect_index].number);
	else
		shprintf(" ( -> Not Found )");
	printw("%s", eat(shared_buffer));
	tmp_col += strlen(shared_buffer);
	put();
done:
	return tmp_col;
}

static int show_acl_line(int index, int list_indent)
{
	u8 directive = generic_acl_list[index].directive;
	const char *cp1 = directives[directive].alias;
	const char *cp2 = generic_acl_list[index].operand;
	int len = list_indent - directives[directive].alias_len;
	printw("%c%4d: %s ",
	       generic_acl_list[index].selected ? '&' : ' ',
	       index, eat(cp1));
	while (len-- > 0)
		printw("%s", eat(" "));
	printw("%s", eat(cp2));
	return strlen(cp1) + strlen(cp2) + 8 + list_indent;
}

static int show_literal_line(int index)
{
	const char *cp = generic_acl_list[index].operand;
	printw("%c%4d: %s ",
	       generic_acl_list[index].selected ? '&' : ' ',
	       index, eat(cp));
	return strlen(cp) + 8;
}

static void show_list(struct domain_policy *dp)
{
	const int offset = current_item_index[current_screen];
	int i;
	int tmp_col;
	if (current_screen == SCREEN_DOMAIN_LIST)
		list_item_count[SCREEN_DOMAIN_LIST] = dp->list_len;
	else
		list_item_count[current_screen] = generic_acl_list_count;
	clear();
	move(0, 0);
	if (window_height < header_lines + 1) {
		printw("Please enlarge window.");
		clrtobot();
		refresh();
		return;
	}
	/* add color */
	editpolicy_color_change(editpolicy_color_head(current_screen), true);
	if (current_screen == SCREEN_DOMAIN_LIST) {
		int i = list_item_count[SCREEN_DOMAIN_LIST]
			- unnumbered_domain_count;
		printw("<<< Domain Transition Editor >>>"
		       "      %d domain%c    '?' for help",
		       i, i > 1 ? 's' : ' ');
	} else {
		int i = list_item_count[current_screen];
		printw("<<< %s >>>"
		       "      %d entr%s    '?' for help", list_caption,
		       i, i > 1 ? "ies" : "y");
	}
	/* add color */
	editpolicy_color_change(editpolicy_color_head(current_screen), false);
	eat_col = max_eat_col[current_screen];
	max_col = 0;
	if (current_screen == SCREEN_ACL_LIST) {
		get();
		shprintf("%s", eat(current_domain));
		editpolicy_attr_change(A_REVERSE, true);  /* add color */
		move(2, 0);
		printw("%s", shared_buffer);
		editpolicy_attr_change(A_REVERSE, false); /* add color */
		put();
	}
	list_indent = 0;
	switch (current_screen) {
	case SCREEN_SYSTEM_LIST:
	case SCREEN_EXCEPTION_LIST:
	case SCREEN_ACL_LIST:
		for (i = 0; i < list_item_count[current_screen]; i++) {
			const u8 directive = generic_acl_list[i].directive;
			const int len = directives[directive].alias_len;
			if (len > list_indent)
				list_indent = len;
		}
		break;
	}
	for (i = 0; i < body_lines; i++) {
		const int index = offset + i;
		eat_col = max_eat_col[current_screen];
		if (index >= list_item_count[current_screen])
			break;
		move(header_lines + i, 0);
		switch (current_screen) {
		case SCREEN_DOMAIN_LIST:
			tmp_col = show_domain_line(dp, index);
			break;
		case SCREEN_SYSTEM_LIST:
		case SCREEN_EXCEPTION_LIST:
		case SCREEN_ACL_LIST:
			tmp_col = show_acl_line(index, list_indent);
			break;
		default:
			tmp_col = show_literal_line(index);
			break;
		}
		clrtoeol();
		tmp_col -= window_width;
		if (tmp_col > max_col)
			max_col = tmp_col;
	}
	show_current(dp);
}

static void resize_window(void)
{
	getmaxyx(stdscr, window_height, window_width);
	body_lines = window_height - header_lines;
	if (body_lines <= current_y[current_screen])
		current_y[current_screen] = body_lines - 1;
	if (current_y[current_screen] < 0)
		current_y[current_screen] = 0;
}

static void up_arrow_key(struct domain_policy *dp)
{
	if (current_y[current_screen] > 0) {
		current_y[current_screen]--;
		show_current(dp);
	} else if (current_item_index[current_screen] > 0) {
		current_item_index[current_screen]--;
		show_list(dp);
	}
}

static void down_arrow_key(struct domain_policy *dp)
{
	if (current_y[current_screen] < body_lines - 1) {
		if (current_item_index[current_screen]
		    + current_y[current_screen]
		    < list_item_count[current_screen] - 1) {
			current_y[current_screen]++;
			show_current(dp);
		}
	} else if (current_item_index[current_screen]
		   + current_y[current_screen]
		   < list_item_count[current_screen] - 1) {
		current_item_index[current_screen]++;
		show_list(dp);
	}
}

static void page_up_key(struct domain_policy *dp)
{
	if (current_item_index[current_screen] + current_y[current_screen]
	    > body_lines) {
		current_item_index[current_screen] -= body_lines;
		if (current_item_index[current_screen] < 0)
			current_item_index[current_screen] = 0;
		show_list(dp);
	} else if (current_item_index[current_screen]
		   + current_y[current_screen] > 0) {
		current_item_index[current_screen] = 0;
		current_y[current_screen] = 0;
		show_list(dp);
	}
}

static void page_down_key(struct domain_policy *dp)
{
	if (list_item_count[current_screen] - current_item_index[current_screen]
	    > body_lines) {
		current_item_index[current_screen] += body_lines;
		if (current_item_index[current_screen]
		    + current_y[current_screen]
		    > list_item_count[current_screen] - 1)
			current_y[current_screen]
				= list_item_count[current_screen] - 1
				- current_item_index[current_screen];
		show_list(dp);
	} else if (current_item_index[current_screen]
		   + current_y[current_screen]
		   < list_item_count[current_screen] - 1) {
		current_y[current_screen]
			= list_item_count[current_screen]
			- current_item_index[current_screen] - 1;
		show_current(dp);
	}
}

static int get_current(void)
{
	if (list_item_count[current_screen] == 0)
		return EOF;
	if (current_item_index[current_screen] + current_y[current_screen] < 0
	    || current_item_index[current_screen] + current_y[current_screen]
	    >= list_item_count[current_screen]) {
		fprintf(stderr, "ERROR: current_item_index=%d current_y=%d\n",
			current_item_index[current_screen],
			current_y[current_screen]);
		exit(127);
	}
	return current_item_index[current_screen] + current_y[current_screen];
}

/* add color start */
#ifdef COLOR_ON
static int before_current[MAXSCREEN] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static int before_y[MAXSCREEN]       = { -1, -1, -1, -1, -1, -1, -1, -1 };

static void editpolicy_line_draw(void)
{
	int current = get_current();
	int y, x;

	if (current == EOF)
		return;

	getyx(stdscr, y, x);
	if (-1 < before_current[current_screen] &&
	    current != before_current[current_screen]){
		move(header_lines + before_y[current_screen], 0);
		chgat(-1, A_NORMAL, NORMAL, NULL);
	}

	move(y, x);
	chgat(-1, A_NORMAL, editpolicy_color_cursor(current_screen), NULL);
	touchwin(stdscr);

	before_current[current_screen] = current;
	before_y[current_screen] = current_y[current_screen];
}
#else
#define editpolicy_line_draw()
#endif
/* add color end */

static void show_current(struct domain_policy *dp)
{
	if (current_screen == SCREEN_DOMAIN_LIST) {
		get();
		eat_col = max_eat_col[current_screen];
		shprintf("%s", eat(domain_name(dp, get_current())));
		if (window_width < sizeof(shared_buffer))
			shared_buffer[window_width] = '\0';
		move(2, 0);
		clrtoeol();
		editpolicy_attr_change(A_REVERSE, true);  /* add color */
		printw("%s", shared_buffer);
		editpolicy_attr_change(A_REVERSE, false); /* add color */
		put();
	}
	move(header_lines + current_y[current_screen], 0);
	editpolicy_line_draw();     /* add color */
	refresh();
}

static void adjust_cursor_pos(const int item_count)
{
	if (item_count == 0) {
		current_item_index[current_screen] = 0;
		current_y[current_screen] = 0;
	} else {
		while (current_item_index[current_screen]
		       + current_y[current_screen] >= item_count) {
			if (current_y[current_screen] > 0)
				current_y[current_screen]--;
			else if (current_item_index[current_screen] > 0)
				current_item_index[current_screen]--;
		}
	}
}

static void set_cursor_pos(const int index)
{
	while (index < current_y[current_screen]
	       + current_item_index[current_screen]) {
		if (current_y[current_screen] > 0)
			current_y[current_screen]--;
		else
			current_item_index[current_screen]--;
	}
	while (index > current_y[current_screen]
	       + current_item_index[current_screen]) {
		if (current_y[current_screen] < body_lines - 1)
			current_y[current_screen]++;
		else
			current_item_index[current_screen]++;
	}
}

static int count(const unsigned char *array, const int len)
{
	int i;
	int c = 0;
	for (i = 0; i < len; i++)
		if (array[i])
			c++;
	return c;
}

static int count2(const struct generic_acl *array, int len)
{
	int i;
	int c = 0;
	for (i = 0; i < len; i++)
		if (array[i].selected)
			c++;
	return c;
}

static int select_item(struct domain_policy *dp, const int current)
{
	if (current >= 0) {
		int x;
		int y;
		if (current_screen == SCREEN_DOMAIN_LIST) {
			if (is_deleted_domain(dp, current) ||
			    is_initializer_source(dp, current))
				return 0;
			dp->list_selected[current] ^= 1;
		} else {
			generic_acl_list[current].selected ^= 1;
		}
		getyx(stdscr, y, x);
		editpolicy_sttr_save();    /* add color */
		show_list(dp);
		editpolicy_sttr_restore(); /* add color */
		move(y, x);
		return 1;
	}
	return 0;
}

static u8 split_acl(const u8 index, char *data, struct path_info *arg1,
		    struct path_info *arg2, struct path_info *arg3)
{
	/* data = w[0] w[1] ... w[n-1] w[n] if c[0] c[1] ... c[m] ; set ... */
	/*                                                                  */
	/* arg1 = w[0]                                                      */
	/* arg2 = w[1] ... w[n-1] w[n]                                      */
	/* arg3 = if c[0] c[1] ... c[m] ; set ...                           */
	u8 subtype = 0;
	char *cp;
	arg1->name = data;
	cp = strstr(data, " if ");
	if (cp) {
		while (true) {
			char *cp2 = strstr(cp + 3, " if ");
			if (!cp2)
				break;
			cp = cp2;
		}
		*cp++ = '\0';
		goto ok;
	}
	cp = strstr(data, " ; set ");
	if (cp)
		*cp++ = '\0';
	else
		cp = "";
ok:
	arg3->name = cp;
	if (index == DIRECTIVE_ALLOW_NETWORK) {
		/*
		 * Prune protocol component and operation component so that
		 * arg1 will point to IP address component.
		 */
		if (str_starts(data, "UDP bind "))
			subtype = 1;
		else if (str_starts(data, "UDP connect "))
			subtype = 2;
		else if (str_starts(data, "TCP bind "))
			subtype = 3;
		else if (str_starts(data, "TCP listen "))
			subtype = 4;
		else if (str_starts(data, "TCP connect "))
			subtype = 5;
		else if (str_starts(data, "TCP accept "))
			subtype = 6;
		else if (str_starts(data, "RAW bind "))
			subtype = 7;
		else if (str_starts(data, "RAW connect "))
			subtype = 8;
	}
	cp = strchr(data, ' ');
	if (cp)
		*cp++ = '\0';
	else
		cp = "";
	arg2->name = cp;
	fill_path_info(arg1);
	fill_path_info(arg2);
	fill_path_info(arg3);
	return subtype;
}

static int sort_type = 1;

static int generic_acl_compare(const void *a, const void *b)
{
	const struct generic_acl *a0 = (struct generic_acl *) a;
	const struct generic_acl *b0 = (struct generic_acl *) b;
	const char *a1 = directives[a0->directive].alias;
	const char *b1 = directives[b0->directive].alias;
	const char *a2 = a0->operand;
	const char *b2 = b0->operand;
	if (sort_type == 0) {
		const int ret = strcmp(a1, b1);
		if (ret)
			return ret;
		return strcmp(a2, b2);
	} else {
		const int ret = strcmp(a2, b2);
		if (ret)
			return ret;
		return strcmp(a1, b1);
	}
}

static _Bool compare_path(struct path_info *sarg, struct path_info *darg,
			 u8 directive)
{
	int i;
	struct path_group_entry *group;
	_Bool may_use_pattern = !darg->is_patterned
		&& (directive != DIRECTIVE_1)
		&& (directive != DIRECTIVE_3)
		&& (directive != DIRECTIVE_5)
		&& (directive != DIRECTIVE_7)
		&& (directive != DIRECTIVE_ALLOW_EXECUTE);
	if (!pathcmp(sarg, darg))
		return true;
	if (darg->name[0] == '@')
		return false;
	if (sarg->name[0] != '@') {
		/* Pathname component. */
		return may_use_pattern && path_matches_pattern(darg, sarg);
	}
	/* path_group component. */
	group = find_path_group(sarg->name + 1);
	if (!group)
		return false;
	for (i = 0; i < group->member_name_len; i++) {
		const struct path_info *member_name;
		member_name = group->member_name[i];
		if (!pathcmp(member_name, darg))
			return true;
		if (may_use_pattern && path_matches_pattern(darg, member_name))
			return true;
	}
	return false;
}

static _Bool compare_address(struct path_info *sarg, struct path_info *darg)
{
	int i;
	struct ip_address_entry sentry;
	struct ip_address_entry dentry;
	struct address_group_entry *group;
	if (parse_ip(darg->name, &dentry))
		return false;
	if (sarg->name[0] != '@') {
		/* IP address component. */
		if (parse_ip(sarg->name, &sentry))
			return false;
		if (sentry.is_ipv6 != dentry.is_ipv6 ||
		    memcmp(dentry.min, sentry.min, 16) < 0 ||
		    memcmp(sentry.max, dentry.max, 16) < 0)
			return false;
		return true;
	}
	/* IP address group component. */
	group = find_address_group(sarg->name + 1);
	if (!group)
		return false;
	for (i = 0; i < group->member_name_len; i++) {
		struct ip_address_entry *sentry = &group->member_name[i];
		if (sentry->is_ipv6 == dentry.is_ipv6
		    && memcmp(sentry->min, dentry.min, 16) <= 0
		    && memcmp(dentry.max, sentry->max, 16) <= 0)
			return true;
	}
	return false;
}

static void try_optimize(struct domain_policy *dp, const int current)
{
	char *cp;
	u8 s_index;
	int index;
	struct path_info sarg1;
	struct path_info sarg2;
	struct path_info sarg3;
	struct path_info darg1;
	struct path_info darg2;
	struct path_info darg3;
	u8 subtype1;
	u8 subtype2;
	if (current < 0)
		return;
	s_index = generic_acl_list[current].directive;
	if (s_index == DIRECTIVE_NONE)
		return;
	cp = strdup(generic_acl_list[current].operand);
	if (!cp)
		return;

	subtype1 = split_acl(s_index, cp, &sarg1, &sarg2, &sarg3);

	get();
	for (index = 0; index < list_item_count[current_screen]; index++) {
		const u8 d_index = generic_acl_list[index].directive;
		if (index == current)
			continue;
		if (generic_acl_list[index].selected)
			continue;
		if (s_index == DIRECTIVE_6 ||
		    s_index == DIRECTIVE_ALLOW_READ_WRITE) {
			/* Source starts with "6 " or "allow_read/write " */
			if (d_index == DIRECTIVE_6) {
				/* Dest starts with "6 " */
			} else if (d_index == DIRECTIVE_ALLOW_READ_WRITE) {
				/* Dest starts with "allow_read/write " */
			} else if (d_index == DIRECTIVE_2) {
				/* Dest starts with "2 " */
			} else if (d_index == DIRECTIVE_4) {
				/* Dest starts with "4 " */
			} else if (d_index == DIRECTIVE_ALLOW_READ) {
				/* Dest starts with "allow_read " */
			} else if (d_index == DIRECTIVE_ALLOW_WRITE) {
				/* Dest starts with "allow_write " */
			} else {
				/* Source and dest start with same directive. */
				continue;
			}
		} else if (s_index == DIRECTIVE_2 &&
			   d_index == DIRECTIVE_ALLOW_WRITE) {
			/* Source starts with "2 " and dest starts with
			   "allow_write " */
		} else if (s_index == DIRECTIVE_4 &&
			   d_index == DIRECTIVE_ALLOW_READ) {
			/* Source starts with "4 " and dest starts with
			   "allow_read " */
		} else if (s_index == DIRECTIVE_ALLOW_WRITE &&
			   d_index == DIRECTIVE_2) {
			/* Source starts with "allow_write " and dest starts
			   with "2 " */
		} else if (s_index == DIRECTIVE_ALLOW_READ &&
			   d_index == DIRECTIVE_4) {
			/* Source starts with "allow_read " and dest starts
			   with "4 " */
		} else if (s_index == d_index) {
			/* Source and dest start with same directive. */
		} else {
			/* Source and dest start with different directive. */
			continue;
		}
		strncpy(shared_buffer, generic_acl_list[index].operand,
			sizeof(shared_buffer));
		if (!memchr(shared_buffer, '\0', sizeof(shared_buffer)))
			continue; /* Line too long. */

		subtype2 = split_acl(d_index, shared_buffer, &darg1, &darg2,
				     &darg3);

		if (subtype1 != subtype2)
			continue;

		/* Compare condition part. */
		if (pathcmp(&sarg3, &darg3))
			continue;

		/* Compare first word. */
		switch (d_index) {
		case DIRECTIVE_1:
		case DIRECTIVE_2:
		case DIRECTIVE_3:
		case DIRECTIVE_4:
		case DIRECTIVE_5:
		case DIRECTIVE_6:
		case DIRECTIVE_7:
		case DIRECTIVE_ALLOW_EXECUTE:
		case DIRECTIVE_ALLOW_READ:
		case DIRECTIVE_ALLOW_WRITE:
		case DIRECTIVE_ALLOW_READ_WRITE:
		case DIRECTIVE_ALLOW_CREATE:
		case DIRECTIVE_ALLOW_UNLINK:
		case DIRECTIVE_ALLOW_MKDIR:
		case DIRECTIVE_ALLOW_RMDIR:
		case DIRECTIVE_ALLOW_MKFIFO:
		case DIRECTIVE_ALLOW_MKSOCK:
		case DIRECTIVE_ALLOW_MKBLOCK:
		case DIRECTIVE_ALLOW_MKCHAR:
		case DIRECTIVE_ALLOW_TRUNCATE:
		case DIRECTIVE_ALLOW_SYMLINK:
		case DIRECTIVE_ALLOW_LINK:
		case DIRECTIVE_ALLOW_RENAME:
		case DIRECTIVE_ALLOW_REWRITE:
			if (!compare_path(&sarg1, &darg1, d_index))
				continue;
			break;
		case DIRECTIVE_ALLOW_ARGV0:
			/* Pathname component. */
			if (!pathcmp(&sarg1, &darg1))
				break;
			/* allow_argv0 doesn't support path_group. */
			if (darg1.name[0] == '@' || darg1.is_patterned ||
			    !path_matches_pattern(&darg1, &sarg1))
				continue;
			break;
		case DIRECTIVE_ALLOW_SIGNAL:
			/* Signal number component. */
			if (strcmp(sarg1.name, darg1.name))
				continue;
			break;
		case DIRECTIVE_ALLOW_NETWORK:
			if (!compare_address(&sarg1, &darg1))
				continue;
			break;
		case DIRECTIVE_ALLOW_ENV:
			/* An environemnt variable name component. */
			if (!pathcmp(&sarg1, &darg1))
				break;
			/* allow_env doesn't interpret leading @ as
			   path_group. */
			if (darg1.is_patterned ||
			    !path_matches_pattern(&darg1, &sarg1))
				continue;
			break;
		default:
			continue;
		}

		/* Compare rest words. */
		switch (d_index) {
			char c;
			unsigned int smin;
			unsigned int smax;
			unsigned int dmin;
			unsigned int dmax;
		case DIRECTIVE_ALLOW_LINK:
		case DIRECTIVE_ALLOW_RENAME:
			if (!compare_path(&sarg2, &darg2, d_index))
				continue;
			break;
		case DIRECTIVE_ALLOW_ARGV0:
			/* Basename component. */
			if (!pathcmp(&sarg2, &darg2))
				break;
			if (darg2.is_patterned ||
			    !path_matches_pattern(&darg2, &sarg2))
				continue;
			break;
		case DIRECTIVE_ALLOW_SIGNAL:
			/* Domainname component. */
			if (strncmp(sarg2.name, darg2.name, sarg2.total_len))
				continue;
			c = darg2.name[sarg2.total_len];
			if (c && c != ' ')
				continue;
			break;
		case DIRECTIVE_ALLOW_NETWORK:
			/* Port number component. */
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
			if (smin > dmin || smax < dmax)
				continue;
			break;
		default:
			/* This must be empty. */
			if (sarg2.total_len || darg2.total_len)
				continue;
		}
		generic_acl_list[index].selected = 1;
	}
	put();
	free(cp);
	show_list(dp);
}

static void delete_entry(struct domain_policy *dp, int current)
{
	int c;
	move(1, 0);
	editpolicy_color_change(DISP_ERR, true);	/* add color */
	if (current_screen == SCREEN_DOMAIN_LIST) {
		c = count(dp->list_selected, dp->list_len);
		if (!c)
			c = select_item(dp, current);
		if (!c)
			printw("Select domain using Space key first.");
		else
			printw("Delete selected domain%s? ('Y'es/'N'o)",
			       c > 1 ? "s" : "");
	} else {
		c = count2(generic_acl_list, generic_acl_list_count);
		if (!c)
			c = select_item(dp, current);
		if (!c)
			printw("Select entry using Space key first.");
		else
			printw("Delete selected entr%s? ('Y'es/'N'o)",
			       c > 1 ? "ies" : "y");
	}
	editpolicy_color_change(DISP_ERR, false);	/* add color */
	clrtoeol();
	refresh();
	if (!c)
		return;
	do {
		c = getch2();
	} while (!(c == 'Y' || c == 'y' || c == 'N' || c == 'n' || c == EOF));
	resize_window();
	if (c != 'Y' && c != 'y') {
		show_list(dp);
		return;
	}
	if (current_screen == SCREEN_DOMAIN_LIST) {
		int index;
		FILE *fp = open_write(proc_policy_domain_policy);
		if (!fp)
			return;
		for (index = 1; index < dp->list_len; index++) {
			if (!dp->list_selected[index])
				continue;
			fprintf(fp, "delete %s\n", domain_name(dp, index));
		}
		fclose(fp);
	} else {
		int index;
		FILE *fp = open_write(policy_file);
		if (!fp)
			return;
		if (current_screen == SCREEN_ACL_LIST)
			fprintf(fp, "select %s\n", current_domain);
		for (index = 0; index < generic_acl_list_count; index++) {
			u8 directive;
			if (!generic_acl_list[index].selected)
				continue;
			directive = generic_acl_list[index].directive;
			fprintf(fp, "delete %s %s\n",
				directives[directive].original,
				generic_acl_list[index].operand);
		}
		fclose(fp);
	}
}

struct readline_data {
	const char **history;
	int count;
	int max;
	char *last_error;
	char *search_buffer[MAXSCREEN];
};

static void add_entry(struct readline_data *rl)
{
	char *line;
	editpolicy_attr_change(A_BOLD, true);  /* add color */
	line = simple_readline(window_height - 1, 0, "Enter new entry> ",
			       rl->history, rl->count, 8192, 8);
	editpolicy_attr_change(A_BOLD, false); /* add color */
	if (!line || !*line)
		goto out;
	rl->count = simple_add_history(line, rl->history, rl->count, rl->max);
	FILE *fp = open_write(policy_file);
	if (!fp)
		goto out;
	switch (current_screen) {
		u8 directive;
	case SCREEN_DOMAIN_LIST:
		if (!is_correct_domain(line)) {
			const int len = strlen(line) + 128;
			rl->last_error = realloc(rl->last_error, len);
			if (!rl->last_error)
				out_of_memory();
			memset(rl->last_error, 0, len);
			snprintf(rl->last_error, len - 1,
				 "%s is an invalid domainname.", line);
			line[0] = '\0';
		}
		break;
	case SCREEN_ACL_LIST:
		fprintf(fp, "select %s\n", current_domain);
		/* Fall through. */
	case SCREEN_SYSTEM_LIST:
	case SCREEN_EXCEPTION_LIST:
		directive = find_directive(false, line);
		if (directive != DIRECTIVE_NONE)
			fprintf(fp, "%s ",
				directives[directive].original);
		break;
	case SCREEN_PROFILE_LIST:
		if (!strchr(line, '='))
			fprintf(fp, "%s-COMMENT=\n", line);
		break;
	}
	fprintf(fp, "%s\n", line);
	fclose(fp);
out:
	free(line);
}

static void find_entry(struct domain_policy *dp, _Bool input, _Bool forward,
		       int current, struct readline_data *rl)
{
	int index = current;
	char *line = NULL;
	if (current == EOF)
		return;
	if (!input)
		goto start_search;
	editpolicy_attr_change(A_BOLD, true);  /* add color */
	line = simple_readline(window_height - 1, 0, "Search> ",
			       rl->history, rl->count, 8192, 8);
	editpolicy_attr_change(A_BOLD, false); /* add color */
	if (!line || !*line)
		goto out;
	rl->count = simple_add_history(line, rl->history, rl->count, rl->max);
	free(rl->search_buffer[current_screen]);
	rl->search_buffer[current_screen] = line;
	line = NULL;
	index = -1;
start_search:
	get();
	while (true) {
		const char *cp;
		if (forward) {
			if (++index >= list_item_count[current_screen])
				break;
		} else {
			if (--index < 0)
				break;
		}
		if (current_screen == SCREEN_DOMAIN_LIST)
			cp = get_last_name(dp, index);
		else {
			const u8 directive = generic_acl_list[index].directive;
			shprintf("%s %s", directives[directive].alias,
				 generic_acl_list[index].operand);
			cp = shared_buffer;
		}
		if (!strstr(cp, rl->search_buffer[current_screen]))
			continue;
		set_cursor_pos(index);
		break;
	}
	put();
out:
	free(line);
	show_list(dp);
}

static void set_profile(struct domain_policy *dp, int current)
{
	int index;
	FILE *fp;
	char *line;
	if (!count(dp->list_selected, dp->list_len) &&
	    !select_item(dp, current)) {
		move(1, 0);
		printw("Select domain using Space key first.");
		clrtoeol();
		refresh();
		return;
	}
	editpolicy_attr_change(A_BOLD, true);  /* add color */
	line = simple_readline(window_height - 1, 0, "Enter profile number> ",
			       NULL, 0, 8, 1);
	editpolicy_attr_change(A_BOLD, false); /* add color */
	if (!line || !*line)
		goto out;
	fp = open_write(proc_policy_domain_policy);
	if (!fp)
		goto out;
	for (index = 0; index < dp->list_len; index++) {
		if (!dp->list_selected[index])
			continue;
		fprintf(fp, "select %s\n" KEYWORD_USE_PROFILE "%s\n",
			domain_name(dp, index), line);
	}
	fclose(fp);
out:
	free(line);
}

static void set_level(struct domain_policy *dp, int current)
{
	int index;
	FILE *fp;
	char *line;
	if (!count2(generic_acl_list, generic_acl_list_count))
		select_item(dp, current);
	editpolicy_attr_change(A_BOLD, true);  /* add color */
	line = simple_readline(window_height - 1, 0, "Enter new value> ",
			       NULL, 0, 8192, 1);
	editpolicy_attr_change(A_BOLD, false); /* add color */
	if (!line || !*line)
		goto out;
	fp = open_write(proc_policy_profile);
	if (!fp)
		goto out;
	for (index = 0; index < generic_acl_list_count; index++) {
		char *cp;
		if (!generic_acl_list[index].selected)
			continue;
		get();
		strncpy(shared_buffer, generic_acl_list[index].operand,
			sizeof(shared_buffer) - 1);
		cp = strchr(shared_buffer, '=');
		if (cp)
			*cp = '\0';
		fprintf(fp, "%s=%s\n", shared_buffer, line);
		put();
	}
	fclose(fp);
out:
	free(line);
}

static void set_quota(struct domain_policy *dp, int current)
{
	int index;
	FILE *fp;
	char *line;
	if (!count2(generic_acl_list, generic_acl_list_count))
		select_item(dp, current);
	editpolicy_attr_change(A_BOLD, true);  /* add color */
	line = simple_readline(window_height - 1, 0, "Enter new value> ",
			       NULL, 0, 20, 1);
	editpolicy_attr_change(A_BOLD, false); /* add color */
	if (!line || !*line)
		goto out;
	fp = open_write(proc_policy_meminfo);
	if (!fp)
		goto out;
	for (index = 0; index < generic_acl_list_count; index++) {
		char *cp;
		if (!generic_acl_list[index].selected)
			continue;
		get();
		strncpy(shared_buffer, generic_acl_list[index].operand,
			sizeof(shared_buffer) - 1);
		cp = strchr(shared_buffer, ':');
		if (cp)
			*cp = '\0';
		fprintf(fp, "%s: %s\n", shared_buffer, line);
		put();
	}
	fclose(fp);
out:
	free(line);
}

static int select_window(struct domain_policy *dp, const int current)
{
	move(0, 0);
	printw("Press one of below keys to switch window.\n\n");
	printw("s     <<< System Policy Editor >>>\n");
	printw("e     <<< Exception Policy Editor >>>\n");
	printw("d     <<< Domain Transition Editor >>>\n");
	if (current_screen == SCREEN_DOMAIN_LIST &&
	    !is_initializer_source(dp, current) &&
	    !is_deleted_domain(dp, current))
		printw("a     <<< Domain Policy Editor >>>\n");
	printw("p     <<< Profile Editor >>>\n");
	printw("m     <<< Manager Policy Editor >>>\n");
	if (!offline_mode) {
		/* printw("i     <<< Interactive Enforcing Mode >>>\n"); */
		printw("q     <<< Memory Quota Editor >>>\n");
	}
	clrtobot();
	refresh();
	while (true) {
		int c = getch2();
		if (c == 'S' || c == 's')
			return SCREEN_SYSTEM_LIST;
		if (c == 'E' || c == 'e')
			return SCREEN_EXCEPTION_LIST;
		if (c == 'D' || c == 'd')
			return SCREEN_DOMAIN_LIST;
		if (c == 'A' || c == 'a') {
			if (current_screen == SCREEN_DOMAIN_LIST &&
			    !is_initializer_source(dp, current) &&
			    !is_deleted_domain(dp, current)) {
				free(current_domain);
				current_domain = strdup(domain_name(dp,
								    current));
				if (!current_domain)
					out_of_memory();
				return SCREEN_ACL_LIST;
			}
		}
		if (c == 'P' || c == 'p')
			return SCREEN_PROFILE_LIST;
		if (c == 'M' || c == 'm')
			return SCREEN_MANAGER_LIST;
		if (!offline_mode) {
			/*
			if (c == 'I' || c == 'i')
				return SCREEN_QUERY_LIST;
			*/
			if (c == 'Q' || c == 'q')
				return SCREEN_MEMINFO_LIST;
		}
		if (c == EOF)
			return MAXSCREEN;
	}
}

static void show_command_key(void)
{
	int c;
	clear();
	printw("Commands available for this screen are:\n\n");
	printw("Q/q        Quit this editor.\n");
	printw("R/r        Refresh to the latest information.\n");
	switch (current_screen) {
	case SCREEN_MEMINFO_LIST:
		break;
	default:
		printw("F/f        Find first.\n");
		printw("N/n        Find next.\n");
		printw("P/p        Find previous.\n");
	}
	printw("W/w        Switch to selected screen.\n");
	/* printw("Tab        Switch to next screen.\n"); */
	switch (current_screen) {
	case SCREEN_MEMINFO_LIST:
		break;
	default:
		printw("Insert     Copy an entry at the cursor position to "
		       "history buffer.\n");
	}
	printw("Space      Invert selection state of an entry at the cursor "
	       "position.\n");
	printw("C/c        Copy selection state of an entry at the cursor "
	       "position to all entries below the cursor position.\n");
	switch (current_screen) {
	case SCREEN_DOMAIN_LIST:
		printw("A/a        Add a new domain.\n");
		printw("Enter      Edit ACLs of a domain at the cursor "
		       "position.\n");
		printw("D/d        Delete selected domains.\n");
		printw("S/s        Set profile number of selected domains.\n");
		break;
	case SCREEN_MEMINFO_LIST:
		printw("S/s        Set memory quota of selected items.\n");
		break;
	case SCREEN_PROFILE_LIST:
		printw("S/s        Set mode of selected items.\n");
		break;
	}
	switch (current_screen) {
	case SCREEN_SYSTEM_LIST:
	case SCREEN_EXCEPTION_LIST:
	case SCREEN_ACL_LIST:
	case SCREEN_MANAGER_LIST:
		printw("A/a        Add a new entry.\n");
		printw("D/d        Delete selected entries.\n");
	}
	switch (current_screen) {
	case SCREEN_PROFILE_LIST:
		printw("A/a        Define a new profile.\n");
	}
	switch (current_screen) {
	case SCREEN_ACL_LIST:
		printw("O/o        Set selection state to other entries "
		       "included in an entry at the cursor position.\n");
		printw("@          Switch sort type.\n");
	}
	printw("Arrow-keys and PageUp/PageDown/Home/End keys "
	       "for scroll.\n\n");
	printw("Press '?' to escape from this help.\n");
	refresh();
	while (true) {
		c = getch2();
		if (c == '?' || c == EOF)
			break;
	}
}

static int generic_list_loop(struct domain_policy *dp)
{
	static struct readline_data rl;
	static int saved_current_y[MAXSCREEN];
	static int saved_current_item_index[MAXSCREEN];
	static _Bool first = true;
	if (first) {
		memset(&rl, 0, sizeof(rl));
		rl.max = 20;
		rl.history = malloc(rl.max * sizeof(const char *));
		memset(saved_current_y, 0, sizeof(saved_current_y));
		memset(saved_current_item_index, 0,
		       sizeof(saved_current_item_index));
		first = false;
	}
	if (current_screen == SCREEN_SYSTEM_LIST) {
		policy_file = proc_policy_system_policy;
		list_caption = "System Policy Editor";
	} else if (current_screen == SCREEN_EXCEPTION_LIST) {
		policy_file = proc_policy_exception_policy;
		list_caption = "Exception Policy Editor";
	} else if (current_screen == SCREEN_ACL_LIST) {
		policy_file = proc_policy_domain_policy;
		list_caption = "Domain Policy Editor";
	} else if (current_screen == SCREEN_QUERY_LIST) {
		policy_file = proc_policy_query;
		list_caption = "Interactive Enforcing Mode";
	} else if (current_screen == SCREEN_PROFILE_LIST) {
		policy_file = proc_policy_profile;
		list_caption = "Profile Editor";
	} else if (current_screen == SCREEN_MANAGER_LIST) {
		policy_file = proc_policy_manager;
		list_caption = "Manager Policy Editor";
	} else if (current_screen == SCREEN_MEMINFO_LIST) {
		policy_file = proc_policy_meminfo;
		list_caption = "Memory Quota Editor";
	} else {
		policy_file = proc_policy_domain_policy;
		/* list_caption = "Domain Transition Editor"; */
	}
	current_item_index[current_screen]
		= saved_current_item_index[current_screen];
	current_y[current_screen] = saved_current_y[current_screen];
start:
	if (current_screen == SCREEN_DOMAIN_LIST) {
		read_domain_and_exception_policy(dp);
		adjust_cursor_pos(dp->list_len);
	} else {
		read_generic_policy();
		adjust_cursor_pos(generic_acl_list_count);
	}
start2:
	show_list(dp);
	if (rl.last_error && current_screen == SCREEN_DOMAIN_LIST) {
		move(1, 0);
		printw("ERROR: %s", rl.last_error);
		clrtoeol();
		refresh();
		free(rl.last_error);
		rl.last_error = NULL;
	}
	while (true) {
		const int current = get_current();
		const int c = getch2();
		saved_current_item_index[current_screen]
			= current_item_index[current_screen];
		saved_current_y[current_screen] = current_y[current_screen];
		if (c == 'q' || c == 'Q')
			return MAXSCREEN;
		if ((c == '\r' || c == '\n') &&
		    current_screen == SCREEN_ACL_LIST)
			return SCREEN_DOMAIN_LIST;
		if (c == '\t') {
			if (current_screen == SCREEN_DOMAIN_LIST)
				return SCREEN_SYSTEM_LIST;
			else if (current_screen == SCREEN_SYSTEM_LIST)
				return SCREEN_EXCEPTION_LIST;
			else
				return SCREEN_DOMAIN_LIST;
		}
		if (c == ERR)
			continue; /* Ignore invalid key. */
		switch (c) {
			int index;
			const char *line;
		case KEY_RESIZE:
			resize_window();
			show_list(dp);
			break;
		case KEY_UP:
			up_arrow_key(dp);
			break;
		case KEY_DOWN:
			down_arrow_key(dp);
			break;
		case KEY_PPAGE:
			page_up_key(dp);
			break;
		case KEY_NPAGE:
			page_down_key(dp);
			break;
		case ' ':
			select_item(dp, current);
			break;
		case 'c':
		case 'C':
			if (current == EOF)
				break;
			if (current_screen == SCREEN_DOMAIN_LIST) {
				const u8 selected
					= dp->list_selected[current];
				if (is_deleted_domain(dp, current) ||
				    is_initializer_source(dp, current))
					break;
				for (index = current;
				     index < dp->list_len; index++) {
					if (is_deleted_domain(dp, index) ||
					    is_initializer_source(dp, index))
						continue;
					dp->list_selected[index]
						= selected;
				}
			} else {
				const u8 selected
					= generic_acl_list[current].selected;
				for (index = current;
				     index < generic_acl_list_count; index++) {
					generic_acl_list[index].selected
						= selected;
				}
			}
			show_list(dp);
			break;
		case 'f':
		case 'F':
			if (current_screen != SCREEN_MEMINFO_LIST)
				find_entry(dp, true, true, current, &rl);
			break;
		case 'p':
		case 'P':
			if (current_screen == SCREEN_MEMINFO_LIST)
				break;
			if (!rl.search_buffer[current_screen])
				find_entry(dp, true, false, current, &rl);
			else
				find_entry(dp, false, false, current, &rl);
			break;
		case 'n':
		case 'N':
			if (current_screen == SCREEN_MEMINFO_LIST)
				break;
			if (!rl.search_buffer[current_screen])
				find_entry(dp, true, true, current, &rl);
			else
				find_entry(dp, false, true, current, &rl);
			break;
		case 'd':
		case 'D':
			switch (current_screen) {
			case SCREEN_SYSTEM_LIST:
			case SCREEN_EXCEPTION_LIST:
			case SCREEN_ACL_LIST:
			case SCREEN_DOMAIN_LIST:
			case SCREEN_PROFILE_LIST:
			case SCREEN_MANAGER_LIST:
				delete_entry(dp, current);
				goto start;
			}
			break;
		case 'a':
		case 'A':
			switch (current_screen) {
			case SCREEN_SYSTEM_LIST:
			case SCREEN_EXCEPTION_LIST:
			case SCREEN_ACL_LIST:
			case SCREEN_DOMAIN_LIST:
			case SCREEN_PROFILE_LIST:
			case SCREEN_MANAGER_LIST:
				add_entry(&rl);
				goto start;
			}
			break;
		case '\r':
		case '\n':
			if (current_screen != SCREEN_DOMAIN_LIST)
				break;
			if (is_initializer_source(dp, current)) {
				int redirect_index;
				get();
				shprintf(ROOT_NAME "%s",
					 strrchr(domain_name(dp, current),
						 ' '));
				redirect_index = find_domain(dp, shared_buffer,
							     false, false);
				put();
				if (redirect_index == EOF)
					break;
				current_item_index[current_screen]
					= redirect_index
					- current_y[current_screen];
				while (current_item_index[current_screen] < 0) {
					current_item_index[current_screen]++;
					current_y[current_screen]--;
				}
				show_list(dp);
			} else if (!is_deleted_domain(dp, current)) {
				free(current_domain);
				current_domain = strdup(domain_name(dp,
								    current));
				if (!current_domain)
					out_of_memory();
				return SCREEN_ACL_LIST;
			}
			break;
		case 's':
		case 'S':
			switch (current_screen) {
			case SCREEN_DOMAIN_LIST:
				set_profile(dp, current);
				goto start;
			case SCREEN_PROFILE_LIST:
				set_level(dp, current);
				goto start;
			case SCREEN_MEMINFO_LIST:
				set_quota(dp, current);
				goto start;
			}
			break;
		case 'r':
		case 'R':
			goto start;
		case KEY_LEFT:
			if (!max_eat_col[current_screen])
				break;
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
			if (current == EOF)
				break;
			get();
			switch (current_screen) {
				u8 directive;
			case SCREEN_DOMAIN_LIST:
				line = domain_name(dp, current);
				break;
			case SCREEN_SYSTEM_LIST:
			case SCREEN_EXCEPTION_LIST:
			case SCREEN_ACL_LIST:
				directive = generic_acl_list[current].directive;
				shprintf("%s %s", directives[directive].alias,
					 generic_acl_list[current].operand);
				line = shared_buffer;
				break;
			case SCREEN_MEMINFO_LIST:
				line = NULL;
				break;
			default:
				shprintf("%s",
					 generic_acl_list[current].operand);
				line = shared_buffer;
			}
			rl.count = simple_add_history(line, rl.history,
						      rl.count, rl.max);
			put();
			break;
		case 'o':
		case 'O':
			if (current_screen == SCREEN_ACL_LIST)
				try_optimize(dp, current);
			break;
		case '@':
			if (current_screen != SCREEN_ACL_LIST)
				break;
			sort_type = (sort_type + 1) % 2;
			goto start;
		case 'w':
		case 'W':
			return select_window(dp, current);
		case '?':
			show_command_key();
			goto start;
		}
	}
}

static void handle_domain_policy(struct domain_policy *dp, FILE *fp,
				 _Bool is_write)
{
	int i;
	int index = EOF;
	if (!is_write)
		goto read_policy;
	while (freadline(fp)) {
		_Bool is_delete = false;
		_Bool is_select = false;
		unsigned int profile;
		if (str_starts(shared_buffer, "delete "))
			is_delete = true;
		else if (str_starts(shared_buffer, "select "))
			is_select = true;
		if (is_domain_def(shared_buffer)) {
			if (is_delete) {
				index = find_domain(dp, shared_buffer, false,
						    false);
				if (index >= 0)
					delete_domain(dp, index);
				index = EOF;
				continue;
			}
			if (is_select) {
				index = find_domain(dp, shared_buffer, false,
						    false);
				continue;
			}
			index = find_or_assign_new_domain(dp, shared_buffer,
							  false, false);
			continue;
		}
		if (index == EOF || !shared_buffer[0])
			continue;
		if (sscanf(shared_buffer, KEYWORD_USE_PROFILE "%u", &profile)
		    == 1)
			dp->list[index].profile = (u8) profile;
		else if (is_delete)
			del_string_entry(dp, shared_buffer, index);
		else
			add_string_entry(dp, shared_buffer, index);
	}
	return;
read_policy:
	for (i = 0; i < dp->list_len; i++) {
		int j;
		const struct path_info **string_ptr
			= dp->list[i].string_ptr;
		const int string_count = dp->list[i].string_count;
		fprintf(fp, "%s\n" KEYWORD_USE_PROFILE "%u\n\n",
			domain_name(dp, i), dp->list[i].profile);
		for (j = 0; j < string_count; j++)
			fprintf(fp, "%s\n", string_ptr[j]->name);
		fprintf(fp, "\n");
	}
}

struct misc_policy {
	const struct path_info **list;
	int list_len;
};

static void handle_misc_policy(struct misc_policy *mp, FILE *fp, _Bool is_write)
{
	int i;
	if (!is_write)
		goto read_policy;
	while (freadline(fp)) {
		const struct path_info *cp;
		_Bool is_delete;
		if (!shared_buffer[0])
			continue;
		is_delete = str_starts(shared_buffer, "delete ");
		cp = savename(shared_buffer);
		if (!cp)
			out_of_memory();
		if (!is_delete)
			goto append_policy;
		for (i = 0; i < mp->list_len; i++)
			/* Faster comparison, for they are savename'd. */
			if (mp->list[i] == cp)
				break;
		if (i < mp->list_len)
			for (mp->list_len--; i < mp->list_len; i++)
				mp->list[i] = mp->list[i + 1];
		continue;
append_policy:
		for (i = 0; i < mp->list_len; i++)
			/* Faster comparison, for they are savename'd. */
			if (mp->list[i] == cp)
				break;
		if (i < mp->list_len)
			continue;
		mp->list = realloc(mp->list, (mp->list_len + 1)
				   * sizeof(const struct path_info *));
		if (!mp->list)
			out_of_memory();
		mp->list[mp->list_len++] = cp;
	}
	return;
read_policy:
	for (i = 0; i < mp->list_len; i++)
		fprintf(fp, "%s\n", mp->list[i]->name);
}

static void policy_daemon(void)
{
	struct misc_policy mp[4];
	struct domain_policy dp;
	memset(&dp, 0, sizeof(dp));
	memset(&mp, 0, sizeof(mp));
	get();
	find_or_assign_new_domain(&dp, ROOT_NAME, false, false);
	while (true) {
		FILE *fp;
		struct msghdr msg;
		struct iovec iov = { shared_buffer, sizeof(shared_buffer) - 1 };
		char cmsg_buf[CMSG_SPACE(sizeof(int))];
		struct cmsghdr *cmsg = (struct cmsghdr *) cmsg_buf;
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_control = cmsg_buf;
		msg.msg_controllen = sizeof(cmsg_buf);
		memset(shared_buffer, 0, sizeof(shared_buffer));
		errno = 0;
		if (recvmsg(persistent_fd, &msg, 0) <= 0)
			break;
		cmsg = CMSG_FIRSTHDR(&msg);
		if (!cmsg)
			break;
		if (cmsg->cmsg_level == SOL_SOCKET &&
		    cmsg->cmsg_type == SCM_RIGHTS &&
		    cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
			const int fd = *(int *) CMSG_DATA(cmsg);
			fp = fdopen(fd, "w+");
			if (!fp) {
				close(fd);
				continue;
			}
		} else {
			break;
		}
		if (str_starts(shared_buffer, "POST ")) {
			if (!strcmp(shared_buffer, proc_policy_domain_policy))
				handle_domain_policy(&dp, fp, true);
			else if (!strcmp(shared_buffer,
					 proc_policy_exception_policy))
				handle_misc_policy(&mp[0], fp, true);
			else if (!strcmp(shared_buffer,
					 proc_policy_system_policy))
				handle_misc_policy(&mp[1], fp, true);
			else if (!strcmp(shared_buffer, proc_policy_profile))
				handle_misc_policy(&mp[2], fp, true);
			else if (!strcmp(shared_buffer, proc_policy_manager))
				handle_misc_policy(&mp[3], fp, true);
		} else if (str_starts(shared_buffer, "GET ")) {
			if (!strcmp(shared_buffer, proc_policy_domain_policy))
				handle_domain_policy(&dp, fp, false);
			else if (!strcmp(shared_buffer,
					 proc_policy_exception_policy))
				handle_misc_policy(&mp[0], fp, false);
			else if (!strcmp(shared_buffer,
					 proc_policy_system_policy))
				handle_misc_policy(&mp[1], fp, false);
			else if (!strcmp(shared_buffer, proc_policy_profile))
				handle_misc_policy(&mp[2], fp, false);
			else if (!strcmp(shared_buffer, proc_policy_manager))
				handle_misc_policy(&mp[3], fp, false);
		}
		fclose(fp);
	}
	put();
	clear_domain_policy(&dp);
	{
		int i;
		for (i = 0; i < 3; i++) {
			free(mp[i].list);
			mp[i].list = NULL;
			mp[i].list_len = 0;
		}
	}
	_exit(0);
}

static void init_keyword_map(void);

static void copy_fd_to_fp(int fd, FILE *fp)
{
	char buffer[1024];
	while (true) {
		const int len = read(fd, buffer, sizeof(buffer));
		if (len <= 0)
			break;
		fwrite(buffer, len, 1, fp);
	}
}

int editpolicy_main(int argc, char *argv[])
{
	struct domain_policy dp = { NULL, 0, NULL };
	struct domain_policy bp = { NULL, 0, NULL };
	memset(current_y, 0, sizeof(current_y));
	memset(current_item_index, 0, sizeof(current_item_index));
	memset(list_item_count, 0, sizeof(list_item_count));
	memset(max_eat_col, 0, sizeof(max_eat_col));
	if (argc > 1) {
		if (!strcmp(argv[1], "s"))
			current_screen = SCREEN_SYSTEM_LIST;
		else if (!strcmp(argv[1], "e"))
			current_screen = SCREEN_EXCEPTION_LIST;
		else if (!strcmp(argv[1], "d"))
			current_screen = SCREEN_DOMAIN_LIST;
		else if (!strcmp(argv[1], "p"))
			current_screen = SCREEN_PROFILE_LIST;
		else if (!strcmp(argv[1], "m"))
			current_screen = SCREEN_MANAGER_LIST;
		else {
			printf("Usage: %s [s|e|d|p|m]\n", argv[0]);
			return 1;
		}
	}
	init_keyword_map();
	{
		char *cp = strrchr(argv[0], '/');
		if (!cp)
			cp = argv[0];
		else
			cp++;
		if (strstr(cp, "editpolicy_offline"))
			offline_mode = true;
	}
	if (offline_mode) {
		int fd[2] = { EOF, EOF };
		if (chdir(disk_policy_dir)) {
			printf("Directory %s doesn't exist.\n",
			       disk_policy_dir);
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
			int fd = open(base_policy_system_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
					open_write(proc_policy_system_policy);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(disk_policy_system_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
					open_write(proc_policy_system_policy);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(base_policy_exception_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
				open_write(proc_policy_exception_policy);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(disk_policy_exception_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
				open_write(proc_policy_exception_policy);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(base_policy_domain_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
					open_write(proc_policy_domain_policy);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(disk_policy_domain_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
					open_write(proc_policy_domain_policy);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(base_policy_profile, O_RDONLY);
			if (fd != EOF) {
				FILE *fp = open_write(proc_policy_profile);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(disk_policy_profile, O_RDONLY);
			if (fd != EOF) {
				FILE *fp = open_write(proc_policy_profile);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(base_policy_manager, O_RDONLY);
			if (fd != EOF) {
				FILE *fp = open_write(proc_policy_manager);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
			fd = open(disk_policy_manager, O_RDONLY);
			if (fd != EOF) {
				FILE *fp = open_write(proc_policy_manager);
				copy_fd_to_fp(fd, fp);
				fclose(fp);
				close(fd);
			}
		}
	} else {
		if (chdir(proc_policy_dir)) {
			fprintf(stderr,
				"You can't use this editor for this kernel.\n");
			return 1;
		}
		{
			const int fd1 = open(proc_policy_system_policy, O_RDWR);
			const int fd2 = open(proc_policy_exception_policy,
					     O_RDWR);
			const int fd3 = open(proc_policy_domain_policy, O_RDWR);
			if ((fd1 != EOF && write(fd1, "", 0) != 0) ||
			    (fd2 != EOF && write(fd2, "", 0) != 0) ||
			    (fd3 != EOF && write(fd3, "", 0) != 0)) {
				fprintf(stderr,
					"You need to register this program to "
					"%s to run this program.\n",
					proc_policy_manager);
				return 1;
			}
			close(fd1);
			close(fd2);
			close(fd3);
		}
	}
	initscr();
	editpolicy_color_init();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, window_height, window_width);
	while (current_screen < MAXSCREEN) {
		if (!offline_mode) {
			if (current_screen == SCREEN_DOMAIN_LIST &&
			    access(proc_policy_domain_policy, F_OK))
				current_screen = SCREEN_SYSTEM_LIST;
			else if (current_screen == SCREEN_SYSTEM_LIST &&
				 access(proc_policy_system_policy, F_OK))
				current_screen = SCREEN_EXCEPTION_LIST;
			else if (current_screen == SCREEN_EXCEPTION_LIST &&
				 access(proc_policy_exception_policy, F_OK)) {
				current_screen = SCREEN_DOMAIN_LIST;
				if (access(proc_policy_domain_policy, F_OK))
					current_screen = SCREEN_SYSTEM_LIST;
			}
		}
		resize_window();
		current_screen = generic_list_loop(&dp);
	}
	clear();
	move(0, 0);
	refresh();
	endwin();
	if (offline_mode) {
		time_t now = time(NULL);
		char *filename = make_filename("system_policy", now);
		if (move_proc_to_file(proc_policy_system_policy,
				      base_policy_system_policy, filename)) {
			if (is_identical_file("system_policy.conf", filename)) {
				unlink(filename);
			} else {
				unlink("system_policy.conf");
				symlink(filename, "system_policy.conf");
			}
		}
		filename = make_filename("exception_policy", now);
		if (move_proc_to_file(proc_policy_exception_policy,
				      base_policy_exception_policy, filename)) {
			if (is_identical_file("exception_policy.conf",
					      filename)) {
				unlink(filename);
			} else {
				unlink("exception_policy.conf");
				symlink(filename, "exception_policy.conf");
			}
		}
		clear_domain_policy(&dp);
		filename = make_filename("domain_policy", now);
		if (save_domain_policy_with_diff(&dp, &bp,
						 proc_policy_domain_policy,
						 base_policy_domain_policy,
						 filename)) {
			if (is_identical_file("domain_policy.conf", filename)) {
				unlink(filename);
			} else {
				unlink("domain_policy.conf");
				symlink(filename, "domain_policy.conf");
			}
		}
		filename = make_filename("profile", now);
		if (move_proc_to_file(proc_policy_profile, base_policy_profile,
				      filename)) {
			if (is_identical_file("profile.conf", filename)) {
				unlink(filename);
			} else {
				unlink("profile.conf");
				symlink(filename, "profile.conf");
			}
		}
		filename = make_filename("manager", now);
		if (move_proc_to_file(proc_policy_manager, base_policy_manager,
				      filename)) {
			if (is_identical_file("manager.conf", filename)) {
				unlink(filename);
			} else {
				unlink("manager.conf");
				symlink(filename, "manager.conf");
			}
		}
	}
	clear_domain_policy(&bp);
	clear_domain_policy(&dp);
	return 0;
}

/* keyword mapping */

static u8 find_directive(const _Bool forward, char *line)
{
	u8 i;
	for (i = 1; i < MAX_DIRECTIVE_INDEX; i++) {
		if (forward) {
			const int len = directives[i].original_len;
			if (strncmp(line, directives[i].original, len) ||
			    (line[len] != ' ' && line[len]))
				continue;
			if (line[len])
				memmove(line, line + len + 1,
					strlen(line + len + 1) + 1);
			else
				line[0] = '\0';
			return i;
		} else {
			const int len = directives[i].alias_len;
			if (strncmp(line, directives[i].alias, len) ||
			    (line[len] != ' ' && line[len]))
				continue;
			if (line[len])
				memmove(line, line + len + 1,
					strlen(line + len + 1) + 1);
			else
				line[0] = '\0';
			return i;
		}
	}
	return DIRECTIVE_NONE;
}

static void init_keyword_map(void)
{
	FILE *fp = fopen(CCSTOOLS_CONFIG_FILE, "r");
	int i;
	if (!fp)
		goto use_default;
	get();
	while (freadline(fp)) {
		char *cp = shared_buffer + 25;
		if (strncmp(shared_buffer, "editpolicy.keyword_alias ", 25))
			continue;
		memmove(shared_buffer, cp, strlen(cp) + 1);
		cp = strchr(shared_buffer, '=');
		if (!cp)
			continue;
		*cp++ = '\0';
		normalize_line(shared_buffer);
		normalize_line(cp);
		if (!*shared_buffer || !*cp)
			continue;
		for (i = 1; i < MAX_DIRECTIVE_INDEX; i++) {
			if (strcmp(shared_buffer, directives[i].original))
				continue;
			free((void *) directives[i].alias);
			cp = strdup(cp);
			if (!cp)
				out_of_memory();
			directives[i].alias = cp;
			directives[i].alias_len = strlen(cp);
			break;
		}
	}
	put();
	fclose(fp);
use_default:
	for (i = 1; i < MAX_DIRECTIVE_INDEX; i++) {
		if (!directives[i].alias)
			directives[i].alias = directives[i].original;
		directives[i].original_len = strlen(directives[i].original);
		directives[i].alias_len = strlen(directives[i].alias);
	}
}

/***** editpolicy end *****/
