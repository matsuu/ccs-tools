#! /bin/sh

if [ "$1" = "--version" ]
then
cat << EOF
tomoyo_init_policy.sh 1.5.1

Copyright (C) 2005-2007 NTT DATA CORPORATION.

This program is free software; you may redistribute it under the terms of
the GNU General Public License. This program has absolutely no warranty.
EOF
elif [ "$1" = "--help" ]
then
cat << EOF
Usage: tomoyo_init_policy.sh [--file-only-profile|--full-profile]

This program generates templates for exception policy and profile and manager.
You need to review the output because automatically generated exception policy may contain redundant or dangerous entries.

  --file-only-profile    Create profile with only file access control feature enabled.
  --full-profile         Create profile with all features enabled.

Examples:

None.

EOF
else
cat << EOF | help2man -i - -N -s 8 -n "Initialize TOMOYO Linux's policy" $0 | gzip -9 > /usr/share/man/man8/tomoyo_init_policy.sh.8.gz
[SEE ALSO]

 tomoyo-init (8)

[NOTES]

 You don't need to run this program after you have successfully initialized policy.

[AUTHORS]

 penguin-kernel _at_ I-love.SAKURA.ne.jp

EOF
fi
exit 0
