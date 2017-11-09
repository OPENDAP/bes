#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([hdf5_handler], [2.4.1], [opendap-tech@opendap.org])
AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])
AM_INIT_AUTOMAKE([1.10 tar-pax])

AM_CONDITIONAL([DAP_BUILTIN_MODULES], [false])

AC_CONFIG_TESTDIR(bes-testsuite, [.])

AC_CONFIG_SRCDIR([HDF5UInt32.h])
AM_CONFIG_HEADER([config.h])

# The handler is a shared object library, so we don't normally need
# the static lib. Override this default by specifying `--enable-static',
# which can make running unit tests with valgrind far more pleasant. 
AM_DISABLE_STATIC

PACKAGE_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\1@'`
PACKAGE_MINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\2@'`
PACKAGE_SUBMINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\3@'`
AC_MSG_NOTICE(Package Major Version:     $PACKAGE_MAJOR_VERSION)
AC_MSG_NOTICE(Package Minor Version:     $PACKAGE_MINOR_VERSION)
AC_MSG_NOTICE(Package SubMinor Version:  $PACKAGE_SUBMINOR_VERSION)
AC_SUBST(PACKAGE_MAJOR_VERSION)
AC_SUBST(PACKAGE_MINOR_VERSION)
AC_SUBST(PACKAGE_SUBMINOR_VERSION)

# AC_PROG_MKDIR_P
# is a backport of autoconf-2.60's AC_PROG_MKDIR_P.
# Remove this macro when we can assume autoconf >= 2.60.
m4_ifdef([AC_PROG_MKDIR_P], [], [
  AC_DEFUN([AC_PROG_MKDIR_P],
    [AC_REQUIRE([AM_PROG_MKDIR_P])dnl defined by automake
     MKDIR_P='$(mkdir_p)'
     AC_SUBST([MKDIR_P])])])
AC_PROG_MKDIR_P
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MKDIR_P
AC_PROG_LEX
# Check for FLEX version for better memory leak management.
AC_ARG_VAR([FLEX], [Location of the flex program.])
AC_CHECK_PROG([FLEX], [flex], [flex])
if test x"$FLEX" = x""; then
      	AC_MSG_WARN([No version of flex was found!])
	AC_MSG_ERROR([Please install flex])
else
	V=`$FLEX --version | awk '{print $2}'`
   	W=`echo $V | awk -F. '{if (((($1*100 + $2)*100 + $3) < 20535) || $3 == 4) print "no"}'`
   	if test "x$W" != x ; then
	   AC_MSG_WARN([Adjust your FLEX environment variable])
      	   AC_MSG_ERROR([Flex version 2.5.35 or above is needed. You have $V])
      	fi
fi

AC_SUBST(FLEX)

AC_CHECK_PROG(YACC,[bison],[bison])
# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero strchr strdup strrchr])

dnl Checks for specific libraries
AC_CHECK_LIBDAP([3.11.0],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])

dnl Checks for specific libraries
dnl At this point, we use HDF5 1.8.0 specific APIs in our hdf5 handler.
AC_CHECK_HDF5(
  [
      LIBS="$LIBS $H5_LIBS"
      LDFLAGS="$LDFLAGS $H5_LDFLAGS"
      CPPFLAGS="$CPPFLAGS $H5_CPPFLAGS "
      dnl CPPFLAGS="$CPPFLAGS $H5_CPPFLAGS -DH5_USE_16_API"
  ],
  [
      echo "*** Use --with-hdf5 for the root hdf5 directory."
      echo "*** Otherwise use --with-hdf5-include switch for includes directory"
      echo "*** and --with-hdf5-libdir switch for libraries directory."
      AC_MSG_ERROR([hdf5 library and hdf5 headers are required.])
  ]
)

dnl No need to check if having getdap4 since we will support both DAP2 and DAP4 from this version
dnl AC_CHECK_PROGS([GETDAP4],[getdap4])
dnl if test -n "$GETDAP4"; then
dnl    CPPFLAGS="$CPPFLAGS -DUSE_DAP4"
dnl fi

AC_CHECK_BES([3.9.0],
[
],
[ AC_MSG_ERROR([Cannot find bes])
])


AC_CONFIG_FILES([gctp/Makefile
                 gctp/src/Makefile])

OPENDAP_DEBUG_OPTION

dnl autoheader macros; tack some text at the top and bottom of config_hdf5.h.in
dnl need to config_hdf5_h to config_h since config_hdf5.h is manually generated. KY 2015-06-22

dnl AH_TOP([#ifndef _config_hdf5_h
dnl #define _config_hdf5_h])

AH_TOP([#ifndef _config_h
#define _config_h])

AH_BOTTOM([/* Shorthand for gcc's unused attribute feature */
#if defined(__GNUG__) || defined(__GNUC__)
#define not_used __attribute__ ((unused))
#else
#define not_used 
#endif /* __GNUG__ || __GNUC__ */

#endif /* _config_h */])
dnl #endif /* _config_hdf5_h */])

## This sets up the special symbols used to build the bes.conf file for
## the tests. Previously, this was done in the Makefile.am down in the
## test directory, but building the bes.conf here works better with
## autoconf overall and esp. with the 'modules' build. jhrg 11/19/14
modulesdir=$BES_MODULE_DIR
AC_SUBST(modulesdir)

AC_CONFIG_FILES([Makefile bes-testsuite/Makefile bes-testsuite/atlocal bes-testsuite/bes.conf bes-testsuite/bes.default.conf bes-testsuite/bes.ignore.conf])
AC_OUTPUT
