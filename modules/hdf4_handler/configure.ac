# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.63)
AC_INIT([hdf4_handler], [3.13.1], [opendap-tech@opendap.org])
AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])

AM_INIT_AUTOMAKE([1.10 tar-pax])

dnl add c++11 flag, copied from bes. 
cxx_debug_flags="-g -O2"
CXX_STD_FLAG=""

CXX_FLAGS_CHECK([--std=c++11], [CXX_STD_FLAG=--std=c++11], [])

AS_IF([test -z "$CXX_STD_FLAG"],
      [CXX_FLAGS_CHECK([--std=c++0x], [CXX_STD_FLAG=--std=c++0x], [])])

AS_IF([test -z "$CXX_STD_FLAG"],
      [AC_MSG_ERROR([Not using C++-11 (or C++0x)])],
      [AC_MSG_NOTICE([Using $CXX_STD_FLAG for C++-11 support])
       cxx_debug_flags="$cxx_debug_flags --pedantic $CXX_STD_FLAG"])

AC_SUBST(CXX_STD_FLAG)

AS_IF([test -n "$cxx_debug_flags"], [CXXFLAGS="$cxx_debug_flags"])

AM_CONDITIONAL([DAP_BUILTIN_MODULES], [false])

# The handler is a shared object library, so we don't normally need
# the static lib. Override this default by specifying `--enable-static',
# which can make running unit tests with valgrind far more pleasant. 
AM_DISABLE_STATIC

AC_CONFIG_TESTDIR(bes-testsuite, [.])

PACKAGE_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\1@'`
PACKAGE_MINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\2@'`
PACKAGE_SUBMINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\3@'`
AC_MSG_NOTICE(Package Major Version:     $PACKAGE_MAJOR_VERSION)
AC_MSG_NOTICE(Package Minor Version:     $PACKAGE_MINOR_VERSION)
AC_MSG_NOTICE(Package SubMinor Version:  $PACKAGE_SUBMINOR_VERSION)
AC_SUBST(PACKAGE_MAJOR_VERSION)
AC_SUBST(PACKAGE_MINOR_VERSION)
AC_SUBST(PACKAGE_SUBMINOR_VERSION)

AC_CONFIG_SRCDIR([HDFStr.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_AWK
AC_PROG_INSTALL
AM_PROG_LEX
AC_LIBTOOL_DLOPEN
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

AC_ARG_ENABLE([coverage],
    [AS_HELP_STRING([--enable-coverage], [Build so tests emit coverage data and enable coverage target (default: no)])])

AC_CHECK_LIB([gcov], [gcov_open], [GCOV_LIBS="-lgcov"], [GCOV_LIBS=])

AS_IF([test x$enable_coverage = xyes && which gcov],
      [AC_MSG_NOTICE([Building coverage version])
       AM_CONDITIONAL([ENABLE_COVERAGE], [true])
       AS_IF([gcov -help | grep LLVM],
       	     [GCOVR_FLAGS=],
	         [GCOVR_FLAGS="-k -e '.*Test.cc' -e '.*T.cc' -e '.*-test.cc'"
	          LIBS="-lgcov $LIBS"])],
      [AC_MSG_NOTICE([Not building coverage version])
       AS_IF([test x$enable_coverage = xyes],
             [AC_MSG_NOTICE([Check that gcov is on your PATH])])
       AM_CONDITIONAL([ENABLE_COVERAGE], [false])])

# --xml-pretty -o gcovr_ouput.xml

AC_SUBST([GCOVR_FLAGS])

dnl Checks for specific libraries
AC_CHECK_LIBDAP([3.13.0],
 [
  LIBS="$LIBS $DAP_SERVER_LIBS"
  CPPFLAGS="$CPPFLAGS $DAP_CFLAGS"
 ],
 [ AC_MSG_ERROR([Cannot find libdap])
])

dnl echo "LIBS after check_libdap: $LIBS"
dnl echo "..and CPPFLAGS: $CPPFLAGS"

AC_CHECK_HDF4(
   [
       LIBS="$LIBS $HDF4_LIBS"
       LDFLAGS="$LDFLAGS $HDF4_LDFLAGS"
       CPPFLAGS="$CPPFLAGS $HDF4_CFLAGS"
   ],
   [
       AC_MSG_NOTICE([*** Use --with-hdf4 for the root hdf4 directory.])
       AC_MSG_NOTICE([*** Otherwise use --with-hdf4-include switch for includes directory])
       AC_MSG_NOTICE([*** and --with-hdf4-libdir switch for libraries directory.])
       AC_MSG_ERROR([hdf4 library and hdf4 headers are required.])
   ]
)

AC_CHECK_BES([3.13.0], [], [ AC_MSG_ERROR([Cannot find bes]) ])

dnl check for an explict server cache pathname. If so, use that. Otherwise
dnl use /usr/tmp.

dnl ----------------------------------------------------------------------                                
dnl Test for HDF-EOS2                                                                                     

AC_ARG_WITH([hdfeos2],
            [AC_HELP_STRING([--with-hdfeos2=DIR],
                            [Specify path to external hdfeos2 library.])],
            [if test "x$withval" != "xno" -a "x$withval" != "xyes"; then
               HDFEOS2_DIR=$withval
             fi])

if test -n "$HDFEOS2_DIR"; then
  CPPFLAGS="$CPPFLAGS -I$HDFEOS2_DIR/include -DUSE_HDFEOS2_LIB"
  LDFLAGS="$LDFLAGS -L$HDFEOS2_DIR/lib"
  HDFEOS2_LIBS="-lhdfeos -lGctp"
  dnl  AC_CHECK_LIB(Gctp, inv_init,, AC_MSG_ERROR([invalid Gctp in hdfeos2]))
  dnl  AC_CHECK_LIB(hdfeos, GDopen,, AC_MSG_ERROR([invalid hdfeos in hdfeos2]))
  AC_MSG_NOTICE([HDFEOS2 set to true])
  AM_CONDITIONAL([HDFEOS2], [true])
else
  AC_MSG_NOTICE([HDFEOS2 set to false])
  AM_CONDITIONAL([HDFEOS2], [false])
fi

dnl Check if having getdap4. If yes, link with DAP4.
dnl AC_CHECK_PROGS([GETDAP4],[getdap4])
dnl if test -n "$GETDAP4"; then
dnl    CPPFLAGS="$CPPFLAGS -DUSE_DAP4"
dnl fi

AC_SUBST([HDFEOS2_LIBS])
AC_SUBST([HDFEOS2_DIR])

## This sets up the special symbols used to build the bes.conf file for
## the tests. Previously, this was done in the Makefile.am down in the
## test directory, but building the bes.conf here works better with
## autoconf overall and esp. with the 'modules' build. jhrg 11/19/14
modulesdir=$BES_MODULE_DIR
AC_SUBST(modulesdir)

AC_CONFIG_FILES([Makefile
		 bes-testsuite/Makefile
		 bes-testsuite/atlocal
		 bes-testsuite/bes.conf
		 bes-testsuite/bes.with_hdfeos2.conf
         	 hdfclass/Makefile])
AC_CONFIG_FILES(bes-testsuite/generate_metadata_baseline.sh, chmod +x bes-testsuite/generate_metadata_baseline.sh)

dnl Add this later: hdf-testfiles/Makefile
AC_OUTPUT
