tomoyo: ccs-auditd ccs-queryd ccstree checkpolicy editpolicy editpolicy_offline findtemp ld-watch loadpolicy pathmatch patternize savepolicy setlevel setprofile sortpolicy realpath

syaoran: makesyaoranconf

cerberus: candy chaplet checktoken falsh gettoken groovy honey mailauth proxy timeauth

#
# If you want to suppress "warning: pointer targets in passing argument ... differ in signedness" warning messages,
# you can add "-Wno-pointer-sign" options to CFLAGS.
#
#CFLAGS=-Wall -Wno-pointer-sign -O3
#
CFLAGS=-Wall -O3

CC=gcc

all: tomoyo syaoran cerberus

#
# Header files dependency.
#
/usr/include/curses.h:
	@echo "/usr/include/curses.h is missing."
	@echo "Run 'yum install ncurses-devel' or 'apt-get install libncurses5-dev'"

/usr/include/openssl/md5.h:
	@echo "/usr/include/openssl/md5.h is missing."
	@echo "Run 'yum install openssl-devel' or 'apt-get install libssl-dev'"

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

ccstools: ccstools.c /usr/include/curses.h 
	$(CC) $(CFLAGS) -o ccstools ccstools.c -lncurses

realpath: realpath.c 
	$(CC) $(CFLAGS) -o realpath realpath.c

#
# Tools for SYAORAN.
#

makesyaoranconf: makesyaoranconf.c
	$(CC) $(CFLAGS) -o makesyaoranconf makesyaoranconf.c
	chmod 700 makesyaoranconf

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
	chmod 755 candy

chaplet: chaplet.c
	$(CC) $(CFLAGS) -o chaplet chaplet.c
	chmod 755 chaplet

checktoken: checktoken.c
	$(CC) $(CFLAGS) -o checktoken checktoken.c
	chmod 755 checktoken

falsh: falsh.c /usr/include/curses.h /usr/include/readline/readline.h
	$(CC) $(CFLAGS) -o falsh falsh.c -lncurses -lreadline
	chmod 755 falsh

gettoken: gettoken.c
	$(CC) $(CFLAGS) -o gettoken gettoken.c
	chmod 755 gettoken

groovy: groovy.c
	$(CC) $(CFLAGS) -o groovy groovy.c
	chmod 755 groovy
 
honey: honey.c
	$(CC) $(CFLAGS) -o honey honey.c
	chmod 755 honey

# Programs developed after Workshop on Informatics 2005.

mailauth: mailauth.c /usr/include/openssl/md5.h
	$(CC) $(CFLAGS) -o mailauth mailauth.c -lssl

proxy: proxy.c
	$(CC) $(CFLAGS) -o proxy proxy.c
	chown root:root proxy
	chmod 4755 proxy

timeauth: timeauth.c
	$(CC) $(CFLAGS) -o timeauth timeauth.c

#
# Delete all tools in this package.
#

clean:
	rm -f ccs-auditd ccs-queryd ccstree checkpolicy editpolicy editpolicy_offline findtemp ld-watch loadpolicy pathmatch patternize savepolicy setlevel setprofile sortpolicy ccstools realpath
	rm -f makesyaoranconf
	rm -f candy chaplet checktoken falsh gettoken groovy honey mailauth proxy timeauth
