
dnl -*- automake -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

dnl Update version here and below at LIB_CURRENT, ..., if needed.
AC_INIT(bes, 1.0.0, dods-tech@unidata.ucar.edu)
AC_CONFIG_SRCDIR([configure.ac])
AM_CONFIG_HEADER([config.h])

AC_CONFIG_AUX_DIR(conf)

AM_INIT_AUTOMAKE

dnl flags for the compilers and linkers - set these before locating the
dnl actual tools since some of the AC_PROG macros set these `flag variables'
dnl to default values otherwise.

AC_CANONICAL_HOST
AC_SUBST(host)

dnl library visioning: Update these when the interface changes. Generally,
dnl assume that the interface tracks the major and minor release numbers.
LIB_DIS_CURRENT=1
LIB_DIS_AGE=0
LIB_DIS_REVISION=0
AC_SUBST(LIB_DIS_CURRENT)
AC_SUBST(LIB_DIS_AGE)
AC_SUBST(LIB_DIS_REVISION)
LIBDISPATCH_VERSION="$LIB_DIS_CURRENT:$LIB_DIS_REVISION:$LIB_DIS_AGE"
AC_SUBST(LIBDISPATCH_VERSION)

LIB_MOD_CURRENT=1
LIB_MOD_AGE=0
LIB_MOD_REVISION=0
AC_SUBST(LIB_MOD_CURRENT)
AC_SUBST(LIB_MOD_AGE)
AC_SUBST(LIB_MOD_REVISION)
LIBMODULES_VERSION="$LIB_MOD_CURRENT:$LIB_MOD_REVISION:$LIB_MOD_AGE"
AC_SUBST(LIBMODULES_VERSION)

AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_INSTALL

dnl Use libtool instead?
dnl AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_CHECK_PROG(VALGRIND, valgrind, [valgrind --logfile=memcheck])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h limits.h unistd.h pthread.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(strdup strftime timegm mktime)

DISPATCH_MYSQL

AC_CHECK_LIBDAP([3.5.2],
 [
  LIBS="$LIBS $DAP_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])

AC_CONFIG_FILES([Makefile
		 dispatch/Makefile
		 command/Makefile])

AC_CONFIG_FILES([bes-config], [chmod +x bes-config]) 
		 
AC_OUTPUT

