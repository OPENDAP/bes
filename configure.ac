# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([hdf4_handler], [3.5.0], [dods-tech@unidata.ucar.edu])
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
AC_CHECK_LIBDAP([3.5.2],
 [
  LIBS="$LIBS $DAP_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])

echo "LIBS after check_libdap: $LIBS"
echo "..and CPPFLAGS: $CPPFLAGS"

dnl Check for the hdf4 library.
AC_DEFUN([AC_CHECK_HDF4],
[
AC_ARG_WITH([hdf4],
            [AS_HELP_STRING([--with-hdf4=ARG],[hdf4 directory])],
            [H4_PATH=${withval}], 
            [H4_PATH="/usr/local/hdf4.2r1"])

AC_ARG_WITH([hdf4_inc],
            [AS_HELP_STRING([--with-hdf4-include=ARG],[hdf4 include directory])],
            [H4_PATH_INC=${withval}], 
            [H4_PATH_INC=""])

AC_ARG_WITH([hdf4_libdir],
            [AS_HELP_STRING([--with-hdf4-libdir=ARG],[hdf4 library directory])],
            [H4_PATH_LIBDIR=${withval}], 
            [H4_PATH_LIBDIR=""])

AS_IF([test "z$H4_PATH_LIBDIR" = "z"],[H4_PATH_LIBDIR="$H4_PATH/lib"])

AS_IF([test "z$H4_PATH_INC" = "z"],[H4_PATH_INC="$H4_PATH/include"])

H4_LIBS=
H4_LDFLAGS=
ac_hdf4_save_LDFLAGS="$LDFLAGS"
ac_hdf4_save_LIBS="$LIBS"
LDFLAGS="$LDFLAGS -L${H4_PATH_LIBDIR}"

# Checks for libraries.
AC_CHECK_LIB([z], [deflate],
   [
      ac_hdf4_z_ok="yes"
      LIBS="$LIBS -lz"
   ],
   [ac_hdf4_z_ok="no"]
)
AC_CHECK_LIB([sz], [SZ_BufftoBuffCompress],
   [
      ac_hdf4_sz_ok="yes"
      LIBS="$LIBS -lsz"
   ],
   [ac_hdf4_sz_ok="no"]
)
AC_CHECK_LIB([jpeg], [jpeg_start_compress],
   [
      ac_hdf4_jpeg_ok="yes"
      LIBS="-ljpeg $LIBS"
   ],
   [ac_hdf4_jpeg_ok="no"]
)
AC_CHECK_LIB([df], [Hopen], 
   [
      ac_hdf4_df_ok="yes"
      LIBS="-ldf $LIBS"
   ],
   [ac_hdf4_df_ok="no"]
)
AC_CHECK_LIB([mfhdf], [SDstart],
   [
      ac_hdf4_mfhdf_ok="yes"
      LIBS="-lmfhdf $LIBS"
   ],
   [ac_hdf4_mfhdf_ok="no"]
)

ac_hdf4_ok="no"
AS_IF([test "$ac_hdf4_z_ok" = "yes" \
	-a "$ac_hdf4_mfhdf_ok" = "yes" \
	-a "$ac_hdf4_jpeg_ok" = "yes" \
	-a "$ac_hdf4_df_ok" = "yes"],
    [ac_hdf4_ok="yes"]
)

AS_IF([test "$ac_hdf4_ok" = "yes"],
    [H4_LDFLAGS="-L${H4_PATH_LIBDIR}";\
     H4_LIBS="$LIBS"]
)

LIBS="$ac_hdf4_save_LIBS"
LDFLAGS="$ac_hdf4_save_LDFLAGS"

AC_SUBST([H4_LDFLAGS])
AC_SUBST([H4_LIBS])

H4_CPPFLAGS=
ac_hdf4_h="no"
ac_hdf4_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$H4_PATH_INC"
AC_CHECK_HEADERS([hdf.h],
  [ac_hdf4_h="yes"],
  [ac_hdf4_h="no"]
)
CPPFLAGS=$ac_hdf4_save_CPPFLAGS

AS_IF([test "$ac_hdf4_h" = "yes" ],
  [H4_CPPFLAGS="-I$H4_PATH_INC"]
)
AC_SUBST([H4_CPPFLAGS])

AS_IF([test "$ac_hdf4_ok" = "no" -o "$ac_hdf4_h" = "no"],
  [m4_if([$2], [], [:], [$2])],
  [m4_if([$1], [], [:], [$1])]
)

])

AC_CHECK_HDF4(
   [
       LIBS="$LIBS $H4_LIBS"
       LDFLAGS="$LDFLAGS $H4_LDFLAGS"
       CPPFLAGS="$CPPFLAGS $H4_CPPFLAGS"
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
