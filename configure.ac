#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(hdf5_handler, 1.2.2, opendap-tech@opendap.org)
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
AC_CHECK_LIBDAP([3.8.0],
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
      CPPFLAGS="$CPPFLAGS $H5_CPPFLAGS -DH5_USE_16_API"
  ],
  [
      echo "*** Use --with-hdf5 for the root hdf5 directory."
      echo "*** Otherwise use --with-hdf5-include switch for includes directory"
      echo "*** and --with-hdf5-libdir switch for libraries directory."
      AC_MSG_ERROR([hdf5 library and hdf5 headers are required.])
  ]
)

AC_ARG_ENABLE([cf], 
	     [AS_HELP_STRING([--enable-cf=yes],[Pseudo CF metadata convention support will be enabled.])],
	     [use_cf=$enableval],[use_cf=no])
if test "${use_cf}" = "yes"; then
   CPPFLAGS="$CPPFLAGS -DCF"
fi 


AC_ARG_ENABLE([short-path], 
	     [AS_HELP_STRING([--enable-short-path=yes],[Short path name will be used instead of full path])],
	     [use_short_path=$enableval],[use_short_path=no])
if test "${use_short_path}" = "yes"; then
   CPPFLAGS="$CPPFLAGS -DSHORT_PATH"
fi 

AC_ARG_ENABLE([eos-grid], 
	     [AS_HELP_STRING([--enable-eos-grid=yes],[NASA EOS file grid mapping support will be added])],
	     [use_eos_grid=$enableval],[use_eos_grid=yes])
if test "${use_eos_grid}" = "yes"; then
   CPPFLAGS="$CPPFLAGS -DNASA_EOS_GRID"
fi 

AC_ARG_ENABLE([eos-meta], 
	     [AS_HELP_STRING([--enable-eos-meta=yes],[NASA EOS file meta file parse support will be added])],
	     [use_eos_meta=$enableval],[use_eos_meta=yes])
if test "${use_eos_meta}" = "yes"; then
   CPPFLAGS="$CPPFLAGS -DNASA_EOS_META"
fi 

AC_CHECK_BES([3.6.0],
[
    # I think the AC_SUBST([BES_MODULE_DIR]) in bes.m4 will put that 
    # symbol in the output. Same for BES_CPPFLAGS. jhrg 5/1/06
    AM_CONDITIONAL([OPENDAPSERVER], [true])
],
[ AM_CONDITIONAL([OPENDAPSERVER], [false])
])

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
