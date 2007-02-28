#! /bin/bash
#
# Minimal allow_read generator.
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
