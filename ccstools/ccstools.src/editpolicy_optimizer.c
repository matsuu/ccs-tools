/*
 * editpolicy_optimizer.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-rc   2009/03/04
 *
 */
#include "ccstools.h"

/* Prototypes */
static int parse_ip(const char *address, struct ip_address_entry *entry);
static int add_address_group_entry(const char *group_name,
				   const char *member_name,
				   const _Bool is_delete);
static struct address_group_entry *find_address_group(const char *group_name);
static _Bool compare_path(struct path_info *sarg, struct path_info *darg,
			  u8 directive);
static _Bool compare_address(struct path_info *sarg, struct path_info *darg);
static u8 split_acl(const u8 index, char *data, struct path_info *arg1,
		    struct path_info *arg2, struct path_info *arg3);

/* Utility functions */

struct path_group_entry *find_path_group(const char *group_name)
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

int add_address_group_policy(char *data, const _Bool is_delete)
{
	char *cp = strchr(data, ' ');
	if (!cp)
		return -EINVAL;
	*cp++ = '\0';
	return add_address_group_entry(data, cp, is_delete);
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


void editpolicy_try_optimize(struct domain_policy *dp, const int current,
			     const int screen)
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
	for (index = 0; index < list_item_count[screen]; index++) {
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
		shprintf("%s", generic_acl_list[index].operand);
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
		case DIRECTIVE_ALLOW_IOCTL:
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
		case DIRECTIVE_ALLOW_IOCTL:
			/* Ioctl command number component. */
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
}

/* Variables */

static struct address_group_entry *address_group_list = NULL;
int address_group_list_len = 0;

/* Main functions */

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

static struct address_group_entry *find_address_group(const char *group_name)
{
	int i;
	for (i = 0; i < address_group_list_len; i++) {
		if (!strcmp(group_name, address_group_list[i].group_name->name))
			return &address_group_list[i];
	}
	return NULL;
}
