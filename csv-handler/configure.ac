# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Configure script for OPeNDAP CSV data handler

AC_PREREQ(2.59)
AC_INIT(csv_module, 3.5.1, opendap-tech@opendap.org)
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADER([config.h])

AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])

AM_INIT_AUTOMAKE(1.10)
AC_CONFIG_TESTDIR(bes-testsuite, [.])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_CHECK_FUNCS([strchr])

dnl Checks for specific libraries
AC_CHECK_LIBDAP([3.10.2],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])
  

AC_CHECK_BES([3.8.3],
[
],
[ AC_MSG_ERROR([Cannot find bes])
])

OPENDAP_DEBUG_OPTION

AC_CONFIG_FILES([Makefile bes-testsuite/atlocal bes-testsuite/Makefile])
AC_OUTPUT
