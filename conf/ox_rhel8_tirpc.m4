
#
# SYNOPSIS
#
#   OX_RHEL8_TIRPC()
#
# DESCRIPTION
#
# If this system is a RHEL 8 or equivalent, look for the tirpc library on the
# CPPFLAGS and LDFLAGS environment variables. Print an error message if they are
# not listed there if this is a RHEL 8 system.
#
# LICENSE
#
#   Copyright (c) 2022 OPeNDAP
#   Author: James Gallagher <jgallagher@opendap.org>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

dnl Check and see if we are building on RHEL8 or the equivalent and if so,
dnl is the tirpc library available and set up correctly.
AC_DEFUN([OX_RHEL8_TIRPC], [
AS_IF([test -f /etc/redhat-release && grep -q '8\.' /etc/redhat-release],
    dnl if this is RHEL8, then we need the tirpc library on CPPFLAGS and LDFLAGS
    [
        AC_MSG_NOTICE([Found a RHEL 8 or equivalent system...])
        AS_IF([grep -q -v tirpc <<< $CPPFLAGS || grep -q -v tirpc <<< $LDFLAGS],
        dnl if either CPPFLAGS or LDFLAGS lack 'tirpc', error
        [
            AC_MSG_ERROR([Libdap4 on Redhat Linux 8 requires the tirpc library be included on CPPFLAGS and LDFLAGS])
        ],
        [AC_MSG_NOTICE([and CPPFLAGS and LDFLAGS are set])
        ])
    ],
    [
        AC_MSG_NOTICE([Not a RHEL 8 or equivalent system])
    ])
])

