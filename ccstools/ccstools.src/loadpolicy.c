/*
 * loadpolicy.c
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-pre   2009/02/24
 *
 */
#include "ccstools.h"

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
	int load_meminfo = 0;
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
			char *q = strchr(ptr, 'q');
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
			if (q)
				load_meminfo = 1;
			if (f)
				refresh_policy = true;
			if (i)
				read_from_stdin = true;
			if (strcspn(ptr, "sedafpmq-"))
				goto usage;
			if (read_from_stdin && load_system_policy +
			    load_exception_policy + load_domain_policy +
			    load_profile + load_manager + load_meminfo != 1)
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

	if (load_meminfo) {
		if (read_from_stdin)
			move_file_to_proc(NULL, NULL, proc_policy_meminfo);
		else
			move_file_to_proc(base_policy_meminfo,
					  disk_policy_meminfo,
					  proc_policy_meminfo);
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
	printf("%s [s][e][d][a][f][p][m][q][-]\n"
	       "s : Load system_policy.\n"
	       "e : Load exception_policy.\n"
	       "d : Load domain_policy.\n"
	       "a : Load system_policy,exception_policy,domain_policy.\n"
	       "p : Load profile.\n"
	       "m : Load manager.\n"
	       "q : Load meminfo.\n"
	       "- : Read policy from stdin. "
	       "(Only one of 'sedpmq' is possible when using '-'.)\n"
	       "f : Delete on-memory policy before loading on-disk policy. "
	       "(Valid for 'sed'.)\n\n", argv[0]);
	return 0;
}

/***** loadpolicy end *****/
