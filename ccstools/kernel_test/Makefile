all: sakura_bind_test sakura_filesystem_test tomoyo_capability_test tomoyo_file_test tomoyo_rewrite_test tomoyo_argv0_test tomoyo_network_test tomoyo_signal_test newns tomoyo_accept_test
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

sakura_bind_test: sakura_bind_test.c include.h
	$(CC) $(CFLAGS) -o sakura_bind_test sakura_bind_test.c
	chmod 700 sakura_bind_test 

sakura_filesystem_test: sakura_filesystem_test.c include.h
	$(CC) $(CFLAGS) -o sakura_filesystem_test sakura_filesystem_test.c
	chmod 700 sakura_filesystem_test

tomoyo_capability_test: tomoyo_capability_test.c include.h
	$(CC) $(CFLAGS) -o tomoyo_capability_test tomoyo_capability_test.c -lutil
	chmod 700 tomoyo_capability_test

tomoyo_file_test: tomoyo_file_test.c include.h
	$(CC) $(CFLAGS) -o tomoyo_file_test tomoyo_file_test.c
	chmod 700 tomoyo_file_test

tomoyo_rewrite_test: tomoyo_rewrite_test.c include.h
	$(CC) $(CFLAGS) -o tomoyo_rewrite_test tomoyo_rewrite_test.c
	chmod 700 tomoyo_rewrite_test

tomoyo_argv0_test: tomoyo_argv0_test.c include.h
	$(CC) $(CFLAGS) -o tomoyo_argv0_test tomoyo_argv0_test.c
	chmod 700 tomoyo_argv0_test

tomoyo_network_test: tomoyo_network_test.c include.h
	$(CC) $(CFLAGS) -o tomoyo_network_test tomoyo_network_test.c
	chmod 700 tomoyo_network_test

tomoyo_signal_test: tomoyo_signal_test.c include.h
	$(CC) $(CFLAGS) -o tomoyo_signal_test tomoyo_signal_test.c
	chmod 700 tomoyo_signal_test

newns: newns.c
	$(CC) $(CFLAGS) -o newns newns.c

tomoyo_accept_test: tomoyo_accept_test.c
	$(CC) $(CFLAGS) -o tomoyo_accept_test tomoyo_accept_test.c

#
# Delete all tools in this package.
#

clean:
	rm -f sakura_bind_test sakura_filesystem_test
	rm -f tomoyo_capability_test tomoyo_file_test tomoyo_rewrite_test tomoyo_argv0_test tomoyo_network_test tomoyo_signal_test
	rm -f newns tomoyo_accept_test
