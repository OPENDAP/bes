# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(hdf4_handler, 3.7.12, opendap-tech@opendap.org)
AC_CONFIG_AUX_DIR([conf])

AM_INIT_AUTOMAKE
AC_CONFIG_TESTDIR(bes-testsuite, [.])

AC_CONFIG_SRCDIR([HDFStr.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_AWK
AC_PROG_INSTALL
AM_PROG_LEX
AC_PROG_LIBTOOL
# We really need bison and not yacc. If you use AC_PROG_YACC, the resulting 
# Makfile will call bison -y which doesn't know how to make the parsers 
# we require. jhrg 6/15/05
AC_CHECK_PROG(YACC,[bison],[bison])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h sys/param.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_CHECK_FUNCS([isascii])

dnl Checks for specific libraries
AC_CHECK_LIBDAP([3.9.0],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])

echo "LIBS after check_libdap: $LIBS"
echo "..and CPPFLAGS: $CPPFLAGS"

AC_CHECK_HDF4(
   [
dnl       LIBS="$LIBS $HDF4_LIBS"
dnl       LDFLAGS="$LDFLAGS $HDF4_LDFLAGS"
       CPPFLAGS="$CPPFLAGS $HDF4_CFLAGS"
   ],
   [
       echo "*** Use --with-hdf4 for the root hdf4 directory."
       echo "*** Otherwise use --with-hdf4-include switch for includes directory"
       echo "*** and --with-hdf4-libdir switch for libraries directory."
       AC_MSG_ERROR([hdf4 library and hdf4 headers are required.])
   ]
)

AC_CHECK_BES([3.7.0],
[
    # I think the AC_SUBST([BES_MODULE_DIR]) in bes.m4 will put that 
    # symbol in the output. Same for BES_CPPFLAGS. jhrg 5/1/06
    AM_CONDITIONAL([OPENDAPSERVER], [true])
],
[ AM_CONDITIONAL([OPENDAPSERVER], [false])
])

OPENDAP_DEBUG_OPTION

dnl check for an explict server cache pathname. If so, use that. Otherwise
dnl use /usr/tmp.

AC_ARG_WITH(cache_path,
    [  --with-cache=DIR        Server's cache (default: /usr/tmp)],
    HDF_CACHE=${withval}, HDF_CACHE=/usr/tmp)

AC_MSG_RESULT(Setting the HDF server cache directory to $HDF_CACHE)
AC_SUBST(HDF_CACHE)

dnl setup the url for the testsuite.
if test -f tests.conf
then
    default_url=`grep 'url:' tests.conf | sed 's/url:\(.*\)$/\1/'`
else
    default_url=http://localhost/dods-test/nph-dods/data/hdf/server-tests/
fi

AC_ARG_WITH(test-url,
    [  --with-test-url=url     Test file base URL (default: http://localhost/dods-test/nph-dods/data/hdf/server-tests/)],
    testurl=${withval}, testurl=$default_url)

AC_MSG_RESULT(Setting the Test file base URL to $testurl)
AC_SUBST(testurl)

AC_ARG_ENABLE([short-name], 
	     [AS_HELP_STRING([--enable-short-name=yes],[Short dataset name will be used instead of full name.])],
	     [use_short_name=$enableval],[use_short_name=no])
if test "${use_short_name}" = "yes"; then
   CPPFLAGS="$CPPFLAGS -DSHORT_NAME"
fi 


AM_CONDITIONAL([CF], [false])
AC_ARG_ENABLE([cf], 
	     [AS_HELP_STRING([--enable-cf=yes],[Pseudo CF metadata convention support will be enabled.])],
	     [use_cf=$enableval],[use_cf=no])
if test "${use_cf}" = "yes"; then
   CPPFLAGS="$CPPFLAGS -DCF"
   AM_CONDITIONAL([CF], [true])
fi 

dnl ----------------------------------------------------------------------                                
dnl Test for HDF-EOS2                                                                                     

AC_SUBST(H5_USE_HDFEOS2) H5_USE_HDFEOS2="no"

AC_ARG_WITH([hdfeos2],
            [AC_HELP_STRING([--with-hdfeos2=DIR],
                            [Specify path to external hdfeos2 library.])],
            [if test "$withval" != no; then
               if test "$withval" != yes; then
                 HDFEOS2_DIR=$withval
               fi
             fi])
AM_CONDITIONAL([HDFEOS2], [false])
if test -n "$HDFEOS2_DIR"; then
  CPPFLAGS="$CPPFLAGS -I$HDFEOS2_DIR/include -DUSE_HDFEOS2_LIB"
  LDFLAGS="$LDFLAGS -L$HDFEOS2_DIR/lib"
  HDFEOS2_LIBS="-lhdfeos -lGctp"
dnl  AC_CHECK_LIB(Gctp, inv_init,, AC_MSG_ERROR([invalid Gctp in hdfeos2]))
dnl  AC_CHECK_LIB(hdfeos, GDopen,, AC_MSG_ERROR([invalid hdfeos in hdfeos2]))
  H5_USE_HDFEOS2="yes"
  AM_CONDITIONAL([HDFEOS2], [true])
fi
AC_SUBST([HDFEOS2_LIBS])

dnl ----------------------------------------------------------------------                                

AC_CONFIG_FILES([Makefile
		 bes-testsuite/Makefile
		 bes-testsuite/atlocal
                 hdfclass/Makefile])

dnl Add this later: hdf-testfiles/Makefile
AC_OUTPUT
