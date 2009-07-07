SUBDIRS = kernel_test man


EXTRA_DIST = ccs-init ccs-tools.spec ccstools.conf ccstools.src/ccstools.h domainmatch init_policy.sh tomoyo-init tomoyo_init_policy.sh

root_sbindir = /sbin
root_sbin_SCRIPTS = ccs-init tomoyo-init 

ccsdir = $(libdir)/ccs
ccs_PROGRAMS = ccstools realpath make_alias
ccs_SCRIPTS = domainmatch init_policy.sh tomoyo_init_policy.sh
ccs_DATA = ccstools.conf

ccstools_SOURCES = ccstools.src/ccs-auditd.c ccstools.src/ccs-queryd.c ccstools.src/ccstools.c ccstools.src/ccstree.c ccstools.src/checkpolicy.c ccstools.src/editpolicy.c ccstools.src/editpolicy_color.c ccstools.src/editpolicy_keyword.c ccstools.src/editpolicy_offline.c ccstools.src/editpolicy_optimizer.c ccstools.src/findtemp.c ccstools.src/ld-watch.c ccstools.src/loadpolicy.c ccstools.src/pathmatch.c ccstools.src/patternize.c ccstools.src/readline.c ccstools.src/setlevel.c ccstools.src/setprofile.c
ccstools_CPPFLAGS = -DCOLOR_ON
ccstools_LDADD = -lncurses

miscdir = $(ccsdir)/misc
misc_PROGRAMS = makesyaoranconf candy chaplet checktoken gettoken groovy honey mailauth proxy timeauth falsh ccs-notifyd force-logout audit-exec-param convert-exec-param ccs-editpolicy-agent

falsh_LDADD= -lncurses -lreadline

ALIAS_LIST = ccs-auditd ccs-queryd ccstree checkpolicy editpolicy findtemp ld-watch loadpolicy pathmatch patternize savepolicy setlevel setprofile sortpolicy

SBIN_ALIAS_LIST = $(ALIAS_LIST) domainmatch

install-data-hook:
	cd $(DESTDIR)$(ccsdir) && \
	for i in $(ALIAS_LIST); do rm -f $$i; $(LN_S) ccstools $$i; done
	@mkdir_p@ $(DESTDIR)$(sbindir) && \
	cd $(DESTDIR)$(sbindir) && \
	for i in $(SBIN_ALIAS_LIST); do rm -f ccs-`echo $$i | sed 's:^ccs-::'`; $(LN_S) $(ccsdir)/$$i ccs-`echo $$i | sed 's:^ccs-::'`; done
