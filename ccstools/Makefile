INSTALLDIR = /

all: tomoyo syaoran cerberus

tomoyo: ccs-auditd ccs-queryd ccstree checkpolicy editpolicy editpolicy_offline findtemp ld-watch loadpolicy pathmatch patternize savepolicy setlevel setprofile sortpolicy realpath

syaoran: makesyaoranconf

cerberus: candy chaplet checktoken falsh gettoken groovy honey mailauth proxy timeauth

install:
	mkdir -p $(INSTALLDIR)/usr/lib/ccs/misc $(INSTALLDIR)/sbin
	chmod 700 $(INSTALLDIR)/usr/lib/ccs
	chmod 755 domainmatch init_policy.sh tomoyo_init_policy.sh
	cp -af --remove-destination ccs-auditd ccs-queryd ccstree checkpolicy editpolicy editpolicy_offline findtemp ld-watch loadpolicy pathmatch patternize savepolicy setlevel setprofile sortpolicy realpath domainmatch init_policy.sh tomoyo_init_policy.sh $(INSTALLDIR)/usr/lib/ccs/
	cp -af --remove-destination makesyaoranconf candy chaplet checktoken falsh gettoken groovy honey mailauth proxy timeauth $(INSTALLDIR)/usr/lib/ccs/misc/
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

#
# Header files dependency.
#
/usr/include/curses.h:
	@echo "/usr/include/curses.h is missing."
	@echo "Run 'yum install ncurses-devel' or 'apt-get install libncurses5-dev'"

/usr/include/readline/readline.h:
	@echo "/usr/include/readline/readline.h is missing."
	@echo "Run 'yum install readline-devel' or 'apt-get install libreadline5-dev'"

#
# Tools for SAKURA and TOMOYO.
#

ccs-auditd: ccstools
	ln -f ccstools ccs-auditd

ccs-queryd: ccstools
	ln -f ccstools ccs-queryd

ccstree: ccstools
	ln -f ccstools ccstree

checkpolicy: ccstools
	ln -f ccstools checkpolicy

editpolicy: ccstools
	ln -f ccstools editpolicy

editpolicy_offline: ccstools
	ln -f editpolicy editpolicy_offline

findtemp: ccstools
	ln -f ccstools findtemp

ld-watch: ccstools
	ln -f ccstools ld-watch

loadpolicy: ccstools
	ln -f ccstools loadpolicy

pathmatch: ccstools
	ln -f ccstools pathmatch

patternize: ccstools
	ln -f ccstools patternize

savepolicy: ccstools
	ln -f ccstools savepolicy

setlevel: ccstools
	ln -f ccstools setlevel

setprofile: ccstools
	ln -f ccstools setprofile

sortpolicy: ccstools
	ln -f ccstools sortpolicy

ccstools: ccstools.src/*.c ccstools.src/*.h /usr/include/curses.h 
	$(CC) $(CFLAGS) -o ccstools ccstools.src/*.c -lncurses -DCOLOR_ON

realpath: realpath.c 
	$(CC) $(CFLAGS) -o realpath realpath.c

#
# Tools for SYAORAN.
#

makesyaoranconf: makesyaoranconf.c
	$(CC) $(CFLAGS) -o makesyaoranconf makesyaoranconf.c

#
# Tools for CERBERUS.
#
# http://sourceforge.jp/projects/tomoyo/document/winf2005-en.pdf
#
# This package contains only programs prepared for
# Workshop on Informatics 2005 held in Japan.
#

candy: candy.c
	$(CC) $(CFLAGS) -o candy candy.c

chaplet: chaplet.c
	$(CC) $(CFLAGS) -o chaplet chaplet.c

checktoken: checktoken.c
	$(CC) $(CFLAGS) -o checktoken checktoken.c

falsh: falsh.c /usr/include/curses.h /usr/include/readline/readline.h
	$(CC) $(CFLAGS) -o falsh falsh.c -lncurses -lreadline

gettoken: gettoken.c
	$(CC) $(CFLAGS) -o gettoken gettoken.c

groovy: groovy.c
	$(CC) $(CFLAGS) -o groovy groovy.c
 
honey: honey.c
	$(CC) $(CFLAGS) -o honey honey.c

# Programs developed after Workshop on Informatics 2005.

mailauth: mailauth.c
	$(CC) $(CFLAGS) -o mailauth mailauth.c

proxy: proxy.c
	$(CC) $(CFLAGS) -o proxy proxy.c

timeauth: timeauth.c
	$(CC) $(CFLAGS) -o timeauth timeauth.c

#
# Delete all tools in this package.
#

clean:
	rm -f ccs-auditd ccs-queryd ccstree checkpolicy editpolicy editpolicy_offline findtemp ld-watch loadpolicy pathmatch patternize savepolicy setlevel setprofile sortpolicy ccstools realpath
	rm -f makesyaoranconf
	rm -f candy chaplet checktoken falsh gettoken groovy honey mailauth proxy timeauth
