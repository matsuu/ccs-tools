/*
 * ccstools.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-rc   2009/03/16
 *
 */
#include "ccstools.h"

/* Prototypes */

static _Bool is_byte_range(const char *str);
static _Bool is_decimal(const char c);
static _Bool is_hexadecimal(const char c);
static _Bool is_alphabet_char(const char c);
static u8 make_byte(const u8 c1, const u8 c2, const u8 c3);
static inline unsigned long partial_name_hash(unsigned long c,
					      unsigned long prevhash);
static inline unsigned int full_name_hash(const unsigned char *name,
					  unsigned int len);
static void *alloc_element(const unsigned int size);
static int path_depth(const char *pathname);
static int const_part_length(const char *filename);
static int domainname_compare(const void *a, const void *b);
static int path_info_compare(const void *a, const void *b);
static void sort_domain_policy(struct domain_policy *dp);

/* Utility functions */

void out_of_memory(void)
{
	fprintf(stderr, "Out of memory. Aborted.\n");
	exit(1);
}

_Bool str_starts(char *str, const char *begin)
{
	const int len = strlen(begin);
	if (strncmp(str, begin, len))
		return false;
	memmove(str, str + len, strlen(str + len) + 1);
	return true;
}

static _Bool is_byte_range(const char *str)
{
	return *str >= '0' && *str++ <= '3' &&
		*str >= '0' && *str++ <= '7' &&
		*str >= '0' && *str <= '7';
}

static _Bool is_decimal(const char c)
{
	return c >= '0' && c <= '9';
}

static _Bool is_hexadecimal(const char c)
{
	return (c >= '0' && c <= '9') ||
		(c >= 'A' && c <= 'F') ||
		(c >= 'a' && c <= 'f');
}

static _Bool is_alphabet_char(const char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static u8 make_byte(const u8 c1, const u8 c2, const u8 c3)
{
	return ((c1 - '0') << 6) + ((c2 - '0') << 3) + (c3 - '0');
}

void normalize_line(unsigned char *line)
{
	unsigned char *sp = line;
	unsigned char *dp = line;
	_Bool first = true;
	while (*sp && (*sp <= ' ' || 127 <= *sp))
		sp++;
	while (*sp) {
		if (!first)
			*dp++ = ' ';
		first = false;
		while (' ' < *sp && *sp < 127)
			*dp++ = *sp++;
		while (*sp && (*sp <= ' ' || 127 <= *sp))
			sp++;
	}
	*dp = '\0';
}

char *make_filename(const char *prefix, const time_t time)
{
	struct tm *tm = localtime(&time);
	static char filename[1024];
	memset(filename, 0, sizeof(filename));
	snprintf(filename, sizeof(filename) - 1,
		 "%s.%02d-%02d-%02d.%02d:%02d:%02d.conf",
		 prefix, tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday,
		 tm->tm_hour, tm->tm_min, tm->tm_sec);
	return filename;
}

/* Copied from kernel source. */
static inline unsigned long partial_name_hash(unsigned long c,
					      unsigned long prevhash)
{
	return (prevhash + (c << 4) + (c >> 4)) * 11;
}

/* Copied from kernel source. */
static inline unsigned int full_name_hash(const unsigned char *name,
					  unsigned int len)
{
	unsigned long hash = 0;
	while (len--)
		hash = partial_name_hash(*name++, hash);
	return (unsigned int) hash;
}

static void *alloc_element(const unsigned int size)
{
	static char *buf = NULL;
	static unsigned int buf_used_len = PAGE_SIZE;
	char *ptr = NULL;
	if (size > PAGE_SIZE)
		return NULL;
	if (buf_used_len + size > PAGE_SIZE) {
		ptr = malloc(PAGE_SIZE);
		if (!ptr)
			out_of_memory();
		buf = ptr;
		memset(buf, 0, PAGE_SIZE);
		buf_used_len = size;
		ptr = buf;
	} else if (size) {
		int i;
		ptr = buf + buf_used_len;
		buf_used_len += size;
		for (i = 0; i < size; i++)
			if (ptr[i])
				out_of_memory();
	}
	return ptr;
}

static int path_depth(const char *pathname)
{
	int i = 0;
	if (pathname) {
		char *ep = strchr(pathname, '\0');
		if (pathname < ep--) {
			if (*ep != '/')
				i++;
			while (pathname <= ep)
				if (*ep-- == '/')
					i += 2;
		}
	}
	return i;
}

static int const_part_length(const char *filename)
{
	int len = 0;
	if (filename) {
		while (true) {
			char c = *filename++;
			if (!c)
				break;
			if (c != '\\') {
				len++;
				continue;
			}
			c = *filename++;
			switch (c) {
			case '\\':  /* "\\" */
				len += 2;
				continue;
			case '0':   /* "\ooo" */
			case '1':
			case '2':
			case '3':
				c = *filename++;
				if (c < '0' || c > '7')
					break;
				c = *filename++;
				if (c < '0' || c > '7')
					break;
				len += 4;
				continue;
			}
			break;
		}
	}
	return len;
}

_Bool is_domain_def(const unsigned char *domainname)
{
	return !strncmp(domainname, ROOT_NAME, ROOT_NAME_LEN) &&
		(domainname[ROOT_NAME_LEN] == '\0'
		 || domainname[ROOT_NAME_LEN] == ' ');
}

_Bool is_correct_domain(const unsigned char *domainname)
{
	if (!domainname || strncmp(domainname, ROOT_NAME, ROOT_NAME_LEN))
		goto out;
	domainname += ROOT_NAME_LEN;
	if (!*domainname)
		return true;
	do {
		if (*domainname++ != ' ')
			goto out;
		if (*domainname++ != '/')
			goto out;
		while (true) {
			unsigned char c = *domainname;
			if (!c || c == ' ')
				break;
			domainname++;
			if (c == '\\') {
				unsigned char d;
				unsigned char e;
				u8 f;
				c = *domainname++;
				switch (c) {
				case '\\':  /* "\\" */
					continue;
				case '0':   /* "\ooo" */
				case '1':
				case '2':
				case '3':
					d = *domainname++;
					if (d < '0' || d > '7')
						break;
					e = *domainname++;
					if (e < '0' || e > '7')
						break;
					f = (((u8) (c - '0')) << 6) +
						(((u8) (d - '0')) << 3) +
						(((u8) (e - '0')));
					/* pattern is not \000 */
					if (f && (f <= ' ' || f >= 127))
						continue;
				}
				goto out;
			} else if (c < ' ' || c >= 127) {
				goto out;
			}
		}
	} while (*domainname);
	return true;
out:
	return false;
}

void fprintf_encoded(FILE *fp, const char *pathname)
{
	while (true) {
		unsigned char c = *(const unsigned char *) pathname++;
		if (!c)
			break;
		if (c == '\\') {
			fputc('\\', fp);
			fputc('\\', fp);
		} else if (c > ' ' && c < 127) {
			fputc(c, fp);
		} else {
			fprintf(fp, "\\%c%c%c", (c >> 6) + '0',
				((c >> 3) & 7) + '0', (c & 7) + '0');
		}
	}
}

_Bool decode(const char *ascii, char *bin)
{
	while (true) {
		char c = *ascii++;
		*bin++ = c;
		if (!c)
			break;
		if (c == '\\') {
			char d;
			char e;
			u8 f;
			c = *ascii++;
			switch (c) {
			case '\\':      /* "\\" */
				continue;
			case '0':       /* "\ooo" */
			case '1':
			case '2':
			case '3':
				d = *ascii++;
				if (d < '0' || d > '7')
					break;
				e = *ascii++;
				if (e < '0' || e > '7')
					break;
				f = (u8) ((c - '0') << 6) +
					(((u8) (d - '0')) << 3) +
					(((u8) (e - '0')));
				if (f && (f <= ' ' || f >= 127)) {
					*(bin - 1) = f;
					continue; /* pattern is not \000 */
				}
			}
			return false;
		} else if (c <= ' ' || c >= 127) {
			return false;
		}
	}
	return true;
}

_Bool is_correct_path(const char *filename, const s8 start_type,
		     const s8 pattern_type, const s8 end_type)
{
	_Bool contains_pattern = false;
	unsigned char c;
	if (!filename)
		goto out;
	c = *filename;
	if (start_type == 1) { /* Must start with '/' */
		if (c != '/')
			goto out;
	} else if (start_type == -1) { /* Must not start with '/' */
		if (c == '/')
			goto out;
	}
	if (c)
		c = *(strchr(filename, '\0') - 1);
	if (end_type == 1) { /* Must end with '/' */
		if (c != '/')
			goto out;
	} else if (end_type == -1) { /* Must not end with '/' */
		if (c == '/')
			goto out;
	}
	while (true) {
		c = *filename++;
		if (!c)
			break;
		if (c == '\\') {
			unsigned char d;
			unsigned char e;
			c = *filename++;
			switch (c) {
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
				if (pattern_type == -1)
					break; /* Must not contain pattern */
				contains_pattern = true;
				continue;
			case '0':   /* "\ooo" */
			case '1':
			case '2':
			case '3':
				d = *filename++;
				if (d < '0' || d > '7')
					break;
				e = *filename++;
				if (e < '0' || e > '7')
					break;
				c = make_byte(c, d, e);
				if (c && (c <= ' ' || c >= 127))
					continue; /* pattern is not \000 */
			}
			goto out;
		} else if (c <= ' ' || c >= 127) {
			goto out;
		}
	}
	if (pattern_type == 1) { /* Must contain pattern */
		if (!contains_pattern)
			goto out;
	}
	return true;
out:
	return false;
}

static _Bool file_matches_pattern2(const char *filename,
				  const char *filename_end,
				  const char *pattern, const char *pattern_end)
{
	while (filename < filename_end && pattern < pattern_end) {
		char c;
		if (*pattern != '\\') {
			if (*filename++ != *pattern++)
				return false;
			continue;
		}
		c = *filename;
		pattern++;
		switch (*pattern) {
			int i;
			int j;
		case '?':
			if (c == '/') {
				return false;
			} else if (c == '\\') {
				if (filename[1] == '\\')
					filename++;
				else if (is_byte_range(filename + 1))
					filename += 3;
				else
					return false;
			}
			break;
		case '\\':
			if (c != '\\')
				return false;
			if (*++filename != '\\')
				return false;
			break;
		case '+':
			if (!is_decimal(c))
				return false;
			break;
		case 'x':
			if (!is_hexadecimal(c))
				return false;
			break;
		case 'a':
			if (!is_alphabet_char(c))
				return false;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
			if (c == '\\' && is_byte_range(filename + 1)
			    && !strncmp(filename + 1, pattern, 3)) {
				filename += 3;
				pattern += 2;
				break;
			}
			return false; /* Not matched. */
		case '*':
		case '@':
			for (i = 0; i <= filename_end - filename; i++) {
				if (file_matches_pattern2(filename + i,
							  filename_end,
							  pattern + 1,
							  pattern_end))
					return true;
				c = filename[i];
				if (c == '.' && *pattern == '@')
					break;
				if (c != '\\')
					continue;
				if (filename[i + 1] == '\\')
					i++;
				else if (is_byte_range(filename + i + 1))
					i += 3;
				else
					break; /* Bad pattern. */
			}
			return false; /* Not matched. */
		default:
			j = 0;
			c = *pattern;
			if (c == '$') {
				while (is_decimal(filename[j]))
					j++;
			} else if (c == 'X') {
				while (is_hexadecimal(filename[j]))
					j++;
			} else if (c == 'A') {
				while (is_alphabet_char(filename[j]))
					j++;
			}
			for (i = 1; i <= j; i++) {
				if (file_matches_pattern2(filename + i,
							  filename_end,
							  pattern + 1,
							  pattern_end))
					return true;
			}
			return false; /* Not matched or bad pattern. */
		}
		filename++;
		pattern++;
	}
	while (*pattern == '\\' &&
	       (*(pattern + 1) == '*' || *(pattern + 1) == '@'))
		pattern += 2;
	return filename == filename_end && pattern == pattern_end;
}

_Bool file_matches_pattern(const char *filename, const char *filename_end,
			  const char *pattern, const char *pattern_end)
{
	const char *pattern_start = pattern;
	_Bool first = true;
	_Bool result;
	while (pattern < pattern_end - 1) {
		/* Split at "\-" pattern. */
		if (*pattern++ != '\\' || *pattern++ != '-')
			continue;
		result = file_matches_pattern2(filename, filename_end,
					       pattern_start, pattern - 2);
		if (first)
			result = !result;
		if (result)
			return false;
		first = false;
		pattern_start = pattern;
	}
	result = file_matches_pattern2(filename, filename_end,
				       pattern_start, pattern_end);
	return first ? result : !result;
}

_Bool path_matches_pattern(const struct path_info *filename,
			  const struct path_info *pattern)
{
	/*
	if (!filename || !pattern)
		return false;
	*/
	const char *f = filename->name;
	const char *p = pattern->name;
	const int len = pattern->const_len;
	/* If @pattern doesn't contain pattern, I can use strcmp(). */
	if (!pattern->is_patterned)
		return !pathcmp(filename, pattern);
	/* Don't compare if the number of '/' differs. */
	if (filename->depth != pattern->depth)
		return false;
	/* Compare the initial length without patterns. */
	if (strncmp(f, p, len))
		return false;
	f += len;
	p += len;
	/* Main loop. Compare each directory component. */
	while (*f && *p) {
		const char *f_delimiter = strchr(f, '/');
		const char *p_delimiter = strchr(p, '/');
		if (!f_delimiter)
			f_delimiter = strchr(f, '\0');
		if (!p_delimiter)
			p_delimiter = strchr(p, '\0');
		if (!file_matches_pattern(f, f_delimiter, p, p_delimiter))
			return false;
		f = f_delimiter;
		if (*f)
			f++;
		p = p_delimiter;
		if (*p)
			p++;
	}
	/* Ignore trailing "\*" and "\@" in @pattern. */
	while (*p == '\\' &&
	       (*(p + 1) == '*' || *(p + 1) == '@'))
		p += 2;
	return !*f && !*p;
}

int string_compare(const void *a, const void *b)
{
	return strcmp(*(char **) a, *(char **) b);
}

_Bool pathcmp(const struct path_info *a, const struct path_info *b)
{
	return a->hash != b->hash || strcmp(a->name, b->name);
}

void fill_path_info(struct path_info *ptr)
{
	const char *name = ptr->name;
	const int len = strlen(name);
	ptr->total_len = len;
	ptr->const_len = const_part_length(name);
	ptr->is_dir = len && (name[len - 1] == '/');
	ptr->is_patterned = (ptr->const_len < len);
	ptr->hash = full_name_hash(name, len);
	ptr->depth = path_depth(name);
}

static unsigned int memsize(const unsigned int size)
{
	if (size <= 1048576)
		return ((size / PAGE_SIZE) + 1) * PAGE_SIZE;
	return 0;
}

const struct path_info *savename(const char *name)
{
	static struct free_memory_block_list fmb_list = { NULL, NULL, 0 };
	/* The list of names. */
	static struct savename_entry name_list[SAVENAME_MAX_HASH];
	struct savename_entry *ptr;
	struct savename_entry *prev = NULL;
	unsigned int hash;
	struct free_memory_block_list *fmb = &fmb_list;
	int len;
	static _Bool first_call = true;
	if (!name)
		return NULL;
	len = strlen(name) + 1;
	hash = full_name_hash((const unsigned char *) name, len - 1);
	if (first_call) {
		int i;
		first_call = false;
		memset(&name_list, 0, sizeof(name_list));
		for (i = 0; i < SAVENAME_MAX_HASH; i++) {
			name_list[i].entry.name = "/";
			fill_path_info(&name_list[i].entry);
		}
	}
	ptr = &name_list[hash % SAVENAME_MAX_HASH];
	while (ptr) {
		if (hash == ptr->entry.hash && !strcmp(name, ptr->entry.name))
			goto out;
		prev = ptr;
		ptr = ptr->next;
	}
	while (len > fmb->len) {
		char *cp;
		if (fmb->next) {
			fmb = fmb->next;
			continue;
		}
		cp = malloc(memsize(len));
		if (!cp)
			out_of_memory();
		fmb->next = alloc_element(sizeof(*fmb->next));
		if (!fmb->next)
			out_of_memory();
		memset(cp, 0, memsize(len));
		fmb = fmb->next;
		fmb->ptr = cp;
		fmb->len = memsize(len);
	}
	ptr = alloc_element(sizeof(*ptr));
	if (!ptr)
		out_of_memory();
	memset(ptr, 0, sizeof(struct savename_entry));
	ptr->entry.name = fmb->ptr;
	memmove(fmb->ptr, name, len);
	fill_path_info(&ptr->entry);
	fmb->ptr += len;
	fmb->len -= len;
	prev->next = ptr; /* prev != NULL because name_list is not empty. */
	if (!fmb->len) {
		struct free_memory_block_list *ptr = &fmb_list;
		while (ptr->next != fmb)
			ptr = ptr->next;
		ptr->next = fmb->next;
	}
out:
	return ptr ? &ptr->entry : NULL;
}

_Bool move_proc_to_file(const char *src, const char *base, const char *dest)
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

_Bool is_identical_file(const char *file1, const char *file2)
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

void clear_domain_policy(struct domain_policy *dp)
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

int find_domain_by_ptr(struct domain_policy *dp,
		       const struct path_info *domainname)
{
	int i;
	for (i = 0; i < dp->list_len; i++) {
		if (dp->list[i].domainname == domainname)
			return i;
	}
	return EOF;
}

_Bool save_domain_policy_with_diff(struct domain_policy *dp,
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
		_Bool nm = network_mode;
		offline_mode = false;
		network_mode = false;
		read_domain_policy(bp, base);
		offline_mode = om;
		network_mode = nm;
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

const char *domain_name(const struct domain_policy *dp, const int index)
{
	return dp->list[index].domainname->name;
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

void read_domain_policy(struct domain_policy *dp, const char *filename)
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

void delete_domain(struct domain_policy *dp, const int index)
{
	if (index >= 0 && index < dp->list_len) {
		int i;
		free(dp->list[index].string_ptr);
		for (i = index; i < dp->list_len - 1; i++)
			dp->list[i] = dp->list[i + 1];
		dp->list_len--;
	}
}

void handle_domain_policy(struct domain_policy *dp, FILE *fp, _Bool is_write)
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

/* Variables */

const char *proc_policy_dir           = "/proc/ccs/",
	*disk_policy_dir              = "/etc/ccs/",
	*proc_policy_domain_policy    = "/proc/ccs/domain_policy",
	*proc_policy_exception_policy = "/proc/ccs/exception_policy",
	*proc_policy_system_policy    = "/proc/ccs/system_policy",
	*proc_policy_profile          = "/proc/ccs/profile",
	*proc_policy_manager          = "/proc/ccs/manager",
	*proc_policy_meminfo          = "/proc/ccs/meminfo",
	*proc_policy_query            = "/proc/ccs/query",
	*proc_policy_grant_log        = "/proc/ccs/grant_log",
	*proc_policy_reject_log       = "/proc/ccs/reject_log",
	*proc_policy_domain_status    = "/proc/ccs/.domain_status",
	*proc_policy_process_status   = "/proc/ccs/.process_status";

char shared_buffer[sizeof(shared_buffer)];
static _Bool buffer_locked = false;

/* Main functions */

void get(void)
{
	if (buffer_locked)
		out_of_memory();
	buffer_locked = true;
}

void put(void)
{
	if (!buffer_locked)
		out_of_memory();
	buffer_locked = false;
}

void shprintf(const char *fmt, ...)
{
	va_list args;
	if (!buffer_locked)
		out_of_memory();
	memset(shared_buffer, 0, sizeof(shared_buffer));
	va_start(args, fmt);
	vsnprintf(shared_buffer, sizeof(shared_buffer) - 1, fmt, args);
	va_end(args);
}

_Bool freadline(FILE *fp)
{
	char *cp;
	if (!buffer_locked)
		out_of_memory();
	memset(shared_buffer, 0, sizeof(shared_buffer));
	if (network_mode) {
		int i;
		for (i = 0; i < sizeof(shared_buffer) - 1; i++) {
			if (fread(shared_buffer + i, 1, 1, fp) != 1 ||
			    !shared_buffer[i])
				return false;
			if (shared_buffer[i] == '\n')
				break;
		}
	} else {
		if (!fgets(shared_buffer, sizeof(shared_buffer) - 1, fp))
			return false;
	}
	cp = strchr(shared_buffer, '\n');
	if (!cp)
		return false;
	*cp = '\0';
	normalize_line(shared_buffer);
	return true;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	const char *argv0 = argv[0];
	if (!argv0) {
		fprintf(stderr, "Function not specified.\n");
		return 1;
	}
	if (!access("/sys/kernel/security/tomoyo/", F_OK)) {
		proc_policy_dir
			= "/sys/kernel/security/tomoyo/";
		disk_policy_dir
			= "/etc/tomoyo/";
		proc_policy_domain_policy
			= "/sys/kernel/security/tomoyo/domain_policy";
		proc_policy_exception_policy
			= "/sys/kernel/security/tomoyo/exception_policy";
		proc_policy_system_policy
			= "/sys/kernel/security/tomoyo/system_policy";
		proc_policy_profile
			= "/sys/kernel/security/tomoyo/profile";
		proc_policy_manager
			= "/sys/kernel/security/tomoyo/manager";
		proc_policy_meminfo
			= "/sys/kernel/security/tomoyo/meminfo";
		proc_policy_query
			= "/sys/kernel/security/tomoyo/query";
		proc_policy_grant_log
			= "/sys/kernel/security/tomoyo/grant_log";
		proc_policy_reject_log
			= "/sys/kernel/security/tomoyo/reject_log";
		proc_policy_domain_status
			= "/sys/kernel/security/tomoyo/.domain_status";
		proc_policy_process_status
			= "/sys/kernel/security/tomoyo/.process_status";
	}
	if (strrchr(argv0, '/'))
		argv0 = strrchr(argv0, '/') + 1;
retry:
	if (!strcmp(argv0, "sortpolicy"))
		ret = sortpolicy_main(argc, argv);
	else if (!strcmp(argv0, "setprofile"))
		ret = setprofile_main(argc, argv);
	else if (!strcmp(argv0, "setlevel"))
		ret = setlevel_main(argc, argv);
	else if (!strcmp(argv0, "diffpolicy"))
		ret = diffpolicy_main(argc, argv);
	else if (!strcmp(argv0, "savepolicy"))
		ret = savepolicy_main(argc, argv);
	else if (!strcmp(argv0, "pathmatch"))
		ret = pathmatch_main(argc, argv);
	else if (!strcmp(argv0, "loadpolicy"))
		ret = loadpolicy_main(argc, argv);
	else if (!strcmp(argv0, "ld-watch"))
		ret = ldwatch_main(argc, argv);
	else if (!strcmp(argv0, "findtemp"))
		ret = findtemp_main(argc, argv);
	else if (!strcmp(argv0, "editpolicy"))
		ret = editpolicy_main(argc, argv);
	else if (!strcmp(argv0, "checkpolicy"))
		ret = checkpolicy_main(argc, argv);
	else if (!strcmp(argv0, "ccstree"))
		ret = ccstree_main(argc, argv);
	else if (!strcmp(argv0, "ccs-queryd"))
		ret = ccsqueryd_main(argc, argv);
	else if (!strcmp(argv0, "ccs-auditd"))
		ret = ccsauditd_main(argc, argv);
	else if (!strcmp(argv0, "patternize"))
		ret = patternize_main(argc, argv);
	else if (!strncmp(argv0, "ccs-", 4)) {
		argv0 += 4;
		goto retry;
	} else
		goto show_version;
	return ret;
show_version:
	/*
	 * Unlike busybox, I don't use argv[1] if argv[0] is the name of this
	 * program because it is dangerous to allow updating policies via
	 * unchecked argv[1].
	 * You should use either "symbolic links with 'alias' directive" or
	 * "hard links".
	 */
	printf("ccstools version 1.6.7-rc build 2009/03/16\n");
	fprintf(stderr, "Function %s not implemented.\n", argv0);
	return 1;
}
