/*
 * ccstools.h
 *
 * TOMOYO Linux's utilities.
 *
 * Copyright (C) 2005-2008  NTT DATA CORPORATION
 *
 * Version: 1.6.0   2008/04/01
 *
 */

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
#define KEYWORD_ALLOW_ENV                "allow_env "
#define KEYWORD_ALLOW_ENV_LEN            (sizeof(KEYWORD_ALLOW_ENV) - 1)
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
#define KEYWORD_MAC_FOR_CAPABILITY       "MAC_FOR_CAPABILITY::"
#define KEYWORD_MAC_FOR_CAPABILITY_LEN   (sizeof(KEYWORD_MAC_FOR_CAPABILITY) - 1)
#define KEYWORD_SELECT                   "select "
#define KEYWORD_SELECT_LEN               (sizeof(KEYWORD_SELECT) - 1)
#define KEYWORD_UNDELETE                 "undelete "
#define KEYWORD_UNDELETE_LEN             (sizeof(KEYWORD_UNDELETE) - 1)
#define KEYWORD_USE_PROFILE              "use_profile "
#define KEYWORD_USE_PROFILE_LEN          (sizeof(KEYWORD_USE_PROFILE) - 1)
#define KEYWORD_INITIALIZE_DOMAIN        "initialize_domain "
#define KEYWORD_INITIALIZE_DOMAIN_LEN    (sizeof(KEYWORD_INITIALIZE_DOMAIN) - 1)
#define KEYWORD_KEEP_DOMAIN              "keep_domain "
#define KEYWORD_KEEP_DOMAIN_LEN          (sizeof(KEYWORD_KEEP_DOMAIN) - 1)
#define KEYWORD_PATH_GROUP               "path_group "
#define KEYWORD_PATH_GROUP_LEN           (sizeof(KEYWORD_PATH_GROUP) - 1)
#define KEYWORD_ADDRESS_GROUP            "address_group "
#define KEYWORD_ADDRESS_GROUP_LEN        (sizeof(KEYWORD_ADDRESS_GROUP) - 1)
#define KEYWORD_NO_INITIALIZE_DOMAIN     "no_" KEYWORD_INITIALIZE_DOMAIN
#define KEYWORD_NO_INITIALIZE_DOMAIN_LEN (sizeof(KEYWORD_NO_INITIALIZE_DOMAIN) - 1)
#define KEYWORD_NO_KEEP_DOMAIN           "no_" KEYWORD_KEEP_DOMAIN
#define KEYWORD_NO_KEEP_DOMAIN_LEN       (sizeof(KEYWORD_NO_KEEP_DOMAIN) - 1)

#define CCS_AUDITD_MAX_FILES             2
#define SAVENAME_MAX_HASH                256
#define PAGE_SIZE                        4096
#define CCS_MAX_PATHNAME_LEN             4000
#define ROOT_NAME                        "<kernel>"
#define ROOT_NAME_LEN                    (sizeof(ROOT_NAME) - 1)

#define shared_buffer_len 8192

#define CCSTOOLS_CONFIG_FILE "/usr/lib/ccs/ccstools.conf"

/***** CONSTANTS DEFINITION END *****/

/***** STRUCTURES DEFINITION START *****/

struct path_info {
	const char *name;
	u32 hash;        /* = full_name_hash(name, strlen(name)) */
	u16 total_len;   /* = strlen(name)                       */
	u16 const_len;   /* = const_part_length(name)            */
	u8 is_dir;       /* = strendswith(name, "/")             */
	u8 is_patterned; /* = PathContainsPattern(name)          */
	u16 depth;       /* = PathDepth(name)                    */
};

struct path_group_entry {
	const struct path_info *group_name;
	const struct path_info **member_name;
	int member_name_len;
};

struct ip_address_entry {
	u8 min[16];
	u8 max[16];
	u8 is_ipv6;
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
	unsigned char is_not:1;
	unsigned char is_last_name:1;
};

struct domain_keeper_entry {
	const struct path_info *domainname;
	const struct path_info *program;       /* This may be NULL */
	unsigned char is_not:1;
	unsigned char is_last_name:1;
};

struct domain_info {
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
};

struct task_entry {
	pid_t pid;
	pid_t ppid;
	u8 done;
};

/***** STRUCTURES DEFINITION END *****/

/***** PROTOTYPES DEFINITION START *****/

void OutOfMemory(void);
void NormalizeLine(unsigned char *line);
int IsDomainDef(const unsigned char *domainname);
int IsCorrectDomain(const unsigned char *domainname);
void fprintf_encoded(FILE *fp, const char *pathname);
void RemoveHeader(char *line, const int len);
int IsCorrectPath(const char *filename, const int start_type, const int pattern_type, const int end_type);
int FileMatchesToPattern(const char *filename, const char *filename_end, const char *pattern, const char *pattern_end);
int string_compare(const void *a, const void *b);
int pathcmp(const struct path_info *a, const struct path_info *b);
void fill_path_info(struct path_info *ptr);
const struct path_info *SaveName(const char *name);

extern char *shared_buffer;
void get(void);
void put(void);
int freadline(FILE *fp);

char *simple_readline(const int start_y, const int start_x, const char *prompt, const char *history[], const int history_count, const int max_length, const int scroll_width);
int simple_add_history(const char *buffer, const char **history, const int history_count, const int max_history);
int getch2(void);

extern const char *proc_policy_dir,
	*disk_policy_dir,
	*proc_policy_domain_policy,
	*disk_policy_domain_policy,
	*proc_policy_exception_policy,
	*disk_policy_exception_policy,
	*proc_policy_system_policy,
	*disk_policy_system_policy,
	*proc_policy_profile,
	*disk_policy_profile,
	*proc_policy_manager,
	*disk_policy_manager,
	*proc_policy_query,
	*proc_policy_grant_log,
	*proc_policy_reject_log,
	*proc_policy_domain_status,
	*proc_policy_process_status;

/***** PROTOTYPES DEFINITION END *****/
