
dnl -*- automake -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

dnl Update version here and below at LIB_CURRENT, ..., if needed.
AC_INIT(pptcapi, 1.0.3, support@opendap.org)
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
LIB_PPTCAPI_CURRENT=1
LIB_PPTCAPI_AGE=0
LIB_PPTCAPI_REVISION=3
AC_SUBST(LIB_PPTCAPI_CURRENT)
AC_SUBST(LIB_PPTCAPI_AGE)
AC_SUBST(LIB_PPTCAPI_REVISION)
LIBPPTCAPI_VERSION="$LIB_PPTCAPI_CURRENT:$LIB_PPTCAPI_REVISION:$LIB_PPTCAPI_AGE"
AC_SUBST(LIBPPTCAPI_VERSION)

AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL

dnl Use libtool instead?
dnl AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_CHECK_PROG(VALGRIND, valgrind, [valgrind --logfile=memcheck])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h limits.h unistd.h pthread.h bzlib.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_FUNC_ALLOCA

AC_ARG_ENABLE([developer],
  [
    AS_HELP_STRING([--enable-developer],[developer mode])
  ],
  [
    AM_CONDITIONAL([BES_DEVELOPER], [true])
  ],
  [
    AM_CONDITIONAL([BES_DEVELOPER], [false])
  ]
)

AM_PATH_CPPUNIT(1.12.0,
	[AM_CONDITIONAL([CPPUNIT], [true])],
	[AM_CONDITIONAL([CPPUNIT], [false])])

AC_CONFIG_FILES([Makefile
		 bes_pptcapi.pc
		 unit-tests/Makefile
		 testsuite/Makefile
])

AC_CONFIG_FILES([pptcapi-config], [chmod +x pptcapi-config]) 
AC_OUTPUT

