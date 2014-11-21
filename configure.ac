dnl -*- automake -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

dnl Update version here and below at LIB_CURRENT, ..., if needed.
AC_INIT(bes, 3.13.2, support@opendap.org)
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

PACKAGE_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\1@'`
PACKAGE_MINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\2@'`
PACKAGE_SUBMINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\3@'`
AC_MSG_NOTICE(Package Major Version:     $PACKAGE_MAJOR_VERSION)
AC_MSG_NOTICE(Package Minor Version:     $PACKAGE_MINOR_VERSION)
AC_MSG_NOTICE(Package SubMinor Version:  $PACKAGE_SUBMINOR_VERSION)
AC_SUBST(PACKAGE_MAJOR_VERSION)
AC_SUBST(PACKAGE_MINOR_VERSION)
AC_SUBST(PACKAGE_SUBMINOR_VERSION)

dnl Add valgrind options
DODS_GCOV_VALGRIND

AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])

AM_INIT_AUTOMAKE

AC_CONFIG_TESTDIR(cmdln/testsuite, [.])
AC_CONFIG_TESTDIR(dapreader/bes-testsuite, [.])
AC_CONFIG_TESTDIR(functions/tests, [.])

dnl AC_CONFIG_TESTDIR(dap/bes-testsuite, [.])

dnl flags for the compilers and linkers - set these before locating the
dnl actual tools since some of the AC_PROG macros set these `flag variables'
dnl to default values otherwise.
AC_CANONICAL_HOST
AC_SUBST(host)

dnl library visioning: Update these when the interface changes. Generally,
dnl assume that the interface tracks the major and minor release numbers.
LIB_DIS_CURRENT=13
LIB_DIS_AGE=5
LIB_DIS_REVISION=0
AC_SUBST(LIB_DIS_CURRENT)
AC_SUBST(LIB_DIS_AGE)
AC_SUBST(LIB_DIS_REVISION)
LIBDISPATCH_VERSION="$LIB_DIS_CURRENT:$LIB_DIS_REVISION:$LIB_DIS_AGE"
AC_SUBST(LIBDISPATCH_VERSION)

LIB_PPT_CURRENT=4
LIB_PPT_AGE=0
LIB_PPT_REVISION=5
AC_SUBST(LIB_PPT_CURRENT)
AC_SUBST(LIB_PPT_AGE)
AC_SUBST(LIB_PPT_REVISION)
LIBPPT_VERSION="$LIB_PPT_CURRENT:$LIB_PPT_REVISION:$LIB_PPT_AGE"
AC_SUBST(LIBPPT_VERSION)

LIB_XML_CMD_CURRENT=2
LIB_XML_CMD_AGE=1
LIB_XML_CMD_REVISION=4
AC_SUBST(LIB_XML_CMD_CURRENT)
AC_SUBST(LIB_XML_CMD_AGE)
AC_SUBST(LIB_XML_CMD_REVISION)
LIBXMLCOMMAND_VERSION="$LIB_XML_CMD_CURRENT:$LIB_XML_CMD_REVISION:$LIB_XML_CMD_AGE"
AC_SUBST(LIBXMLCOMMAND_VERSION)

AC_CHECK_LIB([pthread], [pthread_kill],
[PTHREAD_LIBS="-lpthread"],
[AC_MSG_ERROR([I could not find pthreads])])
AC_SUBST([PTHREAD_LIBS])

AC_DEFINE([DAPREADER_PACKAGE], ["dapreader_module"], [dapreader_module])
AC_DEFINE([DAPREADER_VERSION], ["0.0.1"], [0.0.1])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LEX
dnl We really need bison and not yacc. If you use AC_PROG_YACC, the resulting 
dnl Makefile will call bison -y which doesn't know how to make the parsers 
dnl we require. jhrg 6/15/05
AC_CHECK_PROG(YACC,[bison],[bison])

dnl Use libtool instead?
dnl AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_CHECK_PROG(VALGRIND, valgrind, [valgrind --logfile=memcheck])

dnl Perl is  used by www-interface during the build
AC_CHECK_PROG([PERL], [perl], [`which perl`])

dnl We need the *-config scripts to build, so the AC_CHECK_LIB 
dnl macro is not needed.
AC_PATH_PROG([CURL], [curl], [*** Where is curl? ***], [/bin:/usr/bin])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h stdlib.h limits.h unistd.h pthread.h bzlib.h string.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL
AC_CHECK_TYPES([ptrdiff_t])

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(strdup strftime timegm mktime atexit strtol strchr)
AC_SYS_LARGEFILE

BES_OLD_LIBS=$LIBS
dnl z and bz2 library?
AC_CHECK_LIB( bz2, BZ2_bzReadOpen,
    [
	AM_CONDITIONAL([BZ2UNCOMPRESS], [true])
	BES_BZ2_LIBS=-lbz2
	AC_DEFINE([HAVE_LIBBZ2], [1], [libbz2])
    ],
    [ AM_CONDITIONAL([BZ2UNCOMPRESS], [false]) ] )
AC_CHECK_LIB( z, gzopen, [BES_ZLIB_LIBS=-lz])

dnl dl lib?
AC_CHECK_FUNC(dlclose, , [ AC_CHECK_LIB(dl, dlopen, [BES_DL_LIBS=-ldl]) ])
LIBS=$BES_OLD_LIBS

AC_SUBST(BES_DL_LIBS)
AC_SUBST(BES_ZLIB_LIBS)
AC_SUBST(BES_BZ2_LIBS)

dnl Checks for libraries.
dnl AC_CHECK_LIB(termcap, main)


## Locate GDAL libraries if possible
##
GDAL_FOUND=

AC_ARG_WITH(gdal, AS_HELP_STRING([--with-gdal], [Use the copy of GDAL at this location]),
            with_gdal_prefix="$withval", with_gdal_prefix="")

if test -n "$with_gdal_prefix" -a -x $with_gdal_prefix/bin/gdal-config
then
    AC_MSG_NOTICE([Using $with_gdal_prefix as the GDAL prefix directory.])
    GDAL_LDFLAGS="`$with_gdal_prefix/bin/gdal-config --libs`"
    GDAL_CFLAGS="`$with_gdal_prefix/bin/gdal-config --cflags`"
    GDAL_FOUND="yes"
elif test -n "$with_gdal_prefix"
then
    AC_MSG_ERROR([You set the gdal-prefix directory to $with_gdal_prefix, but gdal-config is not there.])
fi

if test -z "$GDAL_FOUND"
then
    AX_LIB_GDAL([1.10.0])
    if test ! -z "$GDAL_CFLAGS" -a ! -z "$GDAL_LDFLAGS"; then
        GDAL_FOUND="yes"
    fi
fi

AM_CONDITIONAL([GDAL_FOUND], [test x$GDAL_FOUND = xyes])

if test -z "$GDAL_FOUND"
then
    AC_MSG_WARN([I could not find GDAL. GDAL dependent code will not be built.])
else 
    AC_MSG_NOTICE([GDAL_CFLAGS:  $GDAL_CFLAGS])
    AC_MSG_NOTICE([GDAL_LDFLAGS: $GDAL_LDFLAGS])
fi

# check for readline
VL_LIB_READLINE
if test "$vl_cv_lib_readline" = "no"; then
   AC_MSG_ERROR([I could not find the readline library!])
fi

xmlprivatereq=
xmlprivatelibs=
xml_required_version=2.6.16
libdap_pkgconfig_libxml2=yes
libdap_libxml2_module='libxml-2.0 >= $xml_required_version'
PKG_CHECK_MODULES([XML2],[$libdap_libxml2_module],,
  [libdap_pkgconfig_libxml2=no])
if test $libdap_pkgconfig_libxml2 = 'yes'
then
	xmlprivatereq=$libdap_libxml2_module
	XML2_STATIC_LIBS="`$PKG_CONFIG --static --libs libxml-2.0`"
	AC_MSG_RESULT([yes; used pkg-config])
elif xml2-config --version > /dev/null 2>&1
then
	AC_MSG_RESULT([no; used pkg-config])
        AC_MSG_CHECKING([for libxml2 version >= $xml_required_version])
	version_libxml2=`xml2-config --version`

        AS_VERSION_COMPARE(["$version_libxml2"], ["$xml_required_version"], 
                [AC_MSG_RESULT([no])
                 AC_MSG_ERROR([I could not find libxml2 $xml_required_version or newer, found $version_libxml2])])
	
	XML2_LIBS="`xml2-config --libs`"
	XML2_STATIC_LIBS=$XML2_LIBS
	XML2_CFLAGS="`xml2-config --cflags`"
	xmlprivatelibs="`xml2-config --libs`" # `
	AC_MSG_RESULT([yes; used xml2-config])
else
	AC_MSG_ERROR([I could not find libxml2])
fi
AC_SUBST([xmlprivatereq])
AC_SUBST([xmlprivatelibs])
AC_SUBST([XML2_LIBS])
AC_SUBST([XML2_STATIC_LIBS])
AC_SUBST([XML2_CFLAGS])

SIC_VAR_SYS_ERRLIST

BES_CHECK_OPENSSL
dnl BES_CHECK_KERBEROS

AC_CHECK_LIBDAP([3.11.0],
  [
    AM_CONDITIONAL([LIBDAP], [true])
  ],
  [
    AM_CONDITIONAL([LIBDAP], [false])
  ]
)

AC_ARG_ENABLE([developer],
  [
    AS_HELP_STRING([--enable-developer],[developer mode])
  ],
  [
    AM_CONDITIONAL([BES_DEVELOPER], [true])
  ],
  [
    AM_CONDITIONAL([BES_DEVELOPER], [false])
  ]
)

dnl extra argument: --with-libwrap
AC_MSG_CHECKING(whether to use libwrap)
AC_ARG_WITH(libwrap,
    [  --with-libwrap          Compile in libwrap (tcp_wrappers) support.],
    [ case "$withval" in
    yes) AC_MSG_RESULT(yes)
	AC_CHECK_LIB(wrap, request_init, 
	     [ LIBS="-lwrap $LIBS"
	       AC_DEFINE([HAVE_LIBWRAP], [1], [Description])],
	     [ AC_MSG_ERROR(Could not find libwrap library. You have to insall tcp-wrappers before using --with-libwrap option to configure.) ])
	;;
    *) AC_MSG_RESULT(no);;
    esac ],
    AC_MSG_RESULT(no)
)

AC_HEADER_TR1_FUNCTIONAL

AM_PATH_CPPUNIT(1.12.0,
	[AM_CONDITIONAL([CPPUNIT], [true])],
	[AM_CONDITIONAL([CPPUNIT], [false])])

## dap-mouldes is true by default; use --disable-dap-modules or
## --enable-dap-modules=no to turn this off. jhrg 11/17/13

AC_ARG_WITH([dap-modules],
  [ AS_HELP_STRING([--without-dap-modules],[don't build the bundled dap modules]) ],
  [ ],
  [ with_dap_modules=yes ])

DAP_MODULES=
AS_IF([test "x$with_dap_modules" != xno],
    [AM_CONDITIONAL([DAP_MODULES], [true])],
    [AM_CONDITIONAL([DAP_MODULES], [false])])

## FIXME Unused at the moment
ac_bes_dependencies_prefix=
AC_ARG_WITH([dependencies],
  [ AS_HELP_STRING([--with-dependencies],[The prefix for the dependencies]) ],
  [ 
    AS_IF([test -n "$with_dependencies"], 
    [ac_bes_dependencies_prefix=$with_dependencies
    AC_MSG_RESULT([Using ${ac_bes_dependencies_prefix} as the dependencies prefix])])
  ],
  [ ])

BES_DISPATCH_LIB="-L\$(top_builddir)/dispatch -lbes_dispatch"
AC_SUBST(BES_DISPATCH_LIB)
BES_XML_CMD_LIB="-L\$(top_builddir)/xmlcommand -lbes_xml_command"
AC_SUBST(BES_XML_CMD_LIB)
BES_PPT_LIB="-L\$(top_builddir)/ppt -lbes_ppt"
AC_SUBST(BES_PPT_LIB)
BES_HANDLER_LIBS="$LIBS $openssl_libs"
AC_SUBST(BES_EXTRA_LIBS)

## These tests are for the dependecies of specific handlers. Added jhrg 11/17/13

# This is used by 'usage' which builds the .info response. It's likely 
# old and should be removed
AC_DEFINE([DAP_PROTOCOL_VERSION], ["3.2"], [What DAP version is supported?])
AC_SUBST([DAP_PROTOCOL_VERSION])

dods_HTML_Filter=no
if $PERL -e "use HTML::Filter;" > /dev/null 2>&1; then 
   dods_HTML_Filter=yes
else 
   dods_HTML_Filter=no
fi

if test "z$dods_HTML_Filter" = "zno"; then
   AC_MSG_WARN([HTML::Filter not found. See INSTALL])
fi  

dnl AC_MSG_CHECKING([the netCDF library])

dnl # CHECK_NETCDF defines $NC_LIBS, $NC_LDFLAGS and $NC_CPPFLAGS"
dnl AC_CHECK_NETCDF(
dnl    [AC_MSG_RESULT($NC_LIBS)],
dnl    [echo "*** Use --with-netcdf for the root netcdf directory."
dnl     echo "*** Otherwise use --with-netcdf-include switch for includes directory"
dnl     echo "*** and --with-netcdf-libdir switch for libraries directory."
dnl     AC_MSG_ERROR([netcdf library and netcdf headers are required.]) ],[3])

dnl AC_MSG_CHECKING([the netCDF library version number])

dnl echo "NC_CPPFLAGS: $NC_CPPFLAGS"

dnl save_CPPFLAGS=$CPPFLAGS
dnl CPPFLAGS="$NC_CPPFLAGS $CPPFLAGS"
dnl AC_EGREP_HEADER(NC_FIRSTUSERTYPEID, netcdf.h, NETCDF_MAJOR_VERSION=4, NETCDF_MAJOR_VERSION=3)
dnl CPPFLAGS=$save_CPPFLAGS

dnl AC_DEFINE_UNQUOTED(NETCDF_VERSION, $NETCDF_MAJOR_VERSION, [What version of netcdf were we built with?])

dnl AC_MSG_RESULT($NETCDF_MAJOR_VERSION)

dnl AM_CONDITIONAL([NETCDF4_TESTS], [test "x$NETCDF_MAJOR_VERSION" = "x4"])

## End handler specific tests

dnl autoheader macros; tack some text at the top and bottom of config.h.in
AH_TOP([#ifndef _config_h
#define _config_h])

AH_BOTTOM([/* Shorthand for gcc's unused attribute feature */
#if defined(__GNUG__) || defined(__GNUC__)
#define not_used __attribute__ ((unused))
#else
#define not_used 
#endif /* __GNUG__ || __GNUC__ */

#endif /* _config_h */])

AC_CONFIG_FILES([Makefile
		 abi_checker.xml
		 dispatch/Makefile
		 dispatch/unit-tests/Makefile
		 docs/Makefile
		 xmlcommand/Makefile
		 xmlcommand/unit-tests/Makefile
		 ppt/Makefile
		 ppt/unit-tests/Makefile
		 cmdln/Makefile
		 cmdln/unit-tests/Makefile
		 cmdln/testsuite/Makefile
		 cmdln/testsuite/atlocal
		 standalone/Makefile
		 server/Makefile
		 server/test/Makefile
		 bin/Makefile
		 templates/Makefile
		 bes_dispatch.pc
		 bes_ppt.pc
		 bes_xml_command.pc

		 dapreader/Makefile
		 dapreader/bes-testsuite/Makefile
		 dapreader/bes-testsuite/atlocal

		 dap/Makefile
		 dap/unit-tests/Makefile
		 dap/unit-tests/test_config.h

		 functions/Makefile
		 functions/tests/Makefile
		 functions/tests/atlocal
 		 functions/tests/package.m4
		 functions/unit-tests/Makefile])

if test -n "$GDAL_FOUND"
then
    AC_CONFIG_FILES([functions/swath2grid/Makefile])
fi

AC_CONFIG_FILES([cmdln/testsuite/generate_data_baseline.sh],
	[chmod +x cmdln/testsuite/generate_data_baseline.sh])
AC_CONFIG_FILES([cmdln/testsuite/generate_metadata_baseline.sh],
	[chmod +x cmdln/testsuite/generate_metadata_baseline.sh])

AC_CONFIG_FILES([functions/tests/generate_data_baseline.sh],
	[chmod +x functions/tests/generate_data_baseline.sh])
AC_CONFIG_FILES([functions/tests/generate_metadata_baseline.sh],
	[chmod +x functions/tests/generate_metadata_baseline.sh])

AC_CONFIG_FILES([bes-config], [chmod +x bes-config]) 
AC_CONFIG_FILES([besd], [chmod +x besd]) 
AC_CONFIG_FILES([server/besctl], [chmod +x server/besctl]) 
AC_CONFIG_FILES([server/hyraxctl], [chmod +x server/hyraxctl]) 

## Output files if building the emodules too
		 
AM_COND_IF([DAP_MODULES],
    [
    AC_CONFIG_FILES([modules/Makefile
    modules/csv_handler/Makefile
    modules/csv_handler/bes-testsuite/Makefile
    modules/csv_handler/bes-testsuite/atlocal
    modules/csv_handler/bes-testsuite/bes.conf:modules/csv_handler/bes-testsuite/bes.conf.modules.in

    modules/dap-server/Makefile
    modules/dap-server/asciival/Makefile
    modules/dap-server/asciival/unit-tests/Makefile
    modules/dap-server/asciival/unit-tests/test_config.h
    modules/dap-server/www-interface/Makefile

    modules/fileout_json/Makefile
    modules/fileout_json/unit-tests/Makefile 
    modules/fileout_json/unit-tests/test_config.h
    modules/fileout_json/bes-testsuite/Makefile 
    modules/fileout_json/bes-testsuite/atlocal
    modules/fileout_json/bes-testsuite/bes.conf:modules/fileout_json/bes-testsuite/bes.conf.modules.in])

    AC_CONFIG_FILES([modules/dap-server/www-interface/js2h.pl], [chmod +x modules/dap-server/www-interface/js2h.pl])

    AC_CONFIG_TESTDIR(modules/csv_handler/bes-testsuite, [standalone])
    AC_CONFIG_TESTDIR(modules/fileout_json/bes-testsuite, [standalone])
    ]
)

AC_OUTPUT

