
dnl -*- automake -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

dnl Update version here and below at LIB_CURRENT, ..., if needed.
AC_INIT(bes_apache, 1.0.0, support@opendap.org)
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR(conf)

AM_INIT_AUTOMAKE

dnl flags for the compilers and linkers - set these before locating the
dnl actual tools since some of the AC_PROG macros set these `flag variables'
dnl to default values otherwise.

AC_CANONICAL_HOST
AC_SUBST(host)

dnl library visioning: Update these when the interface changes. Generally,
dnl assume that the interface tracks the major and minor release numbers.
BES_APACHE_CURRENT=1
BES_APACHE_AGE=0
BES_APACHE_REVISION=0
AC_SUBST(BES_APACHE_CURRENT)
AC_SUBST(BES_APACHE_AGE)
AC_SUBST(BES_APACHE_REVISION)
BES_APACHE_VERSION="$BES_APACHE_CURRENT:$BES_APACHE_REVISION:$BES_APACHE_AGE"
AC_SUBST(BES_APACHE_VERSION)

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
AC_CHECK_HEADERS(unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_FUNC_ALLOCA
dnl AC_CHECK_FUNCS(mktime)

AC_CHECK_BES([3.5.3],
 [
 ],
 [ AC_MSG_ERROR([Cannot find bes])
])
  
AC_ARG_WITH([apache],
            [AS_HELP_STRING([--with-apache=PATH],[Apache server source directory])],
            [APACHE_DIR=${withval}], 
            [APACHE_DIR="no"])

AC_MSG_CHECKING([for apache server source directory])
if test "${APACHE_DIR}" != "no"
then
    if ! test -e "${APACHE_DIR}/src/include/httpd.h"
    then
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Apache server source directory not found.])
    else
	AC_MSG_RESULT([${APACHE_DIR}])
    fi
else
    AC_MSG_ERROR([Specify the apache server source directory using --with-apache])
fi

AC_SUBST(APACHE_DIR)

AC_CONFIG_FILES([Makefile unit-tests/Makefile])

AC_OUTPUT

