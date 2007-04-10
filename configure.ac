#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([hdf5_handler], [1.1.0], [dods-tech@unidata.ucar.edu])
AC_CONFIG_AUX_DIR([conf])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([HDF5UInt32.h])
AM_CONFIG_HEADER([config_hdf5.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
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
AC_CHECK_LIBDAP([3.7.0],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])

dnl Checks for specific libraries
AC_CHECK_HDF5(
  [
      LIBS="$LIBS $H5_LIBS"
      LDFLAGS="$LDFLAGS $H5_LDFLAGS"
      CPPFLAGS="$CPPFLAGS $H5_CPPFLAGS"
  ],
  [
      echo "*** Use --with-hdf5 for the root hdf5 directory."
      echo "*** Otherwise use --with-hdf5-include switch for includes directory"
      echo "*** and --with-hdf5-libdir switch for libraries directory."
      AC_MSG_ERROR([hdf5 library and hdf5 headers are required.])
  ]
)

AC_ARG_WITH([bes],
            [AS_HELP_STRING([--with-bes=PATH],[bes install root directory])],
            [BES_PATH=${withval}], 
            [BES_PATH=""])

if test "${BES_PATH}" != "no"
then
    if test -x "${BES_PATH}/bin/bes-config"
    then
	AM_CONDITIONAL([OPENDAPSERVER], [true])
	BES_CPPFLAGS="`${BES_PATH}/bin/bes-config --cflags`"
	AC_SUBST([BES_CPPFLAGS])
	BES_LIBS="`${BES_PATH}/bin/bes-config --dap-libs`"
	AC_SUBST([BES_LIBS])
	BES_MODULE_DIR="`${BES_PATH}/bin/bes-config --modulesdir`"
	AC_SUBST([BES_MODULE_DIR])
    else
	AC_CHECK_BES([3.4.0],
	[
	    # I think the AC_SUBST([BES_MODULE_DIR]) in bes.m4 will put that 
	    # symbol in the output. Same for BES_CPPFLAGS. jhrg 5/1/06
	    AM_CONDITIONAL([OPENDAPSERVER], [true])
	],
	[ AM_CONDITIONAL([OPENDAPSERVER], [false])
	])

    fi
else
    AM_CONDITIONAL([OPENDAPSERVER], [false])
fi

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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
