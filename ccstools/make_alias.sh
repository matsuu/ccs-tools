#! /bin/bash
#
# Automatic alias generator.
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
