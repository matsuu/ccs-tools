#! /bin/sh

if [ "$1" = "--version" ]
then
cat << EOF
init_policy.sh 1.6.7

Copyright (C) 2005-2009 NTT DATA CORPORATION.

This program is free software; you may redistribute it under the terms of
the GNU General Public License. This program has absolutely no warranty.
EOF
elif [ "$1" = "--help" ]
then
cat << EOF
Usage: init_policy.sh [--file-only-profile|--full-profile]

This program generates templates for all policy files.
You need to review the output because automatically generated exception policy may contain redundant or dangerous entries.

  --file-only-profile    Create profile with only file access control feature enabled.
  --full-profile         Create profile with all features enabled.

Examples:

None.

EOF
else
cat << EOF | help2man -i - -N -s 8 -n "Initialize TOMOYO Linux's policy" $0 | gzip -9 > man8/init_policy.sh.8.gz
[SEE ALSO]

 ccs-init (8)

[NOTES]

 You don't need to run this program after you have successfully initialized policy.

[AUTHORS]

 penguin-kernel _at_ I-love.SAKURA.ne.jp

 Bug fix for Gentoo 64bit environment by Naohiro Aota <naota _at_ namazu.org>.

EOF
fi
exit 0
