# Copyright (C) 2010-2016 The HDF Group
#                                                -*- Autoconf -*-
# Author: Hyo-Kyung Lee
#
# Date: Jul 20, 2016
# Note: updated version number.

# Date: Oct 14, 2014
# Note: added copyright notice and updated the version.
#
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(h4mapwriter, 1.0.8, help@hdfgroup.org)
AM_INIT_AUTOMAKE(1.10)
AC_CONFIG_SRCDIR([.])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_TESTDIR(testsuite, [.])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_TYPE_SSIZE_T
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_MSG_CHECKING([for h4cc])
if ($CC -show &> /dev/null); then
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([$CC])
	AC_MSG_ERROR([CC environment variable is not h4cc])
fi

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h string.h fts.h libgen.h openssl/md5.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile testsuite/Makefile testsuite/atlocal])
#AC_OUTPUT(Makefile)
AC_OUTPUT

