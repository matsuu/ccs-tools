#! /bin/sh
#
# Initial policy generator.
#
# Copyright (C) 2005-2008  NTT DATA CORPORATION
#
# Version: 2.2.0-pre   2008/06/25
#

cd ${0%/*}
set -f
export PATH=$PWD:/sbin:/bin:${PATH}

PROFILE_TYPE="--full-profile"

while [ $# -gt 0 ]
do
	case "$1" in
	--file-only-profile|--full-profile)
		PROFILE_TYPE="$1"
		;;
	esac
	shift
done

# Verify realpath works.
if ! realpath -n / > /dev/null; then
    echo "Can't execute program. Please make sure you installed correct package."
    exit 1
fi

if ! pathmatch / > /dev/null; then
    echo "Can't execute program. Please make sure you installed correct package."
    exit 1
fi

if ! which which > /dev/null; then
    echo "Can't execute 'which' command. Please install package containing 'which' command."
    exit 1
fi

if ! find . > /dev/null; then
    echo "Can't execute 'find' command. Please install package containing 'find' command."
    exit 1
fi

make_exception() {
	#
	# Make /sbin/modprobe and /sbin/hotplug as initializers, for they can be called by untrusted programs.
	#
	for i in `cat /proc/sys/kernel/modprobe /proc/sys/kernel/hotplug`
	do
		FILE=`realpath $i`
		[ -n "$FILE" -a -f "$FILE" -a -x "$FILE" -a ! -L "$FILE" ] && echo 'initialize_domain '$FILE
	done
	#
	# Make patterns for /proc/[number]/ and /proc/self/ directory.
	#
	for i in `find /proc/1/ /proc/self/ -type f 2> /dev/null`
	do
		echo "file_pattern "$i | sed -e 's@/[0-9]*/@/\\$/@g' -e 's@/[0-9]*$@/\\$@'
	done | sort | uniq | grep -F '\'
	
	#
	# Make patterns for /sys/ directory.
	#
	#if [ -e /sys/block/ ]
	#then
	#    set +f
	#    DIRS=`echo /sys/*`
	#    set -f
	#    for DIR in $DIRS
	#    do
	#	for i in `find $DIR | awk -F / ' { print NF-1 }'`
	#	do
	#	    echo -n "file_pattern "$DIR; for j in `seq 3 $i`; do echo -n '/\*'; done; echo
	#	done
	#    done | grep -F '\' | sort | uniq
	#fi
	
	#
	# Make patterns for /dev/ directory.
	#
	echo 'file_pattern /dev/pts/\$'
	echo 'file_pattern /dev/vc/\$'
	echo 'file_pattern /dev/tty\$'
	
	#
	# Make patterns for policy directory.
	#
	echo 'file_pattern /etc/tomoyo/system_policy.\$-\$-\$.\$:\$:\$.conf'
	echo 'file_pattern /etc/tomoyo/exception_policy.\$-\$-\$.\$:\$:\$.conf'
	echo 'file_pattern /etc/tomoyo/domain_policy.\$-\$-\$.\$:\$:\$.conf'
	
	#
	# Make patterns for man directory.
	#
	[ -d /usr/share/man/ ] && for i in `find /usr/share/man/ -type f | awk -F / ' { print NF }' | sort | uniq`
	do
		echo -n "file_pattern /usr/share/man"; for j in `seq 5 $i`; do echo -n '/\*'; done; echo
	done
      
	[ -d /usr/X11R6/man/ ] && for i in `find /usr/X11R6/man/ -type f | awk -F / ' { print NF }' | sort | uniq`
	do
		echo -n "file_pattern /usr/X11R6/man"; for j in `seq 5 $i`; do echo -n '/\*'; done; echo
	done
	
	#
	# Make patterns for spool directory. (No trailing /, or detecting symlink fails.)
	#
	for i in /var/spool/clientmqueue /var/spool/mail /var/spool/mqueue /var/spool/at /var/spool/exim4/msglog /var/spool/exim4/input /var/spool/cron/atjobs /var/spool/postfix/maildrop /var/spool/postfix/incoming /var/spool/postfix/active /var/spool/postfix/bounce
	do
		[ -d $i/ -a ! -L $i ] && echo 'file_pattern '$i'/\*'
	done
	[ -d /var/spool/postfix/ ] && echo 'file_pattern /var/spool/postfix/deferred/\x/'
	[ -d /var/spool/postfix/ ] && echo 'file_pattern /var/spool/postfix/deferred/\x/\X'
	
	#
	# Make patterns for man(1).
	#
	echo 'file_pattern /tmp/man.\?\?\?\?\?\?'
      
	# Make patterns for mount(8).
	echo 'file_pattern /etc/mtab~\$'
	
	#
	# Make patterns for crontab(1).
	#
	grep -qF "Red Hat Linux" /etc/issue && echo 'file_pattern /tmp/crontab.\$'                     # RHL9
	grep -qF "Fedora Core" /etc/issue && echo 'file_pattern /tmp/crontab.XXXX\?\?\?\?\?\?'         # FC3
	grep -qF "Debian" /etc/issue && echo 'file_pattern /tmp/crontab.\?\?\?\?\?\?/crontab'          # Sarge
	
	#
	# Allow reading some data files.
	#
	for i in /etc/ld.so.cache /proc/meminfo /proc/sys/kernel/version /etc/localtime /usr/lib/gconv/gconv-modules.cache /usr/share/locale/locale.alias
	do
		FILE=`realpath $i`
		[ -n "$FILE" -a -r "$FILE" -a ! -L "$FILE" ] && echo 'allow_read '$FILE
	done
	set -f
	for dir in `realpath -n /usr/share/` `realpath -n /usr/lib/`
	  do
	  if [ -d $dir ]; then
        # Allow reading font files.
	      for i in `find $dir -type d -name '*fonts*'`
		do
		for j in '/\*' '/\*/\*' '/\*/\*/\*' '/\*/\*/\*/\*' '/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*/\*'
		  do
		  pathmatch $i$j | grep -q / && echo 'allow_read '$i$j
		done
	      done
        # Allow reading icon files.
	      for i in `find $dir -type d -name '*icons*'`
		do
		for j in '/\*' '/\*/\*' '/\*/\*/\*' '/\*/\*/\*/\*' '/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*/\*'
		  do
		  pathmatch $i$j | grep -q / && echo 'allow_read '$i$j
		done
	      done
        # Allow reading locale files.
	      for i in `find $dir -type d -name 'locale'`
		do
		for j in '/\*' '/\*/\*' '/\*/\*/\*' '/\*/\*/\*/\*' '/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*/\*'
		  do
		  pathmatch $i$j | grep -q / && echo 'allow_read '$i$j
		done
	      done
	      for i in `find $dir -type d -name 'locales'`
		do
		for j in '/\*' '/\*/\*' '/\*/\*/\*' '/\*/\*/\*/\*' '/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*/\*'
		  do
		  pathmatch $i$j | grep -q / && echo 'allow_read '$i$j
		done
	      done
	      for i in `find $dir -type d -name 'locale-langpack'`
		do
		for j in '/\*' '/\*/\*' '/\*/\*/\*' '/\*/\*/\*/\*' '/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*' '/\*/\*/\*/\*/\*/\*/\*/\*/\*'
		  do
		  pathmatch $i$j | grep -q / && echo 'allow_read '$i$j
		done
	      done
	  fi
	done

	#
	# Allow reading information for current process.
	#
	for i in `find /proc/self/ -type f 2> /dev/null`
	do
		echo 'allow_read '$i | sed -e 's@/[0-9]*/@/\\$/@g' -e 's@/[0-9]*$@/\\$@'
	done | sort | uniq
	
	#
	# Allow reading DLL files registered with ldconfig(8).
	#
	for i in `ldconfig -NXp | grep -F '=>' | awk ' { print $NF } ' | sort | uniq`
	do
		FILE=`realpath $i`
		[ -n "$FILE" -a -s "$FILE" -a ! -L "$FILE" ] && echo 'allow_read '$FILE
	done | sort | uniq
	
	#
	# Mark programs under /etc/init.d/ directory as initializer.
	#
	DIR=`realpath /etc/init.d/`
	if [ -n "$DIR" ]; then
	    set +f
	    FILES=`echo $DIR/*`
	    set -f
	    for FILE in $FILES
	    do
		[ -n "$FILE" -a -f "$FILE" -a -x "$FILE" ] && echo "initialize_domain "$FILE
	    done
	fi
	
	#
	# Mark some programs that you want to assign short domainname as initializer.
	#
	# You can add as you like to the list below.
	#
	for FILE in /sbin/cardmgr /sbin/getty /sbin/init /sbin/klogd /sbin/mingetty /sbin/portmap /sbin/rpc.statd /sbin/syslogd /sbin/udevd /usr/X11R6/bin/xfs /usr/bin/dbus-daemon-1 /usr/bin/jserver /usr/bin/mDNSResponder /usr/bin/nifd /usr/bin/spamd /usr/sbin/acpid /usr/sbin/afpd /usr/sbin/anacron /usr/sbin/apache2 /usr/sbin/apmd /usr/sbin/atalkd /usr/sbin/atd /usr/sbin/cannaserver /usr/sbin/cpuspeed /usr/sbin/cron /usr/sbin/crond /usr/sbin/cupsd /usr/sbin/dhcpd /usr/sbin/exim4 /usr/sbin/gpm /usr/sbin/hald /usr/sbin/htt /usr/sbin/httpd /usr/sbin/inetd /usr/sbin/logrotate /usr/sbin/lpd /usr/sbin/nmbd /usr/sbin/papd /usr/sbin/rpc.idmapd /usr/sbin/rpc.mountd /usr/sbin/rpc.rquotad /usr/sbin/sendmail.sendmail /usr/sbin/smartd /usr/sbin/smbd /usr/sbin/squid /usr/sbin/sshd /usr/sbin/vmware-guestd /usr/sbin/vsftpd /usr/sbin/xinetd
	do
		FILE=`realpath $FILE 2> /dev/null`
		[ -n "$FILE" -a -f "$FILE" -a -x "$FILE" -a ! -L "$FILE" ] && echo 'initialize_domain '$FILE
	done | sort | uniq
	
	#
	# Make patterns for unnamed pipes and sockets.
	#
	echo 'file_pattern pipe:[\$]'
	echo 'file_pattern socket:[\$]'
	
	#
	# Make patterns for emacs(1).
	#
	[ -d /root/.emacs.d/ ] && echo 'file_pattern /root/.emacs.d/auto-save-list/.saves-\$-\*'
	
	#
	# Make patterns for mh-rmail from emacs(1).
	#
	[ -d /root/Mail/inbox/ ] && echo 'file_pattern /root/Mail/inbox/\$'
      
	#
	# Make patterns for ksymoops(8).
	#
	[ -d /var/log/ksymoops/ ] && echo 'file_pattern /var/log/ksymoops/\*'
      
	#
	# Make patterns for squid(8).
	#
	if [ -d /var/spool/squid/ ]; then
		echo 'file_pattern /var/spool/squid/\*/'
		echo 'file_pattern /var/spool/squid/\*/\*/'
		echo 'file_pattern /var/spool/squid/\*/\*/\*'
	fi
	
	#
	# Make patterns for spamd(1).
	#
	SPAMD_PATH=`which spamd 2> /dev/null`
	if [ -n "$SPAMD_PATH" ]; then
		if grep -qF '/tmp/spamassassin-$$' $SPAMD_PATH; then
			echo 'file_pattern /tmp/spamassassin-\$/'
			echo 'file_pattern /tmp/spamassassin-\$/.spamassassin/'
			echo 'file_pattern /tmp/spamassassin-\$/.spamassassin/auto-whitelist\*'
		fi
		if grep -qF 'spamd-$$-init' $SPAMD_PATH; then
			echo 'file_pattern /tmp/spamd-\$-init/'
			echo 'file_pattern /tmp/spamd-\$-init/.spamassassin/'
			echo 'file_pattern /tmp/spamd-\$-init/.spamassassin/\*'
		fi
	fi
	
	#
	# Make patterns for mail(1).
	#
	MAIL_PATH=`which mail 2> /dev/null`
	if [ -n "$MAIL_PATH" ]; then
		grep -qF '/mail.XXXXXX'       $MAIL_PATH && echo 'file_pattern /tmp/mail.\?\?\?\?\?\?'
		grep -qF '/mail.RsXXXXXXXXXX' $MAIL_PATH && echo 'file_pattern /tmp/mail.RsXXXX\?\?\?\?\?\?'
		grep -qF '/mail.ReXXXXXXXXXX' $MAIL_PATH && echo 'file_pattern /tmp/mail.ReXXXX\?\?\?\?\?\?'
		grep -qF '/mail.XXXXXXXXXX'   $MAIL_PATH && echo 'file_pattern /tmp/mail.XXXX\?\?\?\?\?\?'
		grep -qF '/mail.RxXXXXXXXXXX' $MAIL_PATH && echo 'file_pattern /tmp/mail.RxXXXX\?\?\?\?\?\?'
		grep -qF '/mail.RmXXXXXXXXXX' $MAIL_PATH && echo 'file_pattern /tmp/mail.RmXXXX\?\?\?\?\?\?'
		grep -qF '/mail.RqXXXXXXXXXX' $MAIL_PATH && echo 'file_pattern /tmp/mail.RqXXXX\?\?\?\?\?\?'
		echo 'file_pattern /tmp/Rs\?\?\?\?\?\?'
		echo 'file_pattern /tmp/Rq\?\?\?\?\?\?'
		echo 'file_pattern /tmp/Rm\?\?\?\?\?\?'
		echo 'file_pattern /tmp/Re\?\?\?\?\?\?'
		echo 'file_pattern /tmp/Rx\?\?\?\?\?\?'
	fi
	
	#
	# Make patterns for udev(8).
	#
	if [ -d /dev/.udev/ ]; then
	    echo 'file_pattern /dev/.udev/\*'
	    echo 'file_pattern /dev/.udev/\*/'
	    echo 'file_pattern /dev/.udev/\*/\*'
	    echo 'file_pattern /dev/.udev/\*/\*/'
	    echo 'file_pattern /dev/.udev/\*/\*/\*'
	    echo 'file_pattern /dev/.udev/\*/\*/\*/'
	    echo 'file_pattern /dev/.udev/\*/\*/\*/\*'
	    echo 'file_pattern /dev/.udev/\*/\*/\*/\*/'
	    echo 'file_pattern /dev/.udev/\*/\*/\*/\*/\*'
	fi
	[ -d /dev/.udevdb/ ] && echo 'file_pattern /dev/.udevdb/\*'
	
	#
	# Make patterns for sh(1).
	#
	grep -qF sh-thd /bin/sh && echo 'file_pattern /tmp/sh-thd-\$'
	
	#
	# Make patterns for smbd(8).
	#
	[ -d /var/log/samba/ ]            && echo 'file_pattern /var/log/samba/\*'
	
	#
	# Make patterns for blkid(8).
	#
	[ -f /etc/blkid.tab ]             && echo 'file_pattern /etc/blkid.tab-\?\?\?\?\?\?'
	[ -f /etc/blkid/blkid.tab ]       && echo 'file_pattern /etc/blkid/blkid.tab-\?\?\?\?\?\?'
	
	#
	# Make patterns for gpm(8).
	#
	GPM_PATH=`which gpm 2> /dev/null`
	[ -n "$GPM_PATH" ] && grep -qF '/gpmXXXXXX' $GPM_PATH && echo 'file_pattern /var/run/gpm\?\?\?\?\?\?'
	
	#
	# Make patterns for mrtg(1).
	#
	[ -d /etc/mrtg/ ]                 && echo 'file_pattern /etc/mrtg/mrtg.cfg_l_\$'
	[ -d /var/lock/mrtg/ ]            && echo 'file_pattern /var/lock/mrtg/mrtg_l_\$'
	
	#
	# Make patterns for autofs(8).
	#
	[ -x /etc/init.d/autofs ] && grep -qF '/tmp/autofs.XXXXXX' /etc/init.d/autofs && echo 'file_pattern /tmp/autofs.\?\?\?\?\?\?'
	
	#
	# Make patterns for dhcpd(8).
	#
	[ -f /var/lib/dhcp/dhcpd.leases ] && echo 'file_pattern /var/lib/dhcp/dhcpd.leases.\$'
	
	#
	# Make patterns for mlocate(1).
	#
	[ -d /var/lib/mlocate/ ]          && echo 'file_pattern /var/lib/mlocate/mlocate.db.\?\?\?\?\?\?'
	
	#
	# Make patterns for mailman.
	#
	[ -d /var/mailman/locks/ ]        && echo 'file_pattern /var/mailman/locks/gate_news.lock.\*'
	
	#
	# Make patterns for makewhatis(8).
	#
	MAKEWHATIS_PATH=`which makewhatis 2> /dev/null`
	if [ -n "$MAKEWHATIS_PATH" ]; then
		if grep -qF '/tmp/makewhatisXXXXXX' $MAKEWHATIS_PATH; then
			echo 'file_pattern /tmp/makewhatis\?\?\?\?\?\?/'
			echo 'file_pattern /tmp/makewhatis\?\?\?\?\?\?/w'
		fi
		if grep -qF '/tmp/whatis.XXXXXX' $MAKEWHATIS_PATH; then
			echo 'file_pattern /tmp/whatis.\?\?\?\?\?\?'
		fi
	fi
	
	#
	# Make patterns for automount(8).
	#
	AUTOMOUNT_PATH=`which automount 2> /dev/null`
	if [ -n "$AUTOMOUNT_PATH" ]; then
		if grep -qF '/var/lock/autofs' $AUTOMOUNT_PATH; then
			echo 'file_pattern /var/lock/autofs.\$'
		fi
		echo 'file_pattern /tmp/auto\?\?\?\?\?\?/'
	fi
	
	#
	# Make patterns for logwatch(8).
	#
	LOGWATCH_PATH=`which logwatch 2> /dev/null`
	if [ -n "$LOGWATCH_PATH" ]; then
		if grep -qF '/var/cache/logwatch' $LOGWATCH_PATH; then
			echo 'file_pattern /var/cache/logwatch/logwatch.XX\?\?\?\?\?\?/'
			echo 'file_pattern /var/cache/logwatch/logwatch.XX\?\?\?\?\?\?/\*'
		else
			echo 'file_pattern /tmp/logwatch.XX\?\?\?\?\?\?/'
			echo 'file_pattern /tmp/logwatch.XX\?\?\?\?\?\?/\*'
		fi
	fi
	
	#
	# Make patterns for logrotate(8).
	#
	LOGROTATE_PATH=`which logrotate 2> /dev/null`
	if [ -n "$LOGROTATE_PATH" ]; then
		if grep -qF '/logrotate.XXXXXX' $LOGROTATE_PATH; then
			echo 'file_pattern /tmp/logrotate.\?\?\?\?\?\?'
			echo 'aggregator /tmp/logrotate.\?\?\?\?\?\? /tmp/logrotate.tmp'
		fi
	fi
	
	#
	# Make patterns for cardmgr(8).
	#
	CARDMGR_PATH=`which cardmgr 2> /dev/null`
	if [ -n "$CARDMGR_PATH" ]; then
		if grep -qF '%s/cm-%d-%d' $CARDMGR_PATH; then
			echo 'file_pattern /var/lib/pcmcia/cm-\$-\$'
		fi
	fi
	
	#
	# Make patterns for anacron(8).
	#
	ANACRON_PATH=`which anacron 2> /dev/null`
	if [ -n "$ANACRON_PATH" ]; then
		echo 'file_pattern /tmp/file\?\?\?\?\?\?'
	fi
	
	#
	# Make patterns for run-crons(?).
	#
	if [ -x /usr/lib/cron/run-crons ] && grep -qF '/tmp/run-crons.XXXXXX' /usr/lib/cron/run-crons; then
		echo 'file_pattern /tmp/run-crons.\?\?\?\?\?\?/'
		echo 'file_pattern /tmp/run-crons.\?\?\?\?\?\?/run-crons.\*'
	fi
	
	#
	# Make patterns for postgresql.
	#
	if [ -d /var/lib/pgsql/ ]; then
		echo 'file_pattern /var/lib/pgsql/data/base/\$/'
		echo 'file_pattern /var/lib/pgsql/data/base/\$/\$'
		echo 'file_pattern /var/lib/pgsql/data/base/global/pg_database.\$'
		echo 'file_pattern /var/lib/pgsql/data/base/\$/pg_internal.init.\$'
		echo 'file_pattern /var/lib/pgsql/data/base/\$/pg_internal.init'
		echo 'file_pattern /var/lib/pgsql/data/base/pgsql_tmp/pgsql_tmp\*'
		echo 'file_pattern /var/lib/pgsql/data/base/\$/PG_VERSION'
		echo 'file_pattern /var/lib/pgsql/data/global/\$'
		echo 'file_pattern /var/lib/pgsql/data/global/pg_auth.\$'
		echo 'file_pattern /var/lib/pgsql/data/global/pg_database.\$'
		echo 'file_pattern /var/lib/pgsql/data/pg_clog/\X'
		echo 'file_pattern /var/lib/pgsql/data/pg_multixact/members/\X'
		echo 'file_pattern /var/lib/pgsql/data/pg_multixact/offsets/\X'
		echo 'file_pattern /var/lib/pgsql/data/pg_subtrans/\X'
		echo 'file_pattern /var/lib/pgsql/data/pg_tblspc/\$'
		echo 'file_pattern /var/lib/pgsql/data/pg_twophase/\X'
		echo 'file_pattern /var/lib/pgsql/data/pg_xlog/\X'
		echo 'file_pattern /var/lib/pgsql/data/pg_xlog/xlogtemp.\$'
	fi
	if [ -d /var/lib/postgres/ ]; then
		echo 'file_pattern /var/lib/postgres/data/base/\$/'
		echo 'file_pattern /var/lib/postgres/data/base/\$/\$'
		echo 'file_pattern /var/lib/postgres/data/global/\$'
		echo 'file_pattern /var/lib/postgres/data/global/pgstat.tmp.\$'
		echo 'file_pattern /var/lib/postgres/data/pg_clog/\X'
		echo 'file_pattern /var/lib/postgres/data/pg_xlog/\X'
	fi
	if [ -d /var/lib/postgresql/ ]; then
		echo 'file_pattern /var/lib/postgresql/\*/main/base/\$/'
		echo 'file_pattern /var/lib/postgresql/\*/main/base/\$/\$'
		echo 'file_pattern /var/lib/postgresql/\*/main/base/\$/pg_internal.init.\$'
		echo 'file_pattern /var/lib/postgresql/\*/main/base/\$/PG_VERSION'
		echo 'file_pattern /var/lib/postgresql/\*/main/global/\$'
		echo 'file_pattern /var/lib/postgresql/\*/main/global/\$/pg_auth.\$'
		echo 'file_pattern /var/lib/postgresql/\*/main/global/\$/pg_database.\$'
		echo 'file_pattern /var/lib/postgresql/\*/main/pg_clog/\X'
		echo 'file_pattern /var/lib/postgresql/\*/main/pg_multixact/members/\X'
		echo 'file_pattern /var/lib/postgresql/\*/main/pg_multixact/offsets/\X'
		echo 'file_pattern /var/lib/postgresql/\*/main/pg_subtrans/\X'
		echo 'file_pattern /var/lib/postgresql/\*/main/pg_xlog/\X'
		echo 'file_pattern /var/lib/postgresql/\*/main/pg_xlog/xlogtemp.\$'
	fi
	
	#
	# Miscellaneous patterns.
	#
	if grep -qF "Red Hat Linux" /etc/issue; then
		[ -d /var/log/sa/ ]               && echo 'file_pattern /var/log/sa/sa\*'
		echo 'file_pattern /tmp/man.\?\?\?\?\?\?'
		echo 'file_pattern /tmp/file.\?\?\?\?\?\?'
	fi
	
	if grep -qF "Fedora Core" /etc/issue || grep -qF "CentOS" /etc/issue ; then
		echo 'file_pattern /etc/.fstab.hal.\?'
		echo 'file_pattern /tmp/file\?\?\?\?\?\?'
	fi
	
	if grep -qF "Debian" /etc/issue; then
		echo 'file_pattern /tmp/ex4\?\?\?\?\?\?'
		echo 'file_pattern /tmp/tmpf\?\?\?\?\?\?'
		echo 'file_pattern /tmp/zcat\?\?\?\?\?\?'
		echo 'file_pattern /tmp/zman\?\?\?\?\?\?'
		echo 'file_pattern /var/cache/man/\$'
		echo 'file_pattern /var/cache/man/\*/\$'
		echo 'file_pattern /root/mbox.XXXX\?\?\?\?\?\?'
	fi
	
	if grep -qF "SUSE LINUX 10" /etc/issue; then
		echo 'file_pattern /tmp/used_interface_names.\*'
		echo 'file_pattern /var/run/fence\?\?\?\?\?\?'
		echo 'file_pattern /dev/shm/sysconfig/tmp/if-lo.\$'
		echo 'file_pattern /dev/shm/sysconfig/tmp/if-lo.\$.tmp'
		echo 'file_pattern /dev/shm/sysconfig/tmp/if-eth0.\$'
		echo 'file_pattern /dev/shm/sysconfig/tmp/if-eth0.\$.tmp'
		echo 'file_pattern /var/run/nscd/db\?\?\?\?\?\?'
	fi
	
	if [ -d /var/lib/init.d/ ]; then
	    echo 'file_pattern /var/lib/init.d/mtime-test.\$'
	    echo 'file_pattern /var/lib/init.d/exclusive/\*.\$'
	    echo 'file_pattern /var/lib/init.d/depcache.\?\?\?\?\?\?\?'
	    echo 'file_pattern /var/lib/init.d/treecache.\?\?\?\?\?\?\?'
	fi
	
	echo 'file_pattern /tmp/ib\?\?\?\?\?\?'
	echo 'file_pattern /tmp/PerlIO_\?\?\?\?\?\?'
	[ -d /var/run/hald/ ] && echo 'file_pattern /var/run/hald/acl-list.\?\?\?\?\?\?'
	if [ -d /usr/share/zoneinfo/ ]; then
		echo 'file_pattern /usr/share/zoneinfo/\*'
		echo 'file_pattern /usr/share/zoneinfo/\*/\*'
		echo 'file_pattern /usr/share/zoneinfo/\*/\*/\*'
		echo 'file_pattern /usr/share/zoneinfo/\*/\*/\*/\*'
	fi
	
	#
	# Make /var/log/ directory not rewritable by default.
	#
	[ -d /var/log/ ] && for i in `find /var/log/ -type f | awk -F / ' { print NF }' | sort | uniq`
	do
		echo -n "deny_rewrite /var/log"; for j in `seq 4 $i`; do echo -n '/\*'; done; echo
	done
}

make_alias() {
	for MNT in `df 2> /dev/null | awk ' { print $NF } ' | grep / | sort | uniq`
	do
		[ -d $MNT ] && for SYMLINK in `find $MNT -xdev -type l 2> /dev/null`
		do
			
			# Solve symbolic name.
			ENTITY=`realpath -n $SYMLINK`
			
			# Reject if it is not a regular file.
			[ -f "$ENTITY" -a -x "$ENTITY" ] || continue
			
			# Reject if basename is the same. 
			F1=${ENTITY##*/}
			F2=${SYMLINK##*/}
			[ $F1 = $F2 ] && continue

			# Reject if file is not executable. 
			file $ENTITY | grep -q executable || continue
			
			# Exclude /etc/rc?.d/ directory.
			echo $F2 | grep -q '^[SK][0-9][0-9]' && continue
			
			# This is a candidate.
			echo 'alias '$ENTITY' '$SYMLINK
		done
	done | sort | uniq
}

if [ ! -d /etc/tomoyo/ ]; then
	echo Creating policy directory.
	mkdir -p /etc/tomoyo
fi
chmod 700 /etc/tomoyo/
chown root:root /etc/tomoyo/

if [ ! -r /etc/tomoyo/manager.conf ]; then
	echo Creating manager policy.
	ccs_libdir=`realpath /usr/lib/ccs`
	echo ${ccs_libdir}/loadpolicy  > /etc/tomoyo/manager.conf
	echo ${ccs_libdir}/editpolicy >> /etc/tomoyo/manager.conf
	echo ${ccs_libdir}/setlevel   >> /etc/tomoyo/manager.conf
	echo ${ccs_libdir}/setprofile >> /etc/tomoyo/manager.conf
	echo ${ccs_libdir}/ld-watch   >> /etc/tomoyo/manager.conf
	echo ${ccs_libdir}/ccs-queryd >> /etc/tomoyo/manager.conf
fi

if [ ! -r /etc/tomoyo/profile.conf ]; then
	echo Creating default profile.
	case "$PROFILE_TYPE" in
	--file-only-profile)
		cat > /etc/tomoyo/profile.conf << EOF
0-COMMENT=-----Disabled Mode-----
0-MAC_FOR_FILE=disabled
0-TOMOYO_VERBOSE=disabled
1-COMMENT=-----Learning Mode-----
1-MAC_FOR_FILE=learning
1-TOMOYO_VERBOSE=disabled
2-COMMENT=-----Permissive Mode-----
2-MAC_FOR_FILE=permissive
2-TOMOYO_VERBOSE=enabled
3-COMMENT=-----Enforcing Mode-----
3-MAC_FOR_FILE=enforcing
3-TOMOYO_VERBOSE=enabled
EOF
		;;
	*)
		cat > /etc/tomoyo/profile.conf << EOF
0-COMMENT=-----Disabled Mode-----
0-MAC_FOR_FILE=disabled
0-MAC_FOR_ARGV0=disabled
0-MAC_FOR_ENV=disabled
0-MAC_FOR_NETWORK=disabled
0-MAC_FOR_SIGNAL=disabled
0-DENY_CONCEAL_MOUNT=disabled
0-RESTRICT_CHROOT=disabled
0-RESTRICT_MOUNT=disabled
0-RESTRICT_UNMOUNT=disabled
0-RESTRICT_PIVOT_ROOT=disabled
0-RESTRICT_AUTOBIND=disabled
0-MAX_ACCEPT_ENTRY=2048
0-MAX_GRANT_LOG=1024
0-MAX_REJECT_LOG=1024
0-TOMOYO_VERBOSE=enabled
0-SLEEP_PERIOD=0
0-MAC_FOR_CAPABILITY::inet_tcp_create=disabled
0-MAC_FOR_CAPABILITY::inet_tcp_listen=disabled
0-MAC_FOR_CAPABILITY::inet_tcp_connect=disabled
0-MAC_FOR_CAPABILITY::use_inet_udp=disabled
0-MAC_FOR_CAPABILITY::use_inet_ip=disabled
0-MAC_FOR_CAPABILITY::use_route=disabled
0-MAC_FOR_CAPABILITY::use_packet=disabled
0-MAC_FOR_CAPABILITY::SYS_MOUNT=disabled
0-MAC_FOR_CAPABILITY::SYS_UMOUNT=disabled
0-MAC_FOR_CAPABILITY::SYS_REBOOT=disabled
0-MAC_FOR_CAPABILITY::SYS_CHROOT=disabled
0-MAC_FOR_CAPABILITY::SYS_KILL=disabled
0-MAC_FOR_CAPABILITY::SYS_VHANGUP=disabled
0-MAC_FOR_CAPABILITY::SYS_TIME=disabled
0-MAC_FOR_CAPABILITY::SYS_NICE=disabled
0-MAC_FOR_CAPABILITY::SYS_SETHOSTNAME=disabled
0-MAC_FOR_CAPABILITY::use_kernel_module=disabled
0-MAC_FOR_CAPABILITY::create_fifo=disabled
0-MAC_FOR_CAPABILITY::create_block_dev=disabled
0-MAC_FOR_CAPABILITY::create_char_dev=disabled
0-MAC_FOR_CAPABILITY::create_unix_socket=disabled
0-MAC_FOR_CAPABILITY::SYS_LINK=disabled
0-MAC_FOR_CAPABILITY::SYS_SYMLINK=disabled
0-MAC_FOR_CAPABILITY::SYS_RENAME=disabled
0-MAC_FOR_CAPABILITY::SYS_UNLINK=disabled
0-MAC_FOR_CAPABILITY::SYS_CHMOD=disabled
0-MAC_FOR_CAPABILITY::SYS_CHOWN=disabled
0-MAC_FOR_CAPABILITY::SYS_IOCTL=disabled
0-MAC_FOR_CAPABILITY::SYS_KEXEC_LOAD=disabled
0-MAC_FOR_CAPABILITY::SYS_PIVOT_ROOT=disabled
0-MAC_FOR_CAPABILITY::SYS_PTRACE=disabled
1-COMMENT=-----Learning Mode-----
1-MAC_FOR_FILE=learning
1-MAC_FOR_ARGV0=learning
1-MAC_FOR_ENV=learning
1-MAC_FOR_NETWORK=learning
1-MAC_FOR_SIGNAL=learning
1-DENY_CONCEAL_MOUNT=permissive
1-RESTRICT_CHROOT=learning
1-RESTRICT_MOUNT=learning
1-RESTRICT_UNMOUNT=permissive
1-RESTRICT_PIVOT_ROOT=learning
1-RESTRICT_AUTOBIND=enabled
1-MAX_ACCEPT_ENTRY=2048
1-MAX_GRANT_LOG=1024
1-MAX_REJECT_LOG=1024
1-TOMOYO_VERBOSE=disabled
1-SLEEP_PERIOD=0
1-MAC_FOR_CAPABILITY::inet_tcp_create=learning
1-MAC_FOR_CAPABILITY::inet_tcp_listen=learning
1-MAC_FOR_CAPABILITY::inet_tcp_connect=learning
1-MAC_FOR_CAPABILITY::use_inet_udp=learning
1-MAC_FOR_CAPABILITY::use_inet_ip=learning
1-MAC_FOR_CAPABILITY::use_route=learning
1-MAC_FOR_CAPABILITY::use_packet=learning
1-MAC_FOR_CAPABILITY::SYS_MOUNT=learning
1-MAC_FOR_CAPABILITY::SYS_UMOUNT=learning
1-MAC_FOR_CAPABILITY::SYS_REBOOT=learning
1-MAC_FOR_CAPABILITY::SYS_CHROOT=learning
1-MAC_FOR_CAPABILITY::SYS_KILL=learning
1-MAC_FOR_CAPABILITY::SYS_VHANGUP=learning
1-MAC_FOR_CAPABILITY::SYS_TIME=learning
1-MAC_FOR_CAPABILITY::SYS_NICE=learning
1-MAC_FOR_CAPABILITY::SYS_SETHOSTNAME=learning
1-MAC_FOR_CAPABILITY::use_kernel_module=learning
1-MAC_FOR_CAPABILITY::create_fifo=learning
1-MAC_FOR_CAPABILITY::create_block_dev=learning
1-MAC_FOR_CAPABILITY::create_char_dev=learning
1-MAC_FOR_CAPABILITY::create_unix_socket=learning
1-MAC_FOR_CAPABILITY::SYS_LINK=learning
1-MAC_FOR_CAPABILITY::SYS_SYMLINK=learning
1-MAC_FOR_CAPABILITY::SYS_RENAME=learning
1-MAC_FOR_CAPABILITY::SYS_UNLINK=learning
1-MAC_FOR_CAPABILITY::SYS_CHMOD=learning
1-MAC_FOR_CAPABILITY::SYS_CHOWN=learning
1-MAC_FOR_CAPABILITY::SYS_IOCTL=learning
1-MAC_FOR_CAPABILITY::SYS_KEXEC_LOAD=learning
1-MAC_FOR_CAPABILITY::SYS_PIVOT_ROOT=learning
1-MAC_FOR_CAPABILITY::SYS_PTRACE=learning
2-COMMENT=-----Permissive Mode-----
2-MAC_FOR_FILE=permissive
2-MAC_FOR_ARGV0=permissive
2-MAC_FOR_ENV=permissive
2-MAC_FOR_NETWORK=permissive
2-MAC_FOR_SIGNAL=permissive
2-DENY_CONCEAL_MOUNT=permissive
2-RESTRICT_CHROOT=permissive
2-RESTRICT_MOUNT=permissive
2-RESTRICT_UNMOUNT=permissive
2-RESTRICT_PIVOT_ROOT=permissive
2-RESTRICT_AUTOBIND=enabled
2-MAX_ACCEPT_ENTRY=2048
2-MAX_GRANT_LOG=1024
2-MAX_REJECT_LOG=1024
2-TOMOYO_VERBOSE=enabled
2-SLEEP_PERIOD=0
2-MAC_FOR_CAPABILITY::inet_tcp_create=permissive
2-MAC_FOR_CAPABILITY::inet_tcp_listen=permissive
2-MAC_FOR_CAPABILITY::inet_tcp_connect=permissive
2-MAC_FOR_CAPABILITY::use_inet_udp=permissive
2-MAC_FOR_CAPABILITY::use_inet_ip=permissive
2-MAC_FOR_CAPABILITY::use_route=permissive
2-MAC_FOR_CAPABILITY::use_packet=permissive
2-MAC_FOR_CAPABILITY::SYS_MOUNT=permissive
2-MAC_FOR_CAPABILITY::SYS_UMOUNT=permissive
2-MAC_FOR_CAPABILITY::SYS_REBOOT=permissive
2-MAC_FOR_CAPABILITY::SYS_CHROOT=permissive
2-MAC_FOR_CAPABILITY::SYS_KILL=permissive
2-MAC_FOR_CAPABILITY::SYS_VHANGUP=permissive
2-MAC_FOR_CAPABILITY::SYS_TIME=permissive
2-MAC_FOR_CAPABILITY::SYS_NICE=permissive
2-MAC_FOR_CAPABILITY::SYS_SETHOSTNAME=permissive
2-MAC_FOR_CAPABILITY::use_kernel_module=permissive
2-MAC_FOR_CAPABILITY::create_fifo=permissive
2-MAC_FOR_CAPABILITY::create_block_dev=permissive
2-MAC_FOR_CAPABILITY::create_char_dev=permissive
2-MAC_FOR_CAPABILITY::create_unix_socket=permissive
2-MAC_FOR_CAPABILITY::SYS_LINK=permissive
2-MAC_FOR_CAPABILITY::SYS_SYMLINK=permissive
2-MAC_FOR_CAPABILITY::SYS_RENAME=permissive
2-MAC_FOR_CAPABILITY::SYS_UNLINK=permissive
2-MAC_FOR_CAPABILITY::SYS_CHMOD=permissive
2-MAC_FOR_CAPABILITY::SYS_CHOWN=permissive
2-MAC_FOR_CAPABILITY::SYS_IOCTL=permissive
2-MAC_FOR_CAPABILITY::SYS_KEXEC_LOAD=permissive
2-MAC_FOR_CAPABILITY::SYS_PIVOT_ROOT=permissive
2-MAC_FOR_CAPABILITY::SYS_PTRACE=permissive
3-COMMENT=-----Enforcing Mode-----
3-MAC_FOR_FILE=enforcing
3-MAC_FOR_ARGV0=enforcing
3-MAC_FOR_ENV=enforcing
3-MAC_FOR_NETWORK=enforcing
3-MAC_FOR_SIGNAL=enforcing
3-DENY_CONCEAL_MOUNT=enforcing
3-RESTRICT_CHROOT=enforcing
3-RESTRICT_MOUNT=enforcing
3-RESTRICT_UNMOUNT=enforcing
3-RESTRICT_PIVOT_ROOT=enforcing
3-RESTRICT_AUTOBIND=enabled
3-MAX_ACCEPT_ENTRY=2048
3-MAX_GRANT_LOG=1024
3-MAX_REJECT_LOG=1024
3-TOMOYO_VERBOSE=enabled
3-SLEEP_PERIOD=0
3-MAC_FOR_CAPABILITY::inet_tcp_create=enforcing
3-MAC_FOR_CAPABILITY::inet_tcp_listen=enforcing
3-MAC_FOR_CAPABILITY::inet_tcp_connect=enforcing
3-MAC_FOR_CAPABILITY::use_inet_udp=enforcing
3-MAC_FOR_CAPABILITY::use_inet_ip=enforcing
3-MAC_FOR_CAPABILITY::use_route=enforcing
3-MAC_FOR_CAPABILITY::use_packet=enforcing
3-MAC_FOR_CAPABILITY::SYS_MOUNT=enforcing
3-MAC_FOR_CAPABILITY::SYS_UMOUNT=enforcing
3-MAC_FOR_CAPABILITY::SYS_REBOOT=enforcing
3-MAC_FOR_CAPABILITY::SYS_CHROOT=enforcing
3-MAC_FOR_CAPABILITY::SYS_KILL=enforcing
3-MAC_FOR_CAPABILITY::SYS_VHANGUP=enforcing
3-MAC_FOR_CAPABILITY::SYS_TIME=enforcing
3-MAC_FOR_CAPABILITY::SYS_NICE=enforcing
3-MAC_FOR_CAPABILITY::SYS_SETHOSTNAME=enforcing
3-MAC_FOR_CAPABILITY::use_kernel_module=enforcing
3-MAC_FOR_CAPABILITY::create_fifo=enforcing
3-MAC_FOR_CAPABILITY::create_block_dev=enforcing
3-MAC_FOR_CAPABILITY::create_char_dev=enforcing
3-MAC_FOR_CAPABILITY::create_unix_socket=enforcing
3-MAC_FOR_CAPABILITY::SYS_LINK=enforcing
3-MAC_FOR_CAPABILITY::SYS_SYMLINK=enforcing
3-MAC_FOR_CAPABILITY::SYS_RENAME=enforcing
3-MAC_FOR_CAPABILITY::SYS_UNLINK=enforcing
3-MAC_FOR_CAPABILITY::SYS_CHMOD=enforcing
3-MAC_FOR_CAPABILITY::SYS_CHOWN=enforcing
3-MAC_FOR_CAPABILITY::SYS_IOCTL=enforcing
3-MAC_FOR_CAPABILITY::SYS_KEXEC_LOAD=enforcing
3-MAC_FOR_CAPABILITY::SYS_PIVOT_ROOT=enforcing
3-MAC_FOR_CAPABILITY::SYS_PTRACE=enforcing
EOF
		;;
	esac
fi

if [ ! -r /etc/tomoyo/exception_policy.conf ]; then
	echo Creating exception policy. This will take several minutes.
	make_exception > /etc/tomoyo/exception_policy.conf
	make_alias >> /etc/tomoyo/exception_policy.conf
fi
if [ ! -r /etc/tomoyo/system_policy.conf ]; then
	echo Creating system policy.
	touch /etc/tomoyo/system_policy.conf
fi
if [ ! -r /etc/tomoyo/domain_policy.conf ]; then
	echo Creating domain policy.
	echo '<kernel>' > /etc/tomoyo/domain_policy.conf
	echo 'use_profile 0' >> /etc/tomoyo/domain_policy.conf
fi
