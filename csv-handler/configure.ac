# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Configure script for OPeNDAP CSV data handler

AC_PREREQ(2.59)
AC_INIT([csv_module], [3.5.1], [dods-tech@unidata.ucar.edu])
AC_CONFIG_AUX_DIR([conf])

AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([CSVModule.h])
AC_CONFIG_HEADER([config.h])

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
AC_CHECK_LIBDAP([3.7.4],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])
  

AC_CHECK_BES([3.4.0],
[
    # I think the AC_SUBST([BES_MODULE_DIR]) in bes.m4 will put that 
    # symbol in the output. Same for BES_CPPFLAGS. jhrg 5/1/06
    AM_CONDITIONAL([OPENDAPSERVER], [true])
],
[ AM_CONDITIONAL([OPENDAPSERVER], [false])
])

OPENDAP_DEBUG_OPTION

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
