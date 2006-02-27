# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([hdf4_handler], [3.6.0], [dods-tech@unidata.ucar.edu])
AC_CONFIG_AUX_DIR([conf])

AM_INIT_AUTOMAKE

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
AC_CHECK_LIBDAP([3.6.0],
 [
  LIBS="$LIBS $DAP_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])

echo "LIBS after check_libdap: $LIBS"
echo "..and CPPFLAGS: $CPPFLAGS"

AC_CHECK_HDF4(
   [
       LIBS="$LIBS $HDF4_LIBS"
       LDFLAGS="$LDFLAGS $HDF4_LDFLAGS"
       CPPFLAGS="$CPPFLAGS $HDF4_CPPFLAGS"
   ],
   [
       echo "*** Use --with-hdf4 for the root hdf4 directory."
       echo "*** Otherwise use --with-hdf4-include switch for includes directory"
       echo "*** and --with-hdf4-libdir switch for libraries directory."
       AC_MSG_ERROR([hdf4 library and hdf4 headers are required.])
   ]
)

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

AC_CONFIG_FILES([Makefile
                 hdfclass/Makefile])

dnl Add this later: hdf-testfiles/Makefile
AC_OUTPUT
