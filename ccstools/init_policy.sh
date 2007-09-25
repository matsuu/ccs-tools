#! /bin/bash
#
# Initial policy generator.
#
# Copyright (C) 2005-2007  NTT DATA CORPORATION
#
# Version: 1.5.0   2007/09/20
#

cd ${0%/*}
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
	for i in `find /proc/1/ /proc/self/ -type f`
	do
		echo "file_pattern "$i | sed 's@/[0-9]*/@/\\$/@g'
	done | sort | uniq
	
	#
	# Make patterns for /sys/ directory.
	#
	if [ -e /sys/block/ ]
	then
		for i in /sys/*
		do
			for j in `find $i | awk -F / ' { print NF-1 }'`
			do
				echo -n "file_pattern "$i; for ((k = 2; k < $j; k = $k + 1)); do echo -n '/\*'; done; echo
			done
		done | grep -F '\' | sort | uniq
	fi
      
	#
	# Make patterns for /dev/ directory.
	#
	echo 'file_pattern /dev/pts/\$'
	echo 'file_pattern /dev/vc/\$'
	echo 'file_pattern /dev/tty\$'
	
	#
	# Make patterns for policy directory.
	#
	echo 'file_pattern /etc/ccs/system_policy.\$-\$-\$.\$:\$:\$.conf'
	echo 'file_pattern /etc/ccs/exception_policy.\$-\$-\$.\$:\$:\$.conf'
	echo 'file_pattern /etc/ccs/domain_policy.\$-\$-\$.\$:\$:\$.conf'
	
	#
	# Make patterns for man directory.
	#
	for i in `find /usr/share/man/ -type f | awk -F / ' { print NF }' | sort | uniq`
	do
		echo -n "file_pattern /usr/share/man"; for ((j = 4; j < $i; j = $j + 1)); do echo -n '/\*'; done; echo
	done
      
	for i in `find /usr/X11R6/man/ -type f | awk -F / ' { print NF }' | sort | uniq`
	do
		echo -n "file_pattern /usr/X11R6/man"; for ((j = 4; j < $i; j = $j + 1)); do echo -n '/\*'; done; echo
	done
	
	#
	# Make patterns for spool directory. (No trailing /, or detecting symlink fails.)
	#
	for i in /var/spool/clientmqueue /var/spool/mail /var/spool/mqueue /var/spool/at /var/spool/exim4/msglog /var/spool/exim4/input /var/spool/cron/atjobs /var/spool/postfix/maildrop /var/spool/postfix/incoming /var/spool/postfix/active /var/spool/postfix/bounce
	do
		[ -d $i/ -a ! -L $i ] && echo 'file_pattern '$i'/\*'
	done
	
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
	for i in /etc/ld.so.cache /proc/meminfo /proc/sys/kernel/version /etc/localtime /usr/lib/gconv/gconv-modules.cache /usr/lib/locale/locale-archive /usr/share/locale/locale.alias /usr/share/locale/ja/LC_MESSAGES/coreutils.mo /usr/share/locale/ja/LC_MESSAGES/libc.mo
	do
		FILE=`realpath $i`
		[ -n "$FILE" -a -r "$FILE" -a ! -L "$FILE" ] && echo 'allow_read '$FILE
	done
	
	#
	# Allow reading information for current process.
	#
	for i in `find /proc/self/ -type f | grep -v '[0-9]'`
	do
		echo 'allow_read '$i
	done
	
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
	for FILE in `for i in /etc/init.d/*; do realpath $i; done | sort | uniq`
	do
		[ -n "$FILE" -a -f "$FILE" -a -x "$FILE" -a ! -L "$FILE" ] && echo "initialize_domain "$FILE
	done
	
	#
	# Mark some programs that you want to assign short domainname as initializer.
	#
	# RHL9:  /sbin/cardmgr /sbin/klogd /sbin/mingetty /sbin/portmap /sbin/rpc.statd /sbin/syslogd /usr/bin/jserver /usr/bin/spamd /usr/sbin/anacron /usr/sbin/apmd /usr/sbin/atd /usr/sbin/crond /usr/sbin/dhcpd /usr/sbin/gpm /usr/sbin/httpd /usr/sbin/nmbd /usr/sbin/rpc.mountd /usr/sbin/rpc.rquotad /usr/sbin/sendmail.sendmail /usr/sbin/smbd /usr/sbin/sshd /usr/sbin/vsftpd /usr/sbin/xinetd
	# FC3:   /sbin/cardmgr /sbin/klogd /sbin/mingetty /sbin/portmap /sbin/rpc.statd /sbin/syslogd /sbin/udevd /usr/X11R6/bin/xfs /usr/bin/dbus-daemon-1 /usr/bin/mDNSResponder /usr/bin/nifd /usr/sbin/acpid /usr/sbin/anacron /usr/sbin/atd /usr/sbin/cannaserver /usr/sbin/cpuspeed /usr/sbin/crond /usr/sbin/cupsd /usr/sbin/gpm /usr/sbin/hald /usr/sbin/htt /usr/sbin/nmbd /usr/sbin/rpc.idmapd /usr/sbin/rpc.mountd /usr/sbin/rpc.rquotad /usr/sbin/smartd /usr/sbin/smbd /usr/sbin/sshd /usr/sbin/xinetd
	# Sarge: /sbin/getty /sbin/klogd /sbin/portmap /sbin/rpc.statd /sbin/syslogd /usr/sbin/afpd /usr/sbin/apache2 /usr/sbin/atalkd /usr/sbin/atd /usr/sbin/cron /usr/sbin/exim4 /usr/sbin/inetd /usr/sbin/lpd /usr/sbin/nmbd /usr/sbin/papd /usr/sbin/smbd /usr/sbin/sshd /usr/sbin/vmware-guestd
	#
	# You can choose from the list above or add as you like to the list below.
	#
	for FILE in /sbin/getty /sbin/init /sbin/mingetty /sbin/udevd /usr/sbin/anacron /usr/sbin/apache2 /usr/sbin/atd /usr/sbin/cron /usr/sbin/crond /usr/sbin/httpd /usr/sbin/inetd /usr/sbin/logrotate /usr/sbin/smbd /usr/sbin/squid /usr/sbin/sshd /usr/sbin/vsftpd /usr/sbin/xinetd
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
	SPAMD_PATH=`which spamd`
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
	MAIL_PATH=`which mail`
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
	GPM_PATH=`which gpm`
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
	MAKEWHATIS_PATH=`which makewhatis`
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
	AUTOMOUNT_PATH=`which automount`
	if [ -n "$AUTOMOUNT_PATH" ]; then
		if grep -qF '/var/lock/autofs' $AUTOMOUNT_PATH; then
			echo 'file_pattern /var/lock/autofs.\$'
		fi
		echo 'file_pattern /tmp/auto\?\?\?\?\?\?/'
	fi
	
	#
	# Make patterns for logwatch(8).
	#
	LOGWATCH_PATH=`which logwatch`
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
	LOGROTATE_PATH=`which logrotate`
	if [ -n "$LOGROTATE_PATH" ]; then
		if grep -qF '/logrotate.XXXXXX' $LOGROTATE_PATH; then
			echo 'file_pattern /tmp/logrotate.\?\?\?\?\?\?'
			echo 'aggregator /tmp/logrotate.\?\?\?\?\?\? /tmp/logrotate.tmp'
		fi
	fi
	
	#
	# Make patterns for cardmgr(8).
	#
	CARDMGR_PATH=`which cardmgr`
	if [ -n "$CARDMGR_PATH" ]; then
		if grep -qF '%s/cm-%d-%d' $CARDMGR_PATH; then
			echo 'file_pattern /var/lib/pcmcia/cm-\$-\$'
		fi
	fi
	
	#
	# Make patterns for acacron(8).
	#
	ANACRON_PATH=`which anacron`
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
	
	#
	# Make /var/log/ directory not rewritable by default.
	#
	for i in `find /var/log/ -type f | awk -F / ' { print NF }' | sort | uniq`
	do
		echo -n "deny_rewrite /var/log"; for ((j = 3; j < $i; j = $j + 1)); do echo -n '/\*'; done; echo
	done
}

make_alias() {
	for MNT in `df | awk ' { print $NF } ' | grep / | sort | uniq`
	do
		for SYMLINK in `find $MNT -xdev -type l`
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

if [ ! -d /etc/ccs/ ]; then
	echo Creating policy directory.
	mkdir -p /etc/ccs
fi
chmod 700 /etc/ccs/
chown root:root /etc/ccs/

if [ ! -r /etc/ccs/manager.conf ]; then
	echo Creating manager policy.
	echo /usr/lib/ccs/loadpolicy  > /etc/ccs/manager.conf
	echo /usr/lib/ccs/editpolicy >> /etc/ccs/manager.conf
	echo /usr/lib/ccs/setlevel   >> /etc/ccs/manager.conf
	echo /usr/lib/ccs/setprofile >> /etc/ccs/manager.conf
	echo /usr/lib/ccs/ld-watch   >> /etc/ccs/manager.conf
	echo /usr/lib/ccs/ccs-queryd >> /etc/ccs/manager.conf
fi

if [ ! -r /etc/ccs/profile.conf ]; then
	echo Creating default profile.
	case "$PROFILE_TYPE" in
	--file-only-profile)
		cat > /etc/ccs/profile.conf << EOF
0-COMMENT=-----Disabled Mode-----
0-MAC_FOR_FILE=0
0-TOMOYO_VERBOSE=0
1-COMMENT=-----Learning Mode-----
1-MAC_FOR_FILE=1
1-TOMOYO_VERBOSE=0
2-COMMENT=-----Permissive Mode-----
2-MAC_FOR_FILE=2
2-TOMOYO_VERBOSE=1
3-COMMENT=-----Enforcing Mode-----
3-MAC_FOR_FILE=3
3-TOMOYO_VERBOSE=1
EOF
		;;
	*)
		cat > /etc/ccs/profile.conf << EOF
0-COMMENT=-----Disabled Mode-----
1-COMMENT=-----Learning Mode-----
1-MAC_FOR_FILE=1
1-MAC_FOR_ARGV0=1
1-MAC_FOR_NETWORK=1
1-MAC_FOR_SIGNAL=1
1-DENY_CONCEAL_MOUNT=1
1-RESTRICT_CHROOT=1
1-RESTRICT_MOUNT=1
1-RESTRICT_UNMOUNT=1
1-RESTRICT_PIVOT_ROOT=1
1-RESTRICT_AUTOBIND=1
1-MAX_ACCEPT_ENTRY=2048
1-MAX_GRANT_LOG=1024
1-MAX_REJECT_LOG=1024
1-TOMOYO_VERBOSE=0
1-ALLOW_ENFORCE_GRACE=0
1-MAC_FOR_CAPABILITY::inet_tcp_create=1
1-MAC_FOR_CAPABILITY::inet_tcp_listen=1
1-MAC_FOR_CAPABILITY::inet_tcp_connect=1
1-MAC_FOR_CAPABILITY::use_inet_udp=1
1-MAC_FOR_CAPABILITY::use_inet_ip=1
1-MAC_FOR_CAPABILITY::use_route=1
1-MAC_FOR_CAPABILITY::use_packet=1
1-MAC_FOR_CAPABILITY::SYS_MOUNT=1
1-MAC_FOR_CAPABILITY::SYS_UMOUNT=1
1-MAC_FOR_CAPABILITY::SYS_REBOOT=1
1-MAC_FOR_CAPABILITY::SYS_CHROOT=1
1-MAC_FOR_CAPABILITY::SYS_KILL=1
1-MAC_FOR_CAPABILITY::SYS_VHANGUP=1
1-MAC_FOR_CAPABILITY::SYS_TIME=1
1-MAC_FOR_CAPABILITY::SYS_NICE=1
1-MAC_FOR_CAPABILITY::SYS_SETHOSTNAME=1
1-MAC_FOR_CAPABILITY::use_kernel_module=1
1-MAC_FOR_CAPABILITY::create_fifo=1
1-MAC_FOR_CAPABILITY::create_block_dev=1
1-MAC_FOR_CAPABILITY::create_char_dev=1
1-MAC_FOR_CAPABILITY::create_unix_socket=1
1-MAC_FOR_CAPABILITY::SYS_LINK=1
1-MAC_FOR_CAPABILITY::SYS_SYMLINK=1
1-MAC_FOR_CAPABILITY::SYS_RENAME=1
1-MAC_FOR_CAPABILITY::SYS_UNLINK=1
1-MAC_FOR_CAPABILITY::SYS_CHMOD=1
1-MAC_FOR_CAPABILITY::SYS_CHOWN=1
1-MAC_FOR_CAPABILITY::SYS_IOCTL=1
1-MAC_FOR_CAPABILITY::SYS_KEXEC_LOAD=1
1-MAC_FOR_CAPABILITY::SYS_PIVOT_ROOT=1
2-COMMENT=-----Permissive Mode-----
2-MAC_FOR_FILE=2
2-MAC_FOR_ARGV0=2
2-MAC_FOR_NETWORK=2
2-MAC_FOR_SIGNAL=2
2-DENY_CONCEAL_MOUNT=2
2-RESTRICT_CHROOT=2
2-RESTRICT_MOUNT=2
2-RESTRICT_UNMOUNT=2
2-RESTRICT_PIVOT_ROOT=2
2-RESTRICT_AUTOBIND=1
2-MAX_ACCEPT_ENTRY=2048
2-MAX_GRANT_LOG=1024
2-MAX_REJECT_LOG=1024
2-TOMOYO_VERBOSE=1
2-ALLOW_ENFORCE_GRACE=0
2-MAC_FOR_CAPABILITY::inet_tcp_create=2
2-MAC_FOR_CAPABILITY::inet_tcp_listen=2
2-MAC_FOR_CAPABILITY::inet_tcp_connect=2
2-MAC_FOR_CAPABILITY::use_inet_udp=2
2-MAC_FOR_CAPABILITY::use_inet_ip=2
2-MAC_FOR_CAPABILITY::use_route=2
2-MAC_FOR_CAPABILITY::use_packet=2
2-MAC_FOR_CAPABILITY::SYS_MOUNT=2
2-MAC_FOR_CAPABILITY::SYS_UMOUNT=2
2-MAC_FOR_CAPABILITY::SYS_REBOOT=2
2-MAC_FOR_CAPABILITY::SYS_CHROOT=2
2-MAC_FOR_CAPABILITY::SYS_KILL=2
2-MAC_FOR_CAPABILITY::SYS_VHANGUP=2
2-MAC_FOR_CAPABILITY::SYS_TIME=2
2-MAC_FOR_CAPABILITY::SYS_NICE=2
2-MAC_FOR_CAPABILITY::SYS_SETHOSTNAME=2
2-MAC_FOR_CAPABILITY::use_kernel_module=2
2-MAC_FOR_CAPABILITY::create_fifo=2
2-MAC_FOR_CAPABILITY::create_block_dev=2
2-MAC_FOR_CAPABILITY::create_char_dev=2
2-MAC_FOR_CAPABILITY::create_unix_socket=2
2-MAC_FOR_CAPABILITY::SYS_LINK=2
2-MAC_FOR_CAPABILITY::SYS_SYMLINK=2
2-MAC_FOR_CAPABILITY::SYS_RENAME=2
2-MAC_FOR_CAPABILITY::SYS_UNLINK=2
2-MAC_FOR_CAPABILITY::SYS_CHMOD=2
2-MAC_FOR_CAPABILITY::SYS_CHOWN=2
2-MAC_FOR_CAPABILITY::SYS_IOCTL=2
2-MAC_FOR_CAPABILITY::SYS_KEXEC_LOAD=2
2-MAC_FOR_CAPABILITY::SYS_PIVOT_ROOT=2
3-COMMENT=-----Enforcing Mode-----
3-MAC_FOR_FILE=3
3-MAC_FOR_ARGV0=3
3-MAC_FOR_NETWORK=3
3-MAC_FOR_SIGNAL=3
3-DENY_CONCEAL_MOUNT=3
3-RESTRICT_CHROOT=3
3-RESTRICT_MOUNT=3
3-RESTRICT_UNMOUNT=3
3-RESTRICT_PIVOT_ROOT=3
3-RESTRICT_AUTOBIND=1
3-MAX_ACCEPT_ENTRY=2048
3-MAX_GRANT_LOG=1024
3-MAX_REJECT_LOG=1024
3-TOMOYO_VERBOSE=1
3-ALLOW_ENFORCE_GRACE=0
3-MAC_FOR_CAPABILITY::inet_tcp_create=3
3-MAC_FOR_CAPABILITY::inet_tcp_listen=3
3-MAC_FOR_CAPABILITY::inet_tcp_connect=3
3-MAC_FOR_CAPABILITY::use_inet_udp=3
3-MAC_FOR_CAPABILITY::use_inet_ip=3
3-MAC_FOR_CAPABILITY::use_route=3
3-MAC_FOR_CAPABILITY::use_packet=3
3-MAC_FOR_CAPABILITY::SYS_MOUNT=3
3-MAC_FOR_CAPABILITY::SYS_UMOUNT=3
3-MAC_FOR_CAPABILITY::SYS_REBOOT=3
3-MAC_FOR_CAPABILITY::SYS_CHROOT=3
3-MAC_FOR_CAPABILITY::SYS_KILL=3
3-MAC_FOR_CAPABILITY::SYS_VHANGUP=3
3-MAC_FOR_CAPABILITY::SYS_TIME=3
3-MAC_FOR_CAPABILITY::SYS_NICE=3
3-MAC_FOR_CAPABILITY::SYS_SETHOSTNAME=3
3-MAC_FOR_CAPABILITY::use_kernel_module=3
3-MAC_FOR_CAPABILITY::create_fifo=3
3-MAC_FOR_CAPABILITY::create_block_dev=3
3-MAC_FOR_CAPABILITY::create_char_dev=3
3-MAC_FOR_CAPABILITY::create_unix_socket=3
3-MAC_FOR_CAPABILITY::SYS_LINK=3
3-MAC_FOR_CAPABILITY::SYS_SYMLINK=3
3-MAC_FOR_CAPABILITY::SYS_RENAME=3
3-MAC_FOR_CAPABILITY::SYS_UNLINK=3
3-MAC_FOR_CAPABILITY::SYS_CHMOD=3
3-MAC_FOR_CAPABILITY::SYS_CHOWN=3
3-MAC_FOR_CAPABILITY::SYS_IOCTL=3
3-MAC_FOR_CAPABILITY::SYS_KEXEC_LOAD=3
3-MAC_FOR_CAPABILITY::SYS_PIVOT_ROOT=3
EOF
		;;
	esac
fi

if [ ! -r /etc/ccs/exception_policy.conf ]; then
	echo Creating exception policy. This will take several minutes.
	make_exception > /etc/ccs/exception_policy.conf
	make_alias >> /etc/ccs/exception_policy.conf
fi
