
dnl -*- automake -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

dnl Update version here and below at LIB_CURRENT, ..., if needed.
AC_INIT(bes, 3.6.0, support@opendap.org)
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
LIB_DIS_CURRENT=6
LIB_DIS_AGE=0
LIB_DIS_REVISION=0
AC_SUBST(LIB_DIS_CURRENT)
AC_SUBST(LIB_DIS_AGE)
AC_SUBST(LIB_DIS_REVISION)
LIBDISPATCH_VERSION="$LIB_DIS_CURRENT:$LIB_DIS_REVISION:$LIB_DIS_AGE"
AC_SUBST(LIBDISPATCH_VERSION)

LIB_BESDAP_CURRENT=3
LIB_BESDAP_AGE=0
LIB_BESDAP_REVISION=3
AC_SUBST(LIB_BESDAP_CURRENT)
AC_SUBST(LIB_BESDAP_AGE)
AC_SUBST(LIB_BESDAP_REVISION)
LIBBES_DAP_VERSION="$LIB_BESDAP_CURRENT:$LIB_BESDAP_REVISION:$LIB_BESDAP_AGE"
AC_SUBST(LIBBES_DAP_VERSION)

LIB_PPT_CURRENT=4
LIB_PPT_AGE=0
LIB_PPT_REVISION=0
AC_SUBST(LIB_PPT_CURRENT)
AC_SUBST(LIB_PPT_AGE)
AC_SUBST(LIB_PPT_REVISION)
LIBPPT_VERSION="$LIB_PPT_CURRENT:$LIB_PPT_REVISION:$LIB_PPT_AGE"
AC_SUBST(LIBPPT_VERSION)

LIB_PPTCAPI_CURRENT=1
LIB_PPTCAPI_AGE=0
LIB_PPTCAPI_REVISION=0
AC_SUBST(LIB_PPTCAPI_CURRENT)
AC_SUBST(LIB_PPTCAPI_AGE)
AC_SUBST(LIB_PPTCAPI_REVISION)
LIBPPTCAPI_VERSION="$LIB_PPTCAPI_CURRENT:$LIB_PPTCAPI_REVISION:$LIB_PPTCAPI_AGE"
AC_SUBST(LIBPPTCAPI_VERSION)

LIB_COM_CURRENT=3
LIB_COM_AGE=0
LIB_COM_REVISION=2
AC_SUBST(LIB_COM_CURRENT)
AC_SUBST(LIB_COM_AGE)
AC_SUBST(LIB_COM_REVISION)
LIBCOMMAND_VERSION="$LIB_COM_CURRENT:$LIB_COM_REVISION:$LIB_COM_AGE"
AC_SUBST(LIBCOMMAND_VERSION)

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
AC_CHECK_HEADERS(fcntl.h limits.h unistd.h pthread.h bzlib.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(strdup strftime timegm mktime)


BES_OLD_LIBS=$LIBS
dnl z and bz2 library?
AC_CHECK_LIB( bz2, BZ2_bzReadOpen,
    [
	AM_CONDITIONAL([BZ2UNCOMPRESS], [true])
	BES_BZ2_LIBS=-lbz2
	AC_DEFINE([HAVE_LIBBZ2], [1], [libbz2])
    ],
    [ AM_CONDITIONAL([BZ2UNCOMPRESS], [false]) ] )
AC_CHECK_LIB( z, gzopen, [BES_ZLIB_LIBS=-lz])

dnl dl lib?
AC_CHECK_FUNC(dlclose, , [ AC_CHECK_LIB(dl, dlopen, [BES_DL_LIBS=-ldl]) ])
LIBS=$BES_OLD_LIBS

AC_SUBST(BES_DL_LIBS)
AC_SUBST(BES_ZLIB_LIBS)
AC_SUBST(BES_BZ2_LIBS)

dnl Checks for libraries.
dnl AC_CHECK_LIB(termcap, main)

# check for readline
VL_LIB_READLINE
if test "$vl_cv_lib_readline" = "no"; then
   AC_MSG_ERROR([I could not find the readline library!])
fi

SIC_VAR_SYS_ERRLIST

BES_CHECK_OPENSSL
dnl BES_CHECK_KERBEROS

AC_CHECK_LIBDAP([3.8.0],
  [
    AM_CONDITIONAL([LIBDAP], [true])
  ],
  [
    AM_CONDITIONAL([LIBDAP], [false])
  ]
)

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
		 dispatch/Makefile
		 dispatch/unit-tests/Makefile
		 docs/Makefile
		 command/Makefile
		 ppt/Makefile
		 ppt/unit-tests/Makefile
		 ppt/testsuite/Makefile
		 cmdln/Makefile
		 standalone/Makefile
		 server/Makefile
		 server/test/Makefile
		 bin/Makefile
		 templates/Makefile
		 bes_dispatch.pc
		 bes_ppt.pc
		 bes_command.pc
		 bes_dap.pc
])

AC_CONFIG_FILES([bes-config], [chmod +x bes-config]) 
AC_CONFIG_FILES([server/besctl], [chmod +x server/besctl]) 
AC_CONFIG_FILES([server/hyraxctl], [chmod +x server/hyraxctl]) 
		 
AC_OUTPUT

