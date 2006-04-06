# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# Configure script for OPeNDAP OPENDAP_CLASS data handler

AC_PREREQ(2.59)
AC_INIT([OPENDAP_TYPE-handler], [1.0.0], [dods-tech@unidata.ucar.edu])
AC_CONFIG_AUX_DIR([conf])

AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([OPENDAP_TYPE_handler.cc])
AC_CONFIG_HEADER([config_OPENDAP_TYPE.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

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
AC_CHECK_LIBDAP([3.5.3],
 [
  LIBS="$LIBS $DAP_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])
  
AC_CHECK_BES([3.1.0],
 [
  LDFLAGS="$LDFLAGS $BES_LDFLAGS"
  CPPFLAGS="$CPPFLAGS $BES_CFLAGS"
  LDADD="$LDADD $BES_LDADD"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])
  
OPENDAP_DEBUG_OPTION

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
