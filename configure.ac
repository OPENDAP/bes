#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([hdf5_handler], [1.0], [dods-tech@unidata.ucar.edu])
AC_CONFIG_AUX_DIR([conf])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([HDF5UInt32.h])
AM_CONFIG_HEADER([config_hdf5.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

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
AC_CHECK_LIBDAP([3.7.0],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])

dnl Checks for specific libraries
AC_ARG_WITH(hdf5_path,
    [  --with-hdf5-path=ARG    Pathname to the hdf 5 software],
    hdf5_path=${withval}, hdf5_path="/usr/local/hdf5")

# To find the library, AC_CHECK_LIB needs this set.
LDFLAGS="$LDFLAGS -L${hdf5_path}/lib"
CPPFLAGS="$CPPFLAGS -I${hdf5_path}/include"
AC_CHECK_LIB(hdf5, H5open, hdf_found=1, hdf_found=0, -lz)
if test $hdf_found -ne 1
then
    AC_MSG_ERROR(HDF5 library not found (looked in ${hdf5_path}/lib)!)
fi 
AC_SUBST(hdf5_path)

AC_ARG_WITH([bes],
            [AS_HELP_STRING([--with-bes=PATH],[bes install root directory])],
            [BES_PATH=${withval}], 
            [BES_PATH="/usr/local"])

dnl Simple test for BES. A better test would look something like the libdap
dnl macro written by Patrice. This will get us going... If --with-bes is not
dnl given, then don't build the bes code. jhrg 10/3/05
if test -x "${BES_PATH}/bin/bes-config"
then
    AM_CONDITIONAL([OPENDAPSERVER], [true])
    BES_CPPFLAGS="`${BES_PATH}/bin/bes-config --cflags`"
    AC_SUBST([BES_CPPFLAGS])
    BES_LDFLAGS="`${BES_PATH}/bin/bes-config --libs`"
    AC_SUBST([BES_LDFLAGS])
    BES_MODULE_DIR="`${BES_PATH}/bin/bes-config --prefix`/lib/bes"
    AC_SUBST([BES_MODULE_DIR])
else
    AM_CONDITIONAL([OPENDAPSERVER], [false])
fi

dnl autoheader macros; tack some text at the top and bottom of config_dap.h.in

AH_TOP([#ifndef _config_hdf5_h
#define _config_hdf5_h])

AH_BOTTOM([/* Shorthand for gcc's unused attribute feature */
#if defined(__GNUG__) || defined(__GNUC__)
#define not_used __attribute__ ((unused))
#else
#define not_used 
#endif /* __GNUG__ || __GNUC__ */

#endif /* _config_hdf5_h */])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
