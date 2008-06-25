INSTALLDIR = /

MAIN_FILES = ccstools realpath

MISC_FILES = makesyaoranconf candy chaplet checktoken gettoken groovy honey mailauth proxy timeauth falsh ccs-notifyd force-logout audit-exec-param convert-exec-param

ALIAS_LIST = ccs-auditd ccs-queryd ccstree checkpolicy editpolicy editpolicy_offline findtemp ld-watch loadpolicy pathmatch patternize savepolicy setlevel setprofile sortpolicy domainmatch

all: main misc

main: $(MAIN_FILES)

misc: $(MISC_FILES)

install: all
	mkdir -p $(INSTALLDIR)/usr/lib/ccs/misc $(INSTALLDIR)/sbin $(INSTALLDIR)/usr/sbin
	chmod 755 $(INSTALLDIR)/usr/lib/ccs
	chmod 755 domainmatch init_policy.sh tomoyo_init_policy.sh
	cp -af --remove-destination ccstools ccstools.conf $(INSTALLDIR)/usr/lib/ccs/
	for i in $(ALIAS_LIST); do ln -f $(INSTALLDIR)/usr/lib/ccs/ccstools $(INSTALLDIR)/usr/lib/ccs/$$i; done
	for i in $(ALIAS_LIST); do ln -sf /usr/lib/ccs/$$i $(INSTALLDIR)/usr/sbin/ccs-`echo $$i | sed 's:^ccs-::'`; done
	cp -af --remove-destination realpath domainmatch init_policy.sh tomoyo_init_policy.sh $(INSTALLDIR)/usr/lib/ccs/
	cp -af --remove-destination $(MISC_FILES) $(INSTALLDIR)/usr/lib/ccs/misc/
	cp -af --remove-destination README.ccs COPYING.ccs $(INSTALLDIR)/usr/lib/ccs/
	chown -R root:root ccs-init tomoyo-init $(INSTALLDIR)/usr/lib/ccs/
	chmod 4755 $(INSTALLDIR)/usr/lib/ccs/misc/proxy $(INSTALLDIR)/usr/lib/ccs/misc/force-logout
	chmod 700 ccs-init tomoyo-init
	cp -af --remove-destination ccs-init tomoyo-init $(INSTALLDIR)/sbin/
	mkdir -p $(INSTALLDIR)/usr/share/man/man8/
	cp -af --remove-destination man/man8/* $(INSTALLDIR)/usr/share/man/man8/

CC=gcc

CFLAGS=-Wall -O2 ${shell $(CC) -Wno-pointer-sign -S -o /dev/null -x c - < /dev/null > /dev/null 2>&1 && echo "-Wno-pointer-sign"}

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
	rm -f $(MAIN_FILES) $(MISC_FILES)

.PHONY: clean install
