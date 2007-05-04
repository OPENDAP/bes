
dnl -*- automake -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

dnl Update version here and below at LIB_CURRENT, ..., if needed.
AC_INIT(bes, 3.4.2, dods-tech@unidata.ucar.edu)
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
LIB_DIS_CURRENT=4
LIB_DIS_AGE=1
LIB_DIS_REVISION=0
AC_SUBST(LIB_DIS_CURRENT)
AC_SUBST(LIB_DIS_AGE)
AC_SUBST(LIB_DIS_REVISION)
LIBDISPATCH_VERSION="$LIB_DIS_CURRENT:$LIB_DIS_REVISION:$LIB_DIS_AGE"
AC_SUBST(LIBDISPATCH_VERSION)

LIB_BESDAP_CURRENT=3
LIB_BESDAP_AGE=0
LIB_BESDAP_REVISION=1
AC_SUBST(LIB_BESDAP_CURRENT)
AC_SUBST(LIB_BESDAP_AGE)
AC_SUBST(LIB_BESDAP_REVISION)
LIBBES_DAP_VERSION="$LIB_BESDAP_CURRENT:$LIB_BESDAP_REVISION:$LIB_BESDAP_AGE"
AC_SUBST(LIBBES_DAP_VERSION)

LIB_PPT_CURRENT=3
LIB_PPT_AGE=0
LIB_PPT_REVISION=1
AC_SUBST(LIB_PPT_CURRENT)
AC_SUBST(LIB_PPT_AGE)
AC_SUBST(LIB_PPT_REVISION)
LIBPPT_VERSION="$LIB_PPT_CURRENT:$LIB_PPT_REVISION:$LIB_PPT_AGE"
AC_SUBST(LIBPPT_VERSION)

LIB_COM_CURRENT=3
LIB_COM_AGE=0
LIB_COM_REVISION=1
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
AC_CHECK_PROG(GZIP, [gzip], [`which gzip`])
AC_CHECK_PROG(BZIP2, [bzip2], [`which bzip2`])
AC_CHECK_PROG(PERL, [perl], [`which perl`])

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

dnl dl lib?
AC_CHECK_FUNC(dlclose, , [ AC_CHECK_LIB(dl, dlopen) ])

dnl Checks for libraries.
dnl AC_CHECK_LIB(termcap, main)

# check for readline
VL_LIB_READLINE
if test "$vl_cv_lib_readline" = "no"; then
   AC_MSG_ERROR(I could not find the readline library!)
fi

SIC_VAR_SYS_ERRLIST

BES_CHECK_OPENSSL
BES_CHECK_KERBEROS

AC_CHECK_LIBDAP([3.7.4],
  [
    AM_CONDITIONAL([LIBDAP], [true])
  ],
  [
    AM_CONDITIONAL([LIBDAP], [false])
  ]
)

AC_CONFIG_FILES([Makefile
		 dispatch/Makefile
		 dispatch/test/Makefile
		 docs/Makefile
		 command/Makefile
		 ppt/Makefile
		 cmdln/Makefile
		 server/Makefile
		 server/besctl
		 server/test/Makefile
		 bin/Makefile
		 templates/Makefile])

AC_CONFIG_FILES([bes-config], [chmod +x bes-config]) 
		 
AC_OUTPUT

