#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([ccs-tools], [1.6.8], [tomoyo-dev@lists.sourceforge.jp])
AC_CONFIG_SRCDIR([src/ccstools.src/ccstools.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([config])

AM_INIT_AUTOMAKE([1.10 dist-bzip2 -Wall])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([ncurses], [strscr])
AC_CHECK_LIB([readline], [readline])
AC_CHECK_LIB([util], [forkpty])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/file.h sys/ioctl.h sys/mount.h sys/socket.h sys/time.h sys/vfs.h syslog.h termios.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm dup2 ftruncate getcwd gethostbyname gethostname gettimeofday memchr memmove memset mkdir mkfifo realpath rmdir select setenv sethostname socket stime strchr strcspn strdup strerror strrchr strstr strtoul])

AC_CONFIG_FILES([Makefile
                 kernel_test/Makefile
                 man/Makefile
                 man/man8/Makefile
                 src/Makefile])
AC_OUTPUT
