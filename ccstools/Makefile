INSTALLDIR = /

MAIN_FILES = ccstools realpath

MISC_FILES = makesyaoranconf candy chaplet checktoken gettoken groovy honey mailauth proxy timeauth falsh

ALIAS_LIST = ccs-auditd ccs-queryd ccstree checkpolicy editpolicy editpolicy_offline findtemp ld-watch loadpolicy pathmatch patternize savepolicy setlevel setprofile sortpolicy

all: main misc

main: $(MAIN_FILES)

misc: $(MISC_FILES)

install:
	mkdir -p $(INSTALLDIR)/usr/lib/ccs/misc $(INSTALLDIR)/sbin
	chmod 700 $(INSTALLDIR)/usr/lib/ccs
	chmod 755 domainmatch init_policy.sh tomoyo_init_policy.sh
	cp -af --remove-destination ccstools $(INSTALLDIR)/usr/lib/ccs/
	for i in $(ALIAS_LIST); do ln -f $(INSTALLDIR)/usr/lib/ccs/ccstools $(INSTALLDIR)/usr/lib/ccs/$$i; done
	for i in $(ALIAS_LIST); do ln -sf $(INSTALLDIR)/usr/lib/ccs/$$i $(INSTALLDIR)/usr/sbin/ccs-`echo $$i | sed 's:^ccs-::'`; done
	cp -af --remove-destination realpath domainmatch init_policy.sh tomoyo_init_policy.sh $(INSTALLDIR)/usr/lib/ccs/
	cp -af --remove-destination $(MISC_FILES) $(INSTALLDIR)/usr/lib/ccs/misc/
	chown -R root:root ccs-init tomoyo-init $(INSTALLDIR)/usr/lib/ccs/
	chmod 4755 $(INSTALLDIR)/usr/lib/ccs/misc/proxy
	chmod 700 ccs-init tomoyo-init
	cp -af --remove-destination ccs-init tomoyo-init $(INSTALLDIR)/sbin/

#
# If you want to suppress "warning: pointer targets in passing argument ... differ in signedness" warning messages,
# you can add "-Wno-pointer-sign" options to CFLAGS.
#
#CFLAGS=-Wall -Wno-pointer-sign -O2
#
CFLAGS=-Wall -O2

CC=gcc

/usr/include/curses.h:
	@echo "/usr/include/curses.h is missing."
	@echo "Run 'yum install ncurses-devel' or 'apt-get install libncurses5-dev'"

/usr/include/readline/readline.h:
	@echo "/usr/include/readline/readline.h is missing."
	@echo "Run 'yum install readline-devel' or 'apt-get install libreadline5-dev'"

ccstools: ccstools.src/*.c ccstools.src/*.h /usr/include/curses.h
	$(CC) $(CFLAGS) -o ccstools ccstools.src/*.c -lncurses -DCOLOR_ON

falsh: falsh.c /usr/include/curses.h /usr/include/readline/readline.h
	$(CC) $(CFLAGS) -o falsh falsh.c -lncurses -lreadline

.c:
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(MAIN_FILES) $(MISC_FILES) falsh

.PHONY: clean install
