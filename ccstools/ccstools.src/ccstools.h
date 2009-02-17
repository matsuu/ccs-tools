/*
 * ccstools.h
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2009  NTT DATA CORPORATION
 *
 * Version: 1.6.7-pre   2009/02/17
 *
 */

/***** CONSTANTS DEFINITION START *****/

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define s8 __s8
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
#include <signal.h>
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

#define true     1
#define false    0

enum screen_type {
	SCREEN_SYSTEM_LIST,
	SCREEN_EXCEPTION_LIST,
	SCREEN_DOMAIN_LIST,
	SCREEN_ACL_LIST,
	SCREEN_PROFILE_LIST,
	SCREEN_MANAGER_LIST,
	SCREEN_QUERY_LIST,
	SCREEN_MEMINFO_LIST,
	MAXSCREEN
};

enum policy_type {
	POLICY_TYPE_UNKNOWN,
	POLICY_TYPE_DOMAIN_POLICY,
	POLICY_TYPE_EXCEPTION_POLICY,
	POLICY_TYPE_SYSTEM_POLICY
};

#define VALUE_TYPE_DECIMAL     1
#define VALUE_TYPE_OCTAL       2
#define VALUE_TYPE_HEXADECIMAL 3

enum socket_operation_type {
	NETWORK_ACL_UDP_BIND,
	NETWORK_ACL_UDP_CONNECT,
	NETWORK_ACL_TCP_BIND,
	NETWORK_ACL_TCP_LISTEN,
	NETWORK_ACL_TCP_CONNECT,
	NETWORK_ACL_TCP_ACCEPT,
	NETWORK_ACL_RAW_BIND,
	NETWORK_ACL_RAW_CONNECT
};

#define KEYWORD_AGGREGATOR               "aggregator "
#define KEYWORD_ALIAS                    "alias "
#define KEYWORD_ALLOW_ARGV0              "allow_argv0 "
#define KEYWORD_ALLOW_ENV                "allow_env "
#define KEYWORD_ALLOW_CAPABILITY         "allow_capability "
#define KEYWORD_ALLOW_CHROOT             "allow_chroot "
#define KEYWORD_ALLOW_MOUNT              "allow_mount "
#define KEYWORD_ALLOW_NETWORK            "allow_network "
#define KEYWORD_ALLOW_PIVOT_ROOT         "allow_pivot_root "
#define KEYWORD_ALLOW_READ               "allow_read "
#define KEYWORD_ALLOW_SIGNAL             "allow_signal "
#define KEYWORD_DELETE                   "delete "
#define KEYWORD_DENY_AUTOBIND            "deny_autobind "
#define KEYWORD_DENY_REWRITE             "deny_rewrite "
#define KEYWORD_DENY_UNMOUNT             "deny_unmount "
#define KEYWORD_FILE_PATTERN             "file_pattern "
#define KEYWORD_MAC_FOR_CAPABILITY       "MAC_FOR_CAPABILITY::"
#define KEYWORD_SELECT                   "select "
#define KEYWORD_UNDELETE                 "undelete "
#define KEYWORD_USE_PROFILE              "use_profile "
#define KEYWORD_USE_PROFILE_LEN          (sizeof(KEYWORD_USE_PROFILE) - 1)
#define KEYWORD_INITIALIZE_DOMAIN        "initialize_domain "
#define KEYWORD_KEEP_DOMAIN              "keep_domain "
#define KEYWORD_PATH_GROUP               "path_group "
#define KEYWORD_ADDRESS_GROUP            "address_group "
#define KEYWORD_NO_INITIALIZE_DOMAIN     "no_" KEYWORD_INITIALIZE_DOMAIN
#define KEYWORD_NO_KEEP_DOMAIN           "no_" KEYWORD_KEEP_DOMAIN
#define KEYWORD_EXECUTE_HANDLER          "execute_handler "
#define KEYWORD_DENIED_EXECUTE_HANDLER   "denied_execute_handler "
#define KEYWORD_ALLOW_EXECUTE            "allow_execute "

#define CCS_AUDITD_MAX_FILES             2
#define SAVENAME_MAX_HASH                256
#define PAGE_SIZE                        4096
#define CCS_MAX_PATHNAME_LEN             4000
#define ROOT_NAME                        "<kernel>"
#define ROOT_NAME_LEN                    (sizeof(ROOT_NAME) - 1)

#define CCSTOOLS_CONFIG_FILE "/usr/lib/ccs/ccstools.conf"

enum editpolicy_directives {
	DIRECTIVE_NONE,
	DIRECTIVE_1,
	DIRECTIVE_2,
	DIRECTIVE_3,
	DIRECTIVE_4,
	DIRECTIVE_5,
	DIRECTIVE_6,
	DIRECTIVE_7,
	DIRECTIVE_ALLOW_EXECUTE,
	DIRECTIVE_ALLOW_READ,
	DIRECTIVE_ALLOW_WRITE,
	DIRECTIVE_ALLOW_READ_WRITE,
	DIRECTIVE_ALLOW_CREATE,
	DIRECTIVE_ALLOW_UNLINK,
	DIRECTIVE_ALLOW_MKDIR,
	DIRECTIVE_ALLOW_RMDIR,
	DIRECTIVE_ALLOW_MKFIFO,
	DIRECTIVE_ALLOW_MKSOCK,
	DIRECTIVE_ALLOW_MKBLOCK,
	DIRECTIVE_ALLOW_MKCHAR,
	DIRECTIVE_ALLOW_TRUNCATE,
	DIRECTIVE_ALLOW_SYMLINK,
	DIRECTIVE_ALLOW_LINK,
	DIRECTIVE_ALLOW_RENAME,
	DIRECTIVE_ALLOW_REWRITE,
	DIRECTIVE_ALLOW_ARGV0,
	DIRECTIVE_ALLOW_SIGNAL,
	DIRECTIVE_ALLOW_NETWORK,
	DIRECTIVE_ALLOW_ENV,
	DIRECTIVE_ADDRESS_GROUP,
	DIRECTIVE_AGGREGATOR,
	DIRECTIVE_ALIAS,
	DIRECTIVE_ALLOW_CAPABILITY,
	DIRECTIVE_ALLOW_CHROOT,
	DIRECTIVE_ALLOW_MOUNT,
	DIRECTIVE_ALLOW_PIVOT_ROOT,
	DIRECTIVE_DENY_AUTOBIND,
	DIRECTIVE_DENY_REWRITE,
	DIRECTIVE_DENY_UNMOUNT,
	DIRECTIVE_FILE_PATTERN,
	DIRECTIVE_EXECUTE_HANDLER,
	DIRECTIVE_DENIED_EXECUTE_HANDLER,
	DIRECTIVE_IGNORE_GLOBAL_ALLOW_ENV,
	DIRECTIVE_IGNORE_GLOBAL_ALLOW_READ,
	DIRECTIVE_INITIALIZE_DOMAIN,
	DIRECTIVE_KEEP_DOMAIN,
	DIRECTIVE_NO_INITIALIZE_DOMAIN,
	DIRECTIVE_NO_KEEP_DOMAIN,
	DIRECTIVE_PATH_GROUP,
	DIRECTIVE_QUOTA_EXCEEDED,
	DIRECTIVE_USE_PROFILE,
	MAX_DIRECTIVE_INDEX
};

enum color_pair {
	NORMAL, DOMAIN_HEAD, DOMAIN_CURSOR, SYSTEM_HEAD, SYSTEM_CURSOR,
	EXCEPTION_HEAD, EXCEPTION_CURSOR, ACL_HEAD, ACL_CURSOR, DISP_ERR
};

static const int header_lines = 3;

/***** CONSTANTS DEFINITION END *****/

/***** STRUCTURES DEFINITION START *****/

struct path_info {
	const char *name;
	u32 hash;           /* = full_name_hash(name, strlen(name)) */
	u16 total_len;      /* = strlen(name)                       */
	u16 const_len;      /* = const_part_length(name)            */
	_Bool is_dir;       /* = strendswith(name, "/")             */
	_Bool is_patterned; /* = path_contains_pattern(name)        */
	u16 depth;          /* = path_depth(name)                   */
};

struct path_group_entry {
	const struct path_info *group_name;
	const struct path_info **member_name;
	int member_name_len;
};

struct ip_address_entry {
	u8 min[16];
	u8 max[16];
	_Bool is_ipv6;
};

struct address_group_entry {
	const struct path_info *group_name;
	struct ip_address_entry *member_name;
	int member_name_len;
};

struct savename_entry {
	struct savename_entry *next;
	struct path_info entry;
};

struct free_memory_block_list {
	struct free_memory_block_list *next;
	char *ptr;
	int len;
};

struct dll_pathname_entry {
	char *pathname;
	char *real_pathname;
};

struct domain_initializer_entry {
	const struct path_info *domainname;    /* This may be NULL */
	const struct path_info *program;
	_Bool is_not;
	_Bool is_last_name;
};

struct domain_keeper_entry {
	const struct path_info *domainname;
	const struct path_info *program;       /* This may be NULL */
	_Bool is_not;
	_Bool is_last_name;
};

struct domain_info {
	const struct path_info *domainname;
	const struct domain_initializer_entry *d_i; /* This may be NULL */
	const struct domain_keeper_entry *d_k; /* This may be NULL */
	const struct path_info **string_ptr;
	int string_count;
	int number;   /* domain number (-1 if is_dis or is_dd) */
	u8 profile;
	_Bool is_dis; /* domain initializer source */
	_Bool is_dit; /* domain initializer target */
	_Bool is_dk;  /* domain keeper */
	_Bool is_du;  /* unreachable domain */
	_Bool is_dd;  /* deleted domain */
};

struct domain_policy {
	struct domain_info *list;
	int list_len;
	unsigned char *list_selected;
};

struct generic_acl {
	u8 directive;
	u8 selected;
	const char *operand;
};

struct editpolicy_directive {
	const char *original;
	const char *alias;
	int original_len;
	int alias_len;
};

struct task_entry {
	pid_t pid;
	pid_t ppid;
	_Bool done;
};

/***** STRUCTURES DEFINITION END *****/

/***** PROTOTYPES DEFINITION START *****/

void out_of_memory(void);
void normalize_line(unsigned char *line);
_Bool is_domain_def(const unsigned char *domainname);
_Bool is_correct_domain(const unsigned char *domainname);
void fprintf_encoded(FILE *fp, const char *pathname);
_Bool decode(const char *ascii, char *bin);
_Bool is_correct_path(const char *filename, const s8 start_type,
		      const s8 pattern_type, const s8 end_type);
_Bool file_matches_pattern(const char *filename, const char *filename_end,
			   const char *pattern, const char *pattern_end);
int string_compare(const void *a, const void *b);
_Bool pathcmp(const struct path_info *a, const struct path_info *b);
void fill_path_info(struct path_info *ptr);
const struct path_info *savename(const char *name);
_Bool str_starts(char *str, const char *begin);
_Bool path_matches_pattern(const struct path_info *pathname0,
			   const struct path_info *pattern0);
char *make_filename(const char *prefix, const time_t time);

int sortpolicy_main(int argc, char *argv[]);
int setprofile_main(int argc, char *argv[]);
int setlevel_main(int argc, char *argv[]);
int diffpolicy_main(int argc, char *argv[]);
int savepolicy_main(int argc, char *argv[]);
int pathmatch_main(int argc, char *argv[]);
int loadpolicy_main(int argc, char *argv[]);
int ldwatch_main(int argc, char *argv[]);
int findtemp_main(int argc, char *argv[]);
int editpolicy_main(int argc, char *argv[]);
int checkpolicy_main(int argc, char *argv[]);
int ccstree_main(int argc, char *argv[]);
int ccsqueryd_main(int argc, char *argv[]);
int ccsauditd_main(int argc, char *argv[]);
int patternize_main(int argc, char *argv[]);
void shprintf(const char *fmt, ...)
	__attribute__ ((format(printf, 1, 2)));
_Bool move_proc_to_file(const char *src, const char *base, const char *dest);
_Bool is_identical_file(const char *file1, const char *file2);
FILE *open_read(const char *filename);
void clear_domain_policy(struct domain_policy *dp);
_Bool save_domain_policy_with_diff(struct domain_policy *dp,
				   struct domain_policy *bp,
				   const char *proc, const char *base,
				   const char *diff);
int find_domain_by_ptr(struct domain_policy *dp,
		       const struct path_info *domainname);
void read_domain_policy(struct domain_policy *dp, const char *filename);
void delete_domain(struct domain_policy *dp, const int index);
void handle_domain_policy(struct domain_policy *dp, FILE *fp, _Bool is_write);
int del_string_entry(struct domain_policy *dp, const char *entry,
		     const int index);
int add_string_entry(struct domain_policy *dp, const char *entry,
		     const int index);
int find_domain(struct domain_policy *dp, const char *domainname0,
		const _Bool is_dis, const _Bool is_dd);
int find_or_assign_new_domain(struct domain_policy *dp, const char *domainname,
			      const _Bool is_dis, const _Bool is_dd);
const char *domain_name(const struct domain_policy *dp, const int index);
void send_fd(char *data, int *fd);
void editpolicy_offline_daemon(void);
void editpolicy_init_keyword_map(void);
void editpolicy_line_draw(const int screen);
void editpolicy_try_optimize(struct domain_policy *dp, const int current,
			     const int screen);
struct path_group_entry *find_path_group(const char *group_name);
int add_address_group_policy(char *data, const _Bool is_delete);
u8 find_directive(const _Bool forward, char *line);
void editpolicy_color_init(void);
void editpolicy_color_change(const attr_t attr, const _Bool flg);
void editpolicy_attr_change(const attr_t attr, const _Bool flg);
void editpolicy_sttr_save(void);
void editpolicy_sttr_restore(void);
int editpolicy_color_head(const int screen);
int editpolicy_color_cursor(const int screen);
int editpolicy_get_current(void);

extern char shared_buffer[8192];
void get(void);
void put(void);
_Bool freadline(FILE *fp);

char *simple_readline(const int start_y, const int start_x, const char *prompt,
		      const char *history[], const int history_count,
		      const int max_length, const int scroll_width);
int simple_add_history(const char *buffer, const char **history,
		       const int history_count, const int max_history);
int getch2(void);

extern _Bool offline_mode;
extern int persistent_fd;
extern int query_fd;
extern int path_group_list_len;
extern int address_group_list_len;
extern struct generic_acl *generic_acl_list;
extern int generic_acl_list_count;
extern char *initial_readline_data;
extern struct path_group_entry *path_group_list;
extern int path_group_list_len;
extern int current_y[MAXSCREEN];
extern int list_item_count[MAXSCREEN];
extern struct editpolicy_directive directives[MAX_DIRECTIVE_INDEX];

extern const char *proc_policy_dir,
	*disk_policy_dir,
	*proc_policy_domain_policy,
	*disk_policy_domain_policy,
	*base_policy_domain_policy,
	*proc_policy_exception_policy,
	*disk_policy_exception_policy,
	*base_policy_exception_policy,
	*proc_policy_system_policy,
	*disk_policy_system_policy,
	*base_policy_system_policy,
	*proc_policy_profile,
	*disk_policy_profile,
	*base_policy_profile,
	*proc_policy_manager,
	*disk_policy_manager,
	*base_policy_manager,
	*proc_policy_meminfo,
	*disk_policy_meminfo,
	*base_policy_meminfo,
	*proc_policy_query,
	*proc_policy_grant_log,
	*proc_policy_reject_log,
	*proc_policy_domain_status,
	*proc_policy_process_status;

/***** PROTOTYPES DEFINITION END *****/
