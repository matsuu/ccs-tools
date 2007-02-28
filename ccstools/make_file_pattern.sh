#! /bin/bash
#
# Minimal file_pattern generator.
#
# Copyright (C) 2005-2007  NTT DATA CORPORATION
#
# Version: 1.3.2   2007/02/14
#
# Run this script and add the output to /etc/ccs/exception_policy.txt .
# You MUST review the content of /etc/ccs/exception_policy.txt
# because there would be redundant or dangerous entries.
#

cd ${0%/*}
export PATH=$PWD:/sbin:/bin:${PATH}

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
echo 'file_pattern /etc/ccs/system_policy.\$-\$-\$.\$:\$:\$.txt'
echo 'file_pattern /etc/ccs/exception_policy.\$-\$-\$.\$:\$:\$.txt'
echo 'file_pattern /etc/ccs/domain_policy.\$-\$-\$.\$:\$:\$.txt'

#
# Make patterns for unnamed pipes and sockets.
#
echo 'file_pattern pipe:[\$]'
echo 'file_pattern socket:[\$]'

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
