/*
 * editpolicy.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-pre   2009/02/24
 *
 */
#include "ccstools.h"

struct readline_data {
	const char **history;
	int count;
	int max;
	char *search_buffer[MAXSCREEN];
};

/* Prototypes */

static void sigalrm_handler(int sig);
static const char *get_last_name(const struct domain_policy *dp,
				 const int index);
static _Bool is_keeper_domain(struct domain_policy *dp, const int index);
static _Bool is_initializer_source(struct domain_policy *dp, const int index);
static _Bool is_initializer_target(struct domain_policy *dp, const int index);
static _Bool is_domain_unreachable(struct domain_policy *dp, const int index);
static _Bool is_deleted_domain(struct domain_policy *dp, const int index);
static const struct domain_keeper_entry *
is_domain_keeper(const struct path_info *domainname, const char *program);
static const struct domain_initializer_entry *
is_domain_initializer(const struct path_info *domainname, const char *program);
static FILE *open_write(const char *filename);
static int generic_acl_compare(const void *a, const void *b);
static int generic_acl_compare0(const void *a, const void *b);
static int string_acl_compare(const void *a, const void *b);
static int profile_entry_compare(const void *a, const void *b);
static void read_generic_policy(void);
static int add_domain_initializer_entry(const char *domainname,
					const char *program,
					const _Bool is_not);
static int add_domain_initializer_policy(char *data, const _Bool is_not);
static int add_domain_keeper_entry(const char *domainname, const char *program,
				   const _Bool is_not);
static int add_domain_keeper_policy(char *data, const _Bool is_not);
static int add_path_group_entry(const char *group_name, const char *member_name,
				const _Bool is_delete);
static int add_path_group_policy(char *data, const _Bool is_delete);
static void assign_domain_initializer_source(struct domain_policy *dp,
					     const struct path_info *domainname,
					     const char *program);
static int domainname_attribute_compare(const void *a, const void *b);
static void read_domain_and_exception_policy(struct domain_policy *dp);
static void show_current(struct domain_policy *dp);
static const char *eat(const char *str);
static int show_domain_line(struct domain_policy *dp, int index);
static int show_acl_line(int index, int list_indent);
static int show_profile_line(int index);
static int show_literal_line(int index);
static int show_meminfo_line(int index);
static void show_list(struct domain_policy *dp);
static void resize_window(void);
static void up_arrow_key(struct domain_policy *dp);
static void down_arrow_key(struct domain_policy *dp);
static void page_up_key(struct domain_policy *dp);
static void page_down_key(struct domain_policy *dp);
static void show_current(struct domain_policy *dp);
static void adjust_cursor_pos(const int item_count);
static void set_cursor_pos(const int index);
static int count(const unsigned char *array, const int len);
static int count2(const struct generic_acl *array, int len);
static int select_item(struct domain_policy *dp, const int current);
static int generic_acl_compare(const void *a, const void *b);
static void delete_entry(struct domain_policy *dp, int current);
static void add_entry(struct readline_data *rl);
static void find_entry(struct domain_policy *dp, _Bool input, _Bool forward,
		       int current, struct readline_data *rl);
static void set_profile(struct domain_policy *dp, int current);
static void set_level(struct domain_policy *dp, int current);
static void set_quota(struct domain_policy *dp, int current);
static int select_window(struct domain_policy *dp, const int current);
static void show_command_key(const int screen, const _Bool readonly);
static int generic_list_loop(struct domain_policy *dp);
static void copy_fd_to_fp(int fd, FILE *fp);

/* Utility Functions */

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

static const char *get_last_name(const struct domain_policy *dp,
				 const int index)
{
	const char *cp0 = domain_name(dp, index);
	const char *cp1 = strrchr(cp0, ' ');
	if (cp1)
		return cp1 + 1;
	return cp0;
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

int add_string_entry(struct domain_policy *dp, const char *entry,
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

int del_string_entry(struct domain_policy *dp, const char *entry,
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

int find_domain(struct domain_policy *dp, const char *domainname0,
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

int find_or_assign_new_domain(struct domain_policy *dp, const char *domainname,
			      const _Bool is_dis, const _Bool is_dd)
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
	const char *a1 = a0->operand;
	const char *b1 = b0->operand;
	return strcmp(a1, b1);
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

static int add_path_group_policy(char *data, const _Bool is_delete)
{
	char *cp = strchr(data, ' ');
	if (!cp)
		return -EINVAL;
	*cp++ = '\0';
	return add_path_group_entry(data, cp, is_delete);
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

static int show_profile_line(int index)
{
	const char *cp = generic_acl_list[index].operand;
	const u8 profile = generic_acl_list[index].directive;
	char number[8];
	snprintf(number, sizeof(number) - 1, "%3u-", profile);
	printw("%c%4d: %s", generic_acl_list[index].selected ? '&' : ' ',
	       index, eat(number));
	printw("%s ", eat(cp));
	return strlen(number) + strlen(cp) + 8;
}

static int show_literal_line(int index)
{
	const char *cp = generic_acl_list[index].operand;
	printw("%c%4d: %s ",
	       generic_acl_list[index].selected ? '&' : ' ',
	       index, eat(cp));
	return strlen(cp) + 8;
}

static int show_meminfo_line(int index)
{
	unsigned int now = 0;
	unsigned int quota = 0;
	const char *data = generic_acl_list[index].operand;
	get();
	if (sscanf(data, "Shared: %u (Quota: %u)", &now, &quota) >= 1)
		shprintf("Memory for string data    = %10u bytes    "
			 "Quota = %10u bytes",
			 now, quota ? quota : -1);
	else if (sscanf(data, "Private: %u (Quota: %u)", &now, &quota) >= 1)
		shprintf("Memory for numeric data   = %10u bytes    "
			 "Quota = %10u bytes",
			 now, quota ? quota : -1);
	else if (sscanf(data, "Dynamic: %u (Quota: %u)", &now, &quota) >= 1)
		shprintf("Memory for temporary data = %10u bytes    "
			 "Quota = %10u bytes",
			 now, quota ? quota : -1);
	else if (sscanf(data, "Total: %u", &now) == 1)
		shprintf("Total memory in use       = %10u bytes", now);
	else
		shprintf("%s", data);
	if (shared_buffer[0])
		printw("%s", eat(shared_buffer));
	now = strlen(shared_buffer);
	put();
	return now;
}

static void show_command_key(const int screen, const _Bool readonly)
{
	int c;
	clear();
	printw("Commands available for this screen are:\n\n");
	printw("Q/q        Quit this editor.\n");
	printw("R/r        Refresh to the latest information.\n");
	switch (screen) {
	case SCREEN_MEMINFO_LIST:
		break;
	default:
		printw("F/f        Find first.\n");
		printw("N/n        Find next.\n");
		printw("P/p        Find previous.\n");
	}
	printw("W/w        Switch to selected screen.\n");
	/* printw("Tab        Switch to next screen.\n"); */
	switch (screen) {
	case SCREEN_MEMINFO_LIST:
		break;
	default:
		printw("Insert     Copy an entry at the cursor position to "
		       "history buffer.\n");
		printw("Space      Invert selection state of an entry at "
		       "the cursor position.\n");
		printw("C/c        Copy selection state of an entry at "
		       "the cursor position to all entries below the cursor "
		       "position.\n");
	}
	switch (screen) {
	case SCREEN_DOMAIN_LIST:
		if (!readonly)
			printw("A/a        Add a new domain.\n");
		printw("Enter      Edit ACLs of a domain at the cursor "
		       "position.\n");
		if (!readonly) {
			printw("D/d        Delete selected domains.\n");
			printw("S/s        Set profile number of selected "
			       "domains.\n");
		}
		break;
	case SCREEN_MEMINFO_LIST:
		if (!readonly)
			printw("S/s        Set memory quota of selected "
			       "items.\n");
		break;
	case SCREEN_PROFILE_LIST:
		if (!readonly)
			printw("S/s        Set mode of selected items.\n");
		break;
	}
	switch (screen) {
	case SCREEN_SYSTEM_LIST:
	case SCREEN_EXCEPTION_LIST:
	case SCREEN_ACL_LIST:
	case SCREEN_MANAGER_LIST:
		if (!readonly) {
			printw("A/a        Add a new entry.\n");
			printw("D/d        Delete selected entries.\n");
		}
	}
	switch (screen) {
	case SCREEN_PROFILE_LIST:
		if (!readonly)
			printw("A/a        Define a new profile.\n");
	}
	switch (screen) {
	case SCREEN_ACL_LIST:
		printw("O/o        Set selection state to other entries "
		       "included in an entry at the cursor position.\n");
		/* Fall through. */
	case SCREEN_PROFILE_LIST:
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

/* Variables */

static _Bool readonly_mode = false;
static unsigned int refresh_interval = 0;
static _Bool need_reload = false;

_Bool offline_mode = false;

struct path_group_entry *path_group_list = NULL;
int path_group_list_len = 0;

static const char *policy_file = NULL;
static const char *list_caption = NULL;
static char *current_domain = NULL;

static int current_screen = SCREEN_DOMAIN_LIST;

struct generic_acl *generic_acl_list = NULL;
int generic_acl_list_count = 0;

static struct domain_keeper_entry *domain_keeper_list = NULL;
static int domain_keeper_list_len = 0;
static struct domain_initializer_entry *domain_initializer_list = NULL;
static int domain_initializer_list_len = 0;

static int profile_sort_type = 0;
static int unnumbered_domain_count = 0;

static int window_width = 0;
static int window_height = 0;
static int current_item_index[MAXSCREEN];
int current_y[MAXSCREEN];
int list_item_count[MAXSCREEN];

static int body_lines = 0;

static int max_eat_col[MAXSCREEN];
static int eat_col = 0;
static int max_col = 0;
static int list_indent = 0;

static int acl_sort_type = 1;

static char *last_error = NULL;

/* Main Functions */

static void set_error(const char *filename)
{
	if (filename) {
		const int len = strlen(filename) + 128;
		last_error = realloc(last_error, len);
		if (!last_error)
			out_of_memory();
		memset(last_error, 0, len);
		snprintf(last_error, len - 1, "Can't open %s .", filename);
	} else {
		free(last_error);
		last_error = NULL;
	}
}

static int open2(const char *filename, int mode)
{
	const int fd = open(filename, mode);
	if (fd == EOF && errno != ENOENT)
		set_error(filename);
	return fd;
}

static void sigalrm_handler(int sig)
{
	need_reload = true;
	alarm(refresh_interval);
}

static const char *eat(const char *str)
{
	while (*str && eat_col) {
		str++;
		eat_col--;
	}
	return str;
}

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
		FILE *fp;
		if (readonly_mode)
			return NULL;
		fp = fdopen(open2(filename, O_WRONLY), "w");
		if (!fp)
			set_error(filename);
		return fp;
	}
}


static int profile_entry_compare(const void *a, const void *b)
{
	const struct generic_acl *a0 = (struct generic_acl *) a;
	const struct generic_acl *b0 = (struct generic_acl *) b;
	const char *a1 = a0->operand;
	const char *b1 = b0->operand;
	const int a2 = a0->directive;
	const int b2 = b0->directive;
	if (profile_sort_type == 0) {
		if (a2 == b2)
			return strcmp(a1, b1);
		else
			return a2 - b2;
	} else {
		const int a3 = strcspn(a1, "=");
		const int b3 = strcspn(b1, "=");
		const int c = strncmp(a1, b1, a3 >= b3 ? b3 : a3);
		if (c)
			return c;
		if (a3 != b3)
			return a3 - b3;
		else
			return a2 - b2;
	}
}

static void read_generic_policy(void)
{
	FILE *fp = NULL;
	_Bool flag = false;
	while (generic_acl_list_count)
		free((void *)
		     generic_acl_list[--generic_acl_list_count].operand);
	if (!offline_mode && current_screen == SCREEN_ACL_LIST) {
		/* Don't set error message if failed. */
		fp = fopen(policy_file, "r+");
		if (fp) {
			fprintf(fp, "select domain=%s\n", current_domain);
			fflush(fp);
		}
	}
	if (!fp)
		fp = open_read(policy_file);
	if (!fp) {
		set_error(policy_file);
		return;
	}
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
		case SCREEN_PROFILE_LIST:
			cp = strchr(shared_buffer, '-');
			if (!cp)
				continue;
			*cp++ = '\0';
			directive = atoi(shared_buffer);
			memmove(shared_buffer, cp, strlen(cp) + 1);
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
	case SCREEN_PROFILE_LIST:
		qsort(generic_acl_list, generic_acl_list_count,
		      sizeof(struct generic_acl), profile_entry_compare);
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
	if (!fp) {
		set_error(proc_policy_exception_policy);
		goto no_exception;
	}
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
		/* Don't set error message if failed. */
		fp = fopen(policy_file, "r+");
		if (fp) {
			fprintf(fp, "select allow_execute\n");
			fflush(fp);
		}
	}
	if (!fp)
		fp = open_read(proc_policy_domain_policy);
	if (!fp) {
		set_error(proc_policy_domain_policy);
		goto no_domain;
	}
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
		case SCREEN_PROFILE_LIST:
			tmp_col = show_profile_line(index);
			break;
		case SCREEN_MEMINFO_LIST:
			tmp_col = show_meminfo_line(index);
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

int editpolicy_get_current(void)
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

static void show_current(struct domain_policy *dp)
{
	if (current_screen == SCREEN_DOMAIN_LIST) {
		get();
		eat_col = max_eat_col[current_screen];
		shprintf("%s", eat(domain_name(dp, editpolicy_get_current())));
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
	editpolicy_line_draw(current_screen);     /* add color */
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

static int generic_acl_compare(const void *a, const void *b)
{
	const struct generic_acl *a0 = (struct generic_acl *) a;
	const struct generic_acl *b0 = (struct generic_acl *) b;
	const char *a1 = directives[a0->directive].alias;
	const char *b1 = directives[b0->directive].alias;
	const char *a2 = a0->operand;
	const char *b2 = b0->operand;
	if (acl_sort_type == 0) {
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
			last_error = realloc(last_error, len);
			if (!last_error)
				out_of_memory();
			memset(last_error, 0, len);
			snprintf(last_error, len - 1,
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
		shprintf("%s", generic_acl_list[index].operand);
		cp = strchr(shared_buffer, '=');
		if (cp)
			*cp = '\0';
		fprintf(fp, "%u-%s=%s\n", generic_acl_list[index].directive,
			shared_buffer, line);
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
		shprintf("%s", generic_acl_list[index].operand);
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
	if (last_error) {
		move(1, 0);
		printw("ERROR: %s", last_error);
		clrtoeol();
		refresh();
		free(last_error);
		last_error = NULL;
	}
	while (true) {
		const int current = editpolicy_get_current();
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
		if (need_reload) {
			need_reload = false;
			goto start;
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
			if (readonly_mode)
				break;
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
			if (readonly_mode)
				break;
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
			if (readonly_mode)
				break;
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
			if (current_screen == SCREEN_ACL_LIST) {
				editpolicy_try_optimize(dp, current,
							current_screen);
				show_list(dp);
			}
			break;
		case '@':
			if (current_screen == SCREEN_ACL_LIST) {
				acl_sort_type = (acl_sort_type + 1) % 2;
				goto start;
			} else if (current_screen == SCREEN_PROFILE_LIST) {
				profile_sort_type = (profile_sort_type + 1) % 2;
				goto start;
			}
			break;
		case 'w':
		case 'W':
			return select_window(dp, current);
		case '?':
			show_command_key(current_screen, readonly_mode);
			goto start;
		}
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
		int i;
		for (i = 1; i < argc; i++) {
			if (!strcmp(argv[i], "s"))
				current_screen = SCREEN_SYSTEM_LIST;
			else if (!strcmp(argv[i], "e"))
				current_screen = SCREEN_EXCEPTION_LIST;
			else if (!strcmp(argv[i], "d"))
				current_screen = SCREEN_DOMAIN_LIST;
			else if (!strcmp(argv[i], "p"))
				current_screen = SCREEN_PROFILE_LIST;
			else if (!strcmp(argv[i], "m"))
				current_screen = SCREEN_MANAGER_LIST;
			else if (!strcmp(argv[i], "q"))
				current_screen = SCREEN_MEMINFO_LIST;
			else if (!strcmp(argv[i], "readonly"))
				readonly_mode = true;
			else if (sscanf(argv[i], "refresh=%u",
					&refresh_interval) == 1) {
				/* */
			} else {
				printf("Usage: %s [s|e|d|p|m|q] [readonly] "
				       "[refresh=interval]\n", argv[0]);
				return 1;
			}
		}
	}
	editpolicy_init_keyword_map();
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
			editpolicy_offline_daemon();
			_exit(0);
		case -1:
			fprintf(stderr, "fork()\n");
			exit(1);
		}
		close(fd[1]);
		persistent_fd = fd[0];
		{
			int fd = open2(base_policy_system_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
					open_write(proc_policy_system_policy);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(disk_policy_system_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
					open_write(proc_policy_system_policy);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(base_policy_exception_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
				open_write(proc_policy_exception_policy);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(disk_policy_exception_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
				open_write(proc_policy_exception_policy);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(base_policy_domain_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
					open_write(proc_policy_domain_policy);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(disk_policy_domain_policy, O_RDONLY);
			if (fd != EOF) {
				FILE *fp =
					open_write(proc_policy_domain_policy);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(base_policy_profile, O_RDONLY);
			if (fd != EOF) {
				FILE *fp = open_write(proc_policy_profile);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(disk_policy_profile, O_RDONLY);
			if (fd != EOF) {
				FILE *fp = open_write(proc_policy_profile);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(base_policy_manager, O_RDONLY);
			if (fd != EOF) {
				FILE *fp = open_write(proc_policy_manager);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
			fd = open2(disk_policy_manager, O_RDONLY);
			if (fd != EOF) {
				FILE *fp = open_write(proc_policy_manager);
				if (fp) {
					copy_fd_to_fp(fd, fp);
					fclose(fp);
				}
				close(fd);
			}
		}
	} else {
		if (chdir(proc_policy_dir)) {
			fprintf(stderr,
				"You can't use this editor for this kernel.\n");
			return 1;
		}
		if (!readonly_mode) {
			const int fd1 = open2(proc_policy_system_policy,
					      O_RDWR);
			const int fd2 = open2(proc_policy_exception_policy,
					      O_RDWR);
			const int fd3 = open2(proc_policy_domain_policy,
					      O_RDWR);
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
	if (refresh_interval) {
		signal(SIGALRM, sigalrm_handler);
		alarm(refresh_interval);
		timeout(1000);
	}
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
	alarm(0);
	clear();
	move(0, 0);
	refresh();
	endwin();
	if (offline_mode && !readonly_mode) {
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
