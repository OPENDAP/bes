#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(hdf5_handler, 2.2.0, opendap-tech@opendap.org)
AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])
AM_INIT_AUTOMAKE([1.10 tar-pax])
AC_CONFIG_TESTDIR(bes-testsuite, [.])

AC_CONFIG_SRCDIR([HDF5UInt32.h])
AM_CONFIG_HEADER([config_hdf5.h])

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
dnl In the future, make it optional if you want to support both 1.6 and 1.8.
dnl At this point, we don't use HDF5 1.8.0 specific APIs in our hdf5 handler.
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

AC_CHECK_BES([3.9.0],
[
],
[ AC_MSG_ERROR([Cannot find bes])
])

OPENDAP_DEBUG_OPTION

dnl autoheader macros; tack some text at the top and bottom of config_hdf5.h.in

AH_TOP([#ifndef _config_hdf5_h
#define _config_hdf5_h])

AH_BOTTOM([/* Shorthand for gcc's unused attribute feature */
#if defined(__GNUG__) || defined(__GNUC__)
#define not_used __attribute__ ((unused))
#else
#define not_used 
#endif /* __GNUG__ || __GNUC__ */

#endif /* _config_hdf5_h */])

AC_CONFIG_FILES([Makefile bes-testsuite/Makefile bes-testsuite/atlocal])
AC_OUTPUT
