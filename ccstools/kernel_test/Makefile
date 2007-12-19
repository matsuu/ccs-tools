ALL_FILES = sakura_bind_test sakura_filesystem_test tomoyo_capability_test tomoyo_file_test tomoyo_rewrite_test tomoyo_argv0_test tomoyo_env_test tomoyo_network_test tomoyo_signal_test newns tomoyo_accept_test

all: $(ALL_FILES)
	chmod 755 testall.sh

#
# If you want to suppress "warning: pointer targets in passing argument ... differ in signedness" warning messages,
# you can add "-Wno-pointer-sign" options to CFLAGS.
#
#CFLAGS=-Wall -Wno-pointer-sign -O2
#
CFLAGS=-Wall -O2

CC=gcc

#
# Tools for kernel testing.
#

tomoyo_capability_test: tomoyo_capability_test.c include.h
	$(CC) $(CFLAGS) -o tomoyo_capability_test tomoyo_capability_test.c -lutil
	chmod 700 tomoyo_capability_test

.c:	include.h
	$(CC) $(CFLAGS) -o $@ $<
	chmod 700 $@

#
# Delete all tools in this package.
#

clean:
	rm -f $(ALL_FILES)
