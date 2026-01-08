dnl -*- automake -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])

dnl Update version here and below at LIB_CURRENT, ..., if needed.
AC_INIT([bes],[3.21.1],[support@opendap.org])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

dnl Provide a way to pass in a build number. This is used by CI/CD systems to record
dnl each build. The version number is set by the value in AC_INIT (see above). For
dnl people building the source, it's not customary to supply a build number (i.e.,
dnl if you're building the code for your own use, a build number seems like overkill).
dnl jhrg 3/16/21 (copied from libdap4's configure.ac)
AC_ARG_WITH([build],
            [AS_HELP_STRING([--with-build=<number>],
                   [Inject the integer build number (default is to not define a build number)])],
            [build_number=${withval}], [build_number=])

dnl If a build number is given and valid (it must be an integer), use it. If it's
dnl given but invalid, print a message and exit.
dnl PACKAGE_VERSION is used in the file bes_VERSION.in - and that file is used to inject
dnl the version or version-build_number into packages. jhrg 3/24/21
AS_IF([test -n "$with_build"],
    [AS_IF([echo $build_number | grep '^[[0-9]][[-0-9.]]*$' > /dev/null 2>&1 ],
     [PACKAGE_VERSION=$PACKAGE_VERSION-$build_number
      AC_MSG_NOTICE([PACKAGE_VERSION: $PACKAGE_VERSION])],
     [AC_MSG_ERROR([Invalid build number given (must be an integer): $build_number])])])

AS_IF([echo $PACKAGE_VERSION | grep '^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)-\([[0-9]]*\)$' > /dev/null 2>&1 ],
      [PACKAGE_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)-\([[0-9]]*\)$@\1@'`
       PACKAGE_MINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)-\([[0-9]]*\)$@\2@'`
       PACKAGE_PATCH_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)-\([[0-9]]*\)$@\3@'`
       PACKAGE_BUILD_NUMBER=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)-\([[0-9]]*\)$@\4@'`],
      [AS_IF([echo $PACKAGE_VERSION | grep '^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$' > /dev/null 2>&1 ],
       [PACKAGE_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\1@'`
        PACKAGE_MINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\2@'`
        PACKAGE_PATCH_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\3@'`
        PACKAGE_BUILD_NUMBER=],
       [AC_MSG_ERROR([VERSION file does not contain a valid version number (x.y.z or x.y.z-b)])])])

AC_MSG_NOTICE(Package Major Version: $PACKAGE_MAJOR_VERSION)
AC_MSG_NOTICE(Package Minor Version: $PACKAGE_MINOR_VERSION)
AC_MSG_NOTICE(Package Patch Version: $PACKAGE_PATCH_VERSION)
AC_MSG_NOTICE(Package Build Number: $PACKAGE_BUILD_NUMBER)
AC_SUBST(PACKAGE_MAJOR_VERSION)
AC_SUBST(PACKAGE_MINOR_VERSION)
AC_SUBST(PACKAGE_PATCH_VERSION)
AC_SUBST(PACKAGE_BUILD_NUMBER)

dnl Add valgrind options - no longer include gcov, see --enable-coverage jhrg 12/18/20
DODS_GCOV_VALGRIND

AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])

AM_INIT_AUTOMAKE([1.10 tar-pax])
LT_INIT

AC_DEFINE_UNQUOTED(CVER, "$PACKAGE_VERSION", [Client version number])
AC_SUBST(CVER)

dnl flags for the compilers and linkers - set these before locating the
dnl actual tools since some of the AC_PROG macros set these 'flag variables'
dnl to default values otherwise.
AC_CANONICAL_HOST
AC_SUBST(host)

dnl This is the default. If the caller uses --enable-developer, that option
dnl will override these values
cxx_debug_flags=

dnl Removed "-g -O2" jhrg 2/8/22
dnl Changed -O2 to -Og for --enable-developer jhrg 6/16/23
dnl Changed -Og to -O0 for --enable-developer to improve ease of debugging.
dnl This can introduce some problems where the optimizers introduces or uncovers
dnl bugs not present with the 'vanilla' code. jhrg 2/26/25

AC_ARG_ENABLE([developer],
    [AS_HELP_STRING([--enable-developer],
    [Developer mode: Won't override CXXFLAGS on the command line])])

dnl Explicitly set -DNDEBUG below when CXXFLAGS is set to ensure that
dnl it's actually used - not all the source files include config.h. jhrg 1/30/23

AM_CONDITIONAL([BES_DEVELOPER], [test "x$enable_developer" = "xyes"])
AM_COND_IF([BES_DEVELOPER],
    [cxx_debug_flags="-g3 -O0 -Wall"
     AC_DEFINE([DEVELOPER], [1], [Define this for Developer-only code sections.])
     AC_MSG_NOTICE([Developer Mode is enabled.]) ],
    [AC_DEFINE([NDEBUG], [1], [Define this to suppress assert() statements.])
     AC_MSG_NOTICE([Developer Mode is disabled.])])

AC_SUBST([DEVELOPER])

AC_ARG_ENABLE([asan],
    [AS_HELP_STRING([--enable-asan],
    [Use the address sanitizer. Won't override CXXFLAGS on the command line])])

AM_CONDITIONAL([USE_ASAN], [test "x$enable_asan" = "xyes"])
AM_COND_IF([USE_ASAN],
    [cxx_debug_flags="$cxx_debug_flags -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-common"
        AC_MSG_NOTICE([Address Sanitizer (ASAN) is enabled.])],
    [AC_MSG_NOTICE([Address Sanitizer (ASAN) is disabled.])])

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

AC_SUBST([GCOVR_FLAGS])

AC_ARG_WITH([cmr],
    [AS_HELP_STRING([--without-cmr],
    [Disable support for the CMR Module (Built by default)])])

AM_CONDITIONAL([WITH_CMR], [test "x$with_cmr" != "xno"])
AM_COND_IF([WITH_CMR],
    [AC_MSG_NOTICE([Building support for the CMR Module])],
    [AC_MSG_NOTICE([Disabled support for the CMR Module])])


dnl  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
dnl Build NGAP support by default or if the user provides --with-ngap. Do not build the
dnl module if the user supplies --without-ngap or --with-ngap=no. jhrg 3/31/20

AC_ARG_WITH([ngap],
    AS_HELP_STRING([--without-ngap],
    [Disable support for the NGAP container. (Built by default)])
)
AM_CONDITIONAL([WITH_NGAP], [test "x$with_ngap" != "xno"])
AM_COND_IF([WITH_NGAP],
    [AC_MSG_NOTICE([Building support for the NGAP Module])],
    [AC_MSG_NOTICE([Disabled support for the NGAP Module])])

dnl Build S3 support by default or if the user provides --with-s3. Do not build the
dnl module if the user supplies --without-s3 or --with-s3=no. jhrg 10/18/22

AC_ARG_WITH([s3],
    AS_HELP_STRING([--without-s3],
    [Disable support for the S3 service, no s3_reader. (Built by default)])
)
AM_CONDITIONAL([BUILD_S3], [test "x$with_s3" != "xno"])
AM_COND_IF([BUILD_S3],
    [AC_MSG_NOTICE([Building support for the S3 Module])],
    [AC_MSG_NOTICE([Disabled support for the S3 Module])])

dnl  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

AS_IF([test "$CC" = "gcc"], [AM_CONDITIONAL([COMPILER_IS_GCC],[true])],
    [AM_CONDITIONAL([COMPILER_IS_GCC],[false])])

dnl library visioning: Update these when the interface changes.
dnl
dnl How to set these SO variables:
dnl No interfaces changed, only implementations (good): ==> Increment REVISION.
dnl Interfaces added, none removed (good): ==> Increment CURRENT,
dnl increment AGE, set REVISION to 0.
dnl Interfaces removed or changed (BAD, breaks upward compatibility):
dnl ==> Increment CURRENT, set AGE and REVISION to 0.

LIB_DIS_CURRENT=18
LIB_DIS_REVISION=11
LIB_DIS_AGE=0
AC_SUBST(LIB_DIS_CURRENT)
AC_SUBST(LIB_DIS_REVISION)
AC_SUBST(LIB_DIS_AGE)
LIBDISPATCH_VERSION="$LIB_DIS_CURRENT:$LIB_DIS_REVISION:$LIB_DIS_AGE"
AC_SUBST(LIBDISPATCH_VERSION)

LIB_PPT_CURRENT=5
LIB_PPT_REVISION=9
LIB_PPT_AGE=0
AC_SUBST(LIB_PPT_CURRENT)
AC_SUBST(LIB_PPT_REVISION)
AC_SUBST(LIB_PPT_AGE)
LIBPPT_VERSION="$LIB_PPT_CURRENT:$LIB_PPT_REVISION:$LIB_PPT_AGE"
AC_SUBST(LIBPPT_VERSION)

LIB_XML_CMD_CURRENT=5
LIB_XML_CMD_REVISION=10
LIB_XML_CMD_AGE=0
AC_SUBST(LIB_XML_CMD_CURRENT)
AC_SUBST(LIB_XML_CMD_REVISION)
AC_SUBST(LIB_XML_CMD_AGE)
LIBXMLCOMMAND_VERSION="$LIB_XML_CMD_CURRENT:$LIB_XML_CMD_REVISION:$LIB_XML_CMD_AGE"
AC_SUBST(LIBXMLCOMMAND_VERSION)

AC_CHECK_LIB([pthread], [pthread_kill],
    [PTHREAD_LIBS="-lpthread"],
    [AC_MSG_ERROR([I could not find pthreads])])
    
AC_SUBST([PTHREAD_LIBS])

dnl This seems like it doesn't belong here but in the module, instead.
dnl TODO. jhrg 11/28/17
AC_DEFINE([DAPREADER_PACKAGE], ["dapreader_module"], [dapreader_module])
AC_DEFINE([DAPREADER_VERSION], ["0.0.1"], [0.0.1])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_INSTALL
dnl AC_PROG_LEX

dnl Look for Python3. Then set a flag if we found Python 3.9 or greater. 3.9++ is
dnl needed to run one of the tests in http/tests. jhrg 6/5/23

PYTHON=python3
AM_PATH_PYTHON()
python_major_version=$(echo $PYTHON_VERSION | sed 's@\([[0-9]]*\)\.\([[0-9]]*\)@\1@g')
python_minor_version=$(echo $PYTHON_VERSION | sed 's@\([[0-9]]*\)\.\([[0-9]]*\)@\2@g')
AC_MSG_CHECKING([Python version 3.9 or greater])
AM_CONDITIONAL([HAVE_PYTHON_3_9], [test $python_major_version -ge  3 -a $python_minor_version -ge 9])
AM_COND_IF([HAVE_PYTHON_3_9], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

dnl Here we test for both C++11 and 14, but our code nominally requires 14.
dnl We can use the C++11 test and CXX_STD_FLAG to build with 11 if we add a
dnl compatibility layer for the missing features. jhrg 1/30/23.

CXX_STD_FLAG=""

CXX_FLAGS_CHECK([--std=c++11], [CXX_STD_FLAG=--std=c++11], [AC_MSG_WARN([C++11 is required but not found.])])
CXX_FLAGS_CHECK([--std=c++14], [CXX_STD_FLAG=--std=c++14],
    [AC_MSG_WARN([C++14 is required but not found. Using C++11 compatability mode.])])

AS_IF([test -z "$CXX_STD_FLAG"],
      [AC_MSG_ERROR([Not using modern C++ (C++11 or 14)])],
      [AC_MSG_NOTICE([Using $CXX_STD_FLAG])
       cxx_debug_flags="$cxx_debug_flags --pedantic $CXX_STD_FLAG"])

AC_SUBST(CXX_STD_FLAG)

AS_IF([test -n "$cxx_debug_flags"],
      [CXXFLAGS="$CXXFLAGS $cxx_debug_flags"],
      [CXXFLAGS="$CXXFLAGS $CXX_STD_FLAG -DNDEBUG"])

dnl We really need bison and not yacc. If you use AC_PROG_YACC, the resulting 
dnl Makefile will call bison -y which does not know how to make the parsers 
dnl we require. jhrg 6/15/05
AC_CHECK_PROG(YACC,[bison],[bison])

dnl Use libtool instead

LT_INIT

AC_SEARCH_LIBS([uuid_generate], [uuid],
    [],
    [AC_MSG_ERROR([I could not find a library with uuid_generate])])

AC_CHECK_PROG(VALGRIND, valgrind, [valgrind --logfile=memcheck])

dnl Perl is used by www-interface during the build
dnl Removed www-interface. jhrg 3/21/22
dnl AC_CHECK_PROG([PERL], [perl], [`which perl`])

dnl We need the *-config scripts to build, so the AC_CHECK_LIB 
dnl macro is not needed.
AC_PATH_PROG([CURL], [curl], [Where is curl?], [/bin:/usr/bin:/usr/local/bin])

dnl The DMR++ handler uses, conditionally, the Multi API. These tests determine
dnl if that API is present (iff the multi.h header is present). jhrg 8/27/18
AC_CHECK_HEADERS([curl/curl.h])
AC_CHECK_HEADERS([curl/multi.h])

AC_SEARCH_LIBS([curl_multi_wait], [curl],
    [AC_DEFINE([HAVE_CURL_MULTI_API],[1],[Does libcurl have the multi API])],
    [AC_DEFINE([HAVE_CURL_MULTI_API],[0],[Does libcurl have the multi API])], [])

AC_CHECK_HEADERS_ONCE(fcntl.h float.h malloc.h stddef.h stdlib.h limits.h unistd.h)
AC_CHECK_HEADERS_ONCE(pthread.h bzlib.h string.h strings.h byteswap.h)
dnl AC_CHECK_HEADERS_ONCE([uuid/uuid.h uuid.h])
dnl Do this because we have had a number of problems with the UUID header/library
AC_CHECK_HEADERS([uuid/uuid.h],[found_uuid_uuid_h=true],[found_uuid_uuid_h=false])
AC_CHECK_HEADERS([uuid.h],[found_uuid_h=true],[found_uuid_h=false])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL
AC_CHECK_TYPES([ptrdiff_t])
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(strdup strftime strtol strcasecmp strcspn strerror strncasecmp)
AC_CHECK_FUNCS(strpbrk strchr strrchr strspn strtoul)
AC_CHECK_FUNCS(timegm mktime atexit floor isascii memmove memset pow sqrt)

dnl Make sure we have the cctype library
AC_SEARCH_LIBS([isdigit], [cctype])

AC_SYS_LARGEFILE
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_FUNC_VPRINTF

OPENSSL_LIBS=-lcrypto

AC_SUBST([OPENSSL_LIBS])

OPENSSL_INC=-I/usr/local/opt/openssl/include
OPENSSL_LDFLAGS=-L/usr/local/opt/openssl/lib

AC_CHECK_HEADER([openssl/sha.h], [], [openssl_found=false])

AS_IF([test ! $openssl_found],
    [
    AC_SUBST([OPENSSL_INC])
    AC_SUBST([OPENSSL_LDFLAGS])
    ])

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
AC_CHECK_FUNC(dlclose, [], [ AC_CHECK_LIB(dl, dlopen, [BES_DL_LIBS=-ldl]) ])

LIBS=$BES_OLD_LIBS

AC_SUBST(BES_DL_LIBS)
AC_SUBST(BES_ZLIB_LIBS)
AC_SUBST(BES_BZ2_LIBS)

dnl Checks for libraries.

# check for readline
VL_LIB_READLINE
AS_IF([test "$vl_cv_lib_readline" = "no"], [AC_MSG_ERROR([I could not find the readline library!])])

xmlprivatereq=
xmlprivatelibs=
xml_required_version=2.6.16
libdap_pkgconfig_libxml2=yes
libdap_libxml2_module='libxml-2.0 >= $xml_required_version'

PKG_CHECK_MODULES([XML2],[$libdap_libxml2_module], [], [libdap_pkgconfig_libxml2=no])
 
AS_IF([test $libdap_pkgconfig_libxml2 = 'yes'],
    [
    xmlprivatereq=$libdap_libxml2_module
    XML2_STATIC_LIBS="`$PKG_CONFIG --static --libs libxml-2.0`"
    AC_MSG_RESULT([yes; used pkg-config])
    ],
    [xml2-config --version > /dev/null 2>&1],
    [
    AC_MSG_RESULT([no; used pkg-config])
    AC_MSG_CHECKING([for libxml2 version >= $xml_required_version])
    version_libxml2=`xml2-config --version`
    AS_VERSION_COMPARE(["$version_libxml2"], ["$xml_required_version"], 
        [
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([I could not find libxml2 $xml_required_version or newer, found $version_libxml2])
        ])
    XML2_LIBS="`xml2-config --libs`"
    XML2_STATIC_LIBS=$XML2_LIBS
    XML2_CFLAGS="`xml2-config --cflags`"
    xmlprivatelibs="`xml2-config --libs`" # `
    AC_MSG_RESULT([yes; used xml2-config])
    ],
    [AC_MSG_ERROR([I could not find libxml2])])
    
AC_SUBST([xmlprivatereq])
AC_SUBST([xmlprivatelibs])
AC_SUBST([XML2_LIBS])
AC_SUBST([XML2_STATIC_LIBS])
AC_SUBST([XML2_CFLAGS])

dnl Removed jhrg 1/30/23 SIC_VAR_SYS_ERRLIST

dnl jhrg 10/14/15 BES_CHECK_OPENSSL
dnl BES_CHECK_KERBEROS

AC_CHECK_LIBDAP([3.18.0], [AM_CONDITIONAL([LIBDAP], [true])], [AM_CONDITIONAL([LIBDAP], [false])])
AC_MSG_CHECKING([on libdap])
AM_COND_IF([LIBDAP], [AC_MSG_RESULT([yes])], [AC_MSG_ERROR([Could not find libdap.])])

AC_MSG_CHECKING([on libdap])
AM_COND_IF([LIBDAP], [AC_MSG_RESULT([yes])], [AC_MSG_ERROR([Could not find libdap.])])

dnl AS_CASE([$variable], [foo*], [run1], [bar*], [run2], [catchall])
    
dnl extra argument: --with-libwrap
AC_MSG_CHECKING(whether to use libwrap)
AC_ARG_WITH(libwrap,
    [AS_HELP_STRING([--with-libwrap], [Compile in libwrap (tcp_wrappers) support.])],
    [AS_CASE([$withval],
        [yes],  
        [
            AC_MSG_RESULT(yes)
            AC_CHECK_LIB(wrap, request_init, [LIBS="-lwrap $LIBS" AC_DEFINE([HAVE_LIBWRAP], [1], [Description])],
                [AC_MSG_ERROR(Could not find libwrap library. You have to install tcp-wrappers before using --with-libwrap.)])
        ],
        [AC_MSG_RESULT(no)])],  dnl The default case (!= yes) is no.
    [AC_MSG_RESULT(no)])

# Check for cppunit to enable unit testing.
AM_PATH_CPPUNIT(1.12.0,
	[AM_CONDITIONAL([CPPUNIT], [true])],
	[AM_CONDITIONAL([CPPUNIT], [false])])

dnl Check for RHEL 8 and tirpc
OX_RHEL8_TIRPC

dnl AC_ARG_WITH([dap-exp-modules],
dnl   [AS_HELP_STRING([--with-dap-exp-modules], [build the experimental dap modules (default is no)])],
dnl   [with_dap_exp_modules=${withval} ],
dnl   [with_dap_exp_modules=no])

dnl AM_CONDITIONAL([DAP_EXP_MODULES], [test x$with_dap_exp_modules = xyes])

dnl dap-builtin-mouldes is true by default; use --disable-dap-builtin-modules or
dnl --enable-dap-builtin-modules=no to turn this off. jhrg 11/17/13

AC_ARG_WITH([dap-builtin-modules],
  [AS_HELP_STRING([--with-dap-builtin-modules], [build the builtin dap modules (default is yes)])],
  [with_dap_builtin_modules=${withval} ],
  [with_dap_builtin_modules=yes])

AM_CONDITIONAL([DAP_BUILTIN_MODULES], [test x$with_dap_builtin_modules = xyes])

dnl These are tests that only apply if the modules are being built too.
AM_COND_IF([DAP_BUILTIN_MODULES], [

ac_bes_dependencies_prefix=
AC_MSG_CHECKING([if the dependencies prefix has been set])
AC_ARG_WITH([dependencies],
	[AS_HELP_STRING([--with-dependencies],[The prefix for the dependencies])],
    [AS_IF([test -n "$with_dependencies"], 
           [ac_bes_dependencies_prefix=$with_dependencies
            dnl This conditional and symbol substitution pass the deps value to the 
            dnl top level makefile. jhrg 3/18/15
            AM_CONDITIONAL([WITH_DEPENDENCIES], [true])
            AC_SUBST([ac_bes_dependencies_prefix])
            AC_MSG_RESULT([yes ($ac_bes_dependencies_prefix)])])],
           [AM_CONDITIONAL([WITH_DEPENDENCIES], [false])])


# Help the link step find libs at build time too (not just run time)
deps_libdir=
deps_libdir64=
AS_IF([test -n "$ac_bes_dependencies_prefix"],
    [deps_libdir=$ac_bes_dependencies_prefix/lib
     AC_SUBST([deps_libdir])
     AS_IF([test -d "${deps_libdir}64"],
        [deps_libdir64=${deps_libdir}64
         AC_SUBST([deps_libdir64])])])

# If deps_libdir is set above to a special dependencies dir, then do the following
# to support dynamic linking with the dependencies given that they are not installed
# in a 'standard' location. jhrg 1/7/26

AS_IF([test -n "$deps_libdir"],
   [AM_LDFLAGS="$AM_LDFLAGS -L$deps_libdir"
    echo "AM_LDFLAGS is $AM_LDFLAGS"
    AS_IF([test -n "$deps_libdir64"],[AM_LDFLAGS="$AM_LDFLAGS -L$deps_libdir64"])
        AM_CONDITIONAL([DARWIN],
         [AM_LDFLAGS="$AM_LDFLAGS -Wl,-rpath,${deps_libdir} -Wl,-rpath,@loader_path"
          echo "AM_LDFLAGS is $AM_LDFLAGS"],
         [AM_LDFLAGS="$AM_LDFLAGS -Wl,-rpath,$deps_libdir -Wl,-rpath,''$ORIGIN''"
          AS_IF([test -n "$deps_libdir64"], [AM_LDFLAGS="$AM_LDFLAGS -Wl,-rpath,$deps_libdir64"])])
    AC_SUBST([AM_LDFLAGS])])

AC_MSG_CHECKING([LDFLAGS after dependencies prefix test])
AC_MSG_RESULT([LDFLAGS = $LDFLAGS])


BES_DISPATCH_LIB="-L\$(top_builddir)/dispatch -lbes_dispatch"
AC_SUBST(BES_DISPATCH_LIB)
BES_XML_CMD_LIB="-L\$(top_builddir)/xmlcommand -lbes_xml_command"
AC_SUBST(BES_XML_CMD_LIB)
BES_PPT_LIB="-L\$(top_builddir)/ppt -lbes_ppt"
AC_SUBST(BES_PPT_LIB)
BES_DAP_LIB="-L\$(top_builddir)/dap -ldap_module"
AC_SUBST(BES_DAP_LIB)

dnl This enables code to link with the dap_module without a warning. Use this not
dnl with xxxx_LDADD but with xxxx_LDFLAGS. For test code, which is never installed,
dnl link with .libs/libdap_module.a instead if that's easier.
BES_DAP_LIB_LDFLAGS="-L\$(top_builddir)/dap -static -ldap_module -dynamic"
AC_SUBST(BES_DAP_LIB_LDFLAGS)

dnl jhrg 6/2/23 BES_EXTRA_LIBS="$LIBS $openssl_libs"
dnl AC_SUBST(BES_EXTRA_LIBS)
BES_HTTP_LIB="-L\$(top_builddir)/http -lbes_http"
AC_SUBST(BES_HTTP_LIB)

dnl These tests are for the dependencies of specific handlers. Added jhrg 11/17/13

# This is used by 'usage' which builds the .info response. It's likely 
# old and should be removed
AC_DEFINE([DAP_PROTOCOL_VERSION], ["3.2"], [What DAP version is supported?])
AC_SUBST([DAP_PROTOCOL_VERSION])

dnl DMR++ module support - only when BES_DEVELOPER is set do we need this.
dnl jhrg 11/28/17
dnl
dnl support the --with-curl option for specifying a particular version of curl.
dnl This will be used only when BES_DEVELOPER (aka --enable-developer) is set,
dnl since that is the only time the dmr++ module is build.

AC_ARG_VAR(CURL_LIBS, [libcurl required libraries])
AC_ARG_VAR(CURL_CFLAGS, [libcurl required compiler flags])

AM_COND_IF([BES_DEVELOPER], [

curl_is_ok=no
curl_req_version=7.20.0

AC_ARG_WITH([curl],
    AS_HELP_STRING([--with-curl], [Prefix where curl/libcurl is installed; any version found is used.]),
    with_curl_prefix="$withval",
    with_curl_prefix="")

AS_IF([test -n "$with_curl_prefix" -a -x $with_curl_prefix/bin/curl-config],
    [AC_MSG_CHECKING([for libcurl])
     CURL_LIBS="`$with_curl_prefix/bin/curl-config --libs`"
     CURL_CFLAGS="`$with_curl_prefix/bin/curl-config --cflags`"
     AC_MSG_RESULT([yes; used $with_curl_prefix/bin/curl-config])
     curl_is_ok=yes],
    
    dnl FAIL if $with_curl_prefix is not zero but the script is not executable
    [test -n "$with_curl_prefix"],
    [AC_MSG_ERROR([curl-prefix set to $with_curl_prefix, but curl-config is not there.])])
    
AC_ARG_VAR(PKG_CONFIG, [path to pkg-config utility])
AC_PATH_TOOL([PKG_CONFIG], [pkg-config])

AS_IF([test -n "$PKG_CONFIG"], 
    [AC_MSG_CHECKING([for libcurl using pkg-config])
     AS_IF([pkg-config --atleast-version=$curl_req_version curl],
        [CURL_CFLAGS=`pkg-config --cflags curl`
         CURL_LIBS=`pkg-config --libs curl`
         AC_MSG_RESULT([yes; found at least version $curl_req_version])
         curl_is_ok=yes],
        [AC_MSG_RESULT([no, needed $curl_req_version])])])
    
AC_ARG_VAR(CURL_CONFIG, [path to curl-config utility])
AC_PATH_TOOL([CURL_CONFIG], [curl-config])

AS_IF([test -n "$CURL_CONFIG"], 
    [AC_MSG_CHECKING([for libcurl using curl-config])
     version_libcurl=`curl-config --version | sed 's@libcurl \(.*\)@\1@'`
     AS_VERSION_COMPARE([$version_libcurl], [$curl_req_version], [curl_is_ok=no], [curl_is_ok=yes], [curl_is_ok=yes])
     AS_IF([test $curl_is_ok = yes],
        [CURL_LIBS="`curl-config --libs`"
         CURL_CFLAGS="`curl-config --cflags`"
         AC_MSG_RESULT([yes; found version $version_libcurl])],
        [AC_MSG_RESULT([no, found version $version_libcurl but needed $curl_req_version])])])

AS_IF([test $curl_is_ok = no],       
    [AC_MSG_ERROR([libcurl: insufficient version or not found])])
    
])  dnl End AM_COND_IF BES_DEVELOPER for DMR++/libcurl

dnl Moved this out of the AWS-specific checks. jhrg 10/16/25

AC_MSG_CHECKING([the OS type for shared object dependencies linkage])
AS_CASE([$host],
    [*darwin*], [AM_CONDITIONAL([DARWIN], [true])
                 AC_MSG_NOTICE([OSX found.])],
    [AM_CONDITIONAL([DARWIN], [false])
     AC_MSG_NOTICE([OSX not found.])])

dnl Should we look for the AWS C++ SDK? By default, 'yes' but --without-aws will
dnl disable the check. jhrg 10/16/25

AC_ARG_WITH([aws],
    AS_HELP_STRING([--without-aws],
    [Disable support for the AWS C++ SDK. Do not look for, or use, the AWS SDK. (Used by default)]))

AM_CONDITIONAL([LOOK_FOR_AWS], [test "x$with_aws" != "xno"])
AM_COND_IF([LOOK_FOR_AWS],
    [AC_MSG_NOTICE([Looking for the AWS C++ SDK])],
    [AC_MSG_NOTICE([Not looking for the AWS C++ SDK])
     AM_CONDITIONAL([USE_AWS_SDK], [false])])

dnl Define these no matter what we do. This prevents errors in teh Makefile.am files when the
dnl libraries are not found/used. jhrg 10/16/25

aws_prefix=
aws_libs=
aws_libdir=
aws_libdir64=

AM_COND_IF([LOOK_FOR_AWS],
    [
    dnl Look for the AWS libraries (aws-cpp-sdk-s3 aws-cpp-sdk-core) and headers
    dnl jhrg 3/11/25

    dnl Look for the AWS SDK from the hyrax-dependencies. jhrg 10/9/25

    AC_LANG_PUSH([C++])
    saved_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS -I$ac_bes_dependencies_prefix/include"
    AC_CHECK_HEADERS([aws/core/Aws.h], [found_aws_h=true], [found_aws_h=false])
    AC_CHECK_HEADERS([aws/s3/S3Client.h], [found_s3client_h=true], [found_s3client_h=false])
    CPPFLAGS=$saved_CPPFLAGS
    AC_LANG_POP([C++])

    AS_IF([$found_aws_h && $found_s3client_h],
        [aws_prefix=$ac_bes_dependencies_prefix

         aws_libs="-laws-crt-cpp -laws-cpp-sdk-s3 -laws-cpp-sdk-core \
                  -laws-c-http -laws-c-io -laws-c-auth -laws-c-cal -laws-c-compression \
                  -laws-c-event-stream -laws-c-mqtt -laws-c-s3 -laws-c-sdkutils -laws-c-common \
                  -lxml2 -lz -lbz2 -lcurl"

         dnl AC_CANONICAL_HOST is needed to access the 'host_os' variable
         AC_CANONICAL_HOST

         AS_IF(
            dnl OSX test
            [echo ${host} | grep -q 'darwin.*'],
            [aws_libdir=$aws_prefix/lib; AC_MSG_NOTICE([OSX Arch Found: $host])],
            dnl else
            [
                ARCH=""
                AS_CASE([$host_cpu],
                    [x86_64|aarch64*|mips64*|ppc64*|sparc64*],
                    [ARCH="64"; AC_MSG_NOTICE([64x Arch Found: $host_cpu])],
                    [i?86|arm*|mips*|ppc*|sparc],
                    [ARCH="32"; AC_MSG_NOTICE([32x Arch Found: $host_cpu])])

                AS_IF(
                    dnl 64-bit system
                    [test "$ARCH" = "64"],
                    [aws_libdir64=$aws_prefix/lib64;aws_libdir=$aws_prefix/lib],
                    dnl 32-bit system
                    [test "$ARCH" = "32"],
                    [aws_libdir=$aws_prefix/lib]
                )
         ])

         AC_MSG_NOTICE([AWS C++ SDK Found.])
         AC_MSG_NOTICE([AWS libs 32x dir: $aws_libdir])
         AC_MSG_NOTICE([AWS libs 64x dir: $aws_libdir64])
         AM_CONDITIONAL([USE_AWS_SDK], [true])],
        [AC_MSG_NOTICE([AWS C++ SDK not Found.])
         AM_CONDITIONAL([USE_AWS_SDK], [false])])
    ])

AC_SUBST(aws_prefix)
AC_SUBST(aws_libs)
AC_SUBST(aws_libdir)
AC_SUBST(aws_libdir64)

dnl Freeform hacks

FF_CPPFLAGS=
AC_MSG_CHECKING([the OS type for the freeform code])
AS_CASE([$host],
    [*linux*], [FF_CPPFLAGS=-DLINUX],
    [*hp*], [FF_CPPFLAGS=-DHP9000],
    [*alpha*], [FF_CPPFLAGS=-DDEC_ALPHA],
    [*sun*], [FF_CPPFLAGS=-DSUN],
    [*darwin*], [FF_CPPFLAGS=-DLINUX],
    [*sgi*], [AS_IF([test $ac_cv_sizeof_long -eq 4],
                    [FF_CPPFLAGS=-DIRIS4],
                    [FF_CPPFLAGS=-DIRIX])],
    [AC_MSG_WARN([Either this machine is not supported by FreeForm or the configure script needs to be fixed])])
    
AC_MSG_RESULT([set to $FF_CPPFLAGS])
AC_SUBST(FF_CPPFLAGS)

dnl xml_data_handler hack
AC_DEFINE([ENABLE_UNIT_TESTS], [1], [Should the XML Data Handler unit-test support be compiled?])
AC_SUBST(ENABLE_UNIT_TESTS)

AC_MSG_CHECKING([the netCDF library])

dnl CHECK_NETCDF defines $NC_LIBS, $NC_LDFLAGS and $NC_CPPFLAGS
AC_CHECK_NETCDF(
   [AC_MSG_RESULT($NC_LIBS); BUILD_NETCDF=yes],
   [AC_MSG_WARN([netcdf library and/or netcdf headers not found; will not build the netcdf modules.]); BUILD_NETCDF=no],
   [3],
   $ac_bes_dependencies_prefix)

AM_CONDITIONAL([BUILD_NETCDF], [test "x$BUILD_NETCDF" = "xyes"]) 
	
AM_COND_IF([BUILD_NETCDF], [

save_LIBS=$LIBS
LIBS="$NC_LDFLAGS $NC_LIBS $LIBS"
AC_CHECK_LIB(netcdf, nc_inq_libvers, NETCDF_MAJOR_VERSION=4, NETCDF_MAJOR_VERSION=3, [])
LIBS=$save_LIBS

# save_CPPFLAGS=$CPPFLAGS
# CPPFLAGS="$NC_CPPFLAGS $CPPFLAGS"
# AC_EGREP_HEADER(NC_NETCDF4, netcdf.h, NETCDF_MAJOR_VERSION=4, NETCDF_MAJOR_VERSION=3)
# CPPFLAGS=$save_CPPFLAGS

AC_DEFINE_UNQUOTED(NETCDF_VERSION, $NETCDF_MAJOR_VERSION, [What version of netcdf were we built with?])

dnl This is a bit hokey, since the real test - for nc_lib_vers 
dnl was done above.
AC_MSG_CHECKING([the netCDF library version number])
AC_MSG_RESULT($NETCDF_MAJOR_VERSION)

dnl This is needed for the fileout_netcdf handler unit tests
NC_BIN=`echo $NC_LDFLAGS | sed 's@^-L\(.*\)/lib@\1/bin@g'`
AC_MSG_NOTICE([NC_BIN is $NC_BIN])
AC_SUBST(NC_BIN_PATH, $NC_BIN)

])	# AM_COND_IF BUILD_NETCDF

AM_CONDITIONAL([NETCDF4_TESTS], [test "x$NETCDF_MAJOR_VERSION" = "x4"])

dnl fileout_gdal tests

# Which copy of GDAL should be used for the build?
GDAL_FOUND=0

AC_ARG_WITH(gdal, AS_HELP_STRING([--with-gdal], [Use the copy of GDAL at this location]),
            with_gdal_prefix="$withval", with_gdal_prefix="")

AS_IF([test -z "$with_gdal_prefix" -a -n "$ac_bes_dependencies_prefix"],
	[with_gdal_prefix=$ac_bes_dependencies_prefix])
 
AS_IF([test -n "$with_gdal_prefix" -a $with_gdal_prefix = no],
    dnl then
    [AC_MSG_NOTICE([Not building GDAL-dependent modules.])],

    dnl else if
    [test -n "$with_gdal_prefix" -a -x $with_gdal_prefix/bin/gdal-config],
    dnl then
    [AC_MSG_NOTICE([Using $with_gdal_prefix as the GDAL prefix directory.])

    dnl This fix is for OSX with shared libs. Seems to not be needed on Linux.
    dnl jhrg 11/07/25
    gdal_shared_lib=
    gdal_shared_lib += -Wl,-rpath,$(prefix)/deps/lib -Wl,-rpath,$(prefix)/deps/proj/lib
    gdal_shared_lib += -Wl,-rpath,@loader_path

    GDAL_LDFLAGS="$gdal_shared_lib `$with_gdal_prefix/bin/gdal-config --libs` `$with_gdal_prefix/bin/gdal-config --dep-libs`"
    GDAL_CFLAGS="`$with_gdal_prefix/bin/gdal-config --cflags`"
    GDAL_FOUND=1
    AC_DEFINE([HAVE_GDAL], [1], [The GDAL Library is present])
    AC_MSG_NOTICE([Set GDAL_LDFLAGS to $GDAL_LDFLAGS.])],
    
    dnl else if
    [test -n "$with_gdal_prefix"],
    dnl then
    [AC_MSG_NOTICE([You set the gdal-prefix directory to $with_gdal_prefix,])
    AC_MSG_WARN([but gdal-config is not there. Not building GDAL-dependent modules.])],
    
    dnl else
    [AC_MSG_NOTICE([Looking for GDAL.])
    AX_LIB_GDAL([3.2.0])
    AS_IF([test ! -z "$GDAL_CFLAGS" -a ! -z "$GDAL_LDFLAGS"],
        [GDAL_FOUND=1
        AC_DEFINE([HAVE_GDAL], [1], [The GDAL Library is present])])])

AM_CONDITIONAL([BUILD_GDAL], [test $GDAL_FOUND -eq 1])

dnl Only look for the openjpeg2000 library if GDAL was found

AM_COND_IF([BUILD_GDAL], [

    dnl Which copy of OpenJPEG should be used for the build?
    OPENJPEG_FOUND=

    AC_ARG_WITH(openjpeg, AS_HELP_STRING([--with-openjpeg], [Use the copy of openjpeg at this location]),
            with_openjpeg_prefix="$withval", with_openjpeg_prefix="")

    AS_IF([test -z "$with_openjpeg_prefix" -a -n "$ac_bes_dependencies_prefix"],
	   [with_openjpeg_prefix=$ac_bes_dependencies_prefix])

    save_LIBS=$LIBS
    LIBS="-L$with_openjpeg_prefix/lib $LIBS"
    AC_CHECK_LIB(openjp2, opj_version, [OPENJPEG_FOUND="yes"], [OPENJPEG_FOUND="no"], [-lm])
    LIBS=$save_LIBS

    AM_CONDITIONAL([OPENJPEG_FOUND], [test "x$OPENJPEG_FOUND" = "xyes"])

    AS_IF([test "x$OPENJPEG_FOUND" = "xyes"], 
        [ AC_MSG_NOTICE([Found OPENJPEG; building GMLJP2 support.]) ],
        [ AC_MSG_NOTICE([I could not find OPENJPEG, not building GMLJP2 support.]) ])

],[ # else AN_CONF_IF([BUILD_GDAL]

    dnl if we are not building the gdal-related things, make sure that 
    dnl OPENJPEG_FOUND is defined and is false
    AM_CONDITIONAL([OPENJPEG_FOUND], [false])

]) # AN_CONF_IF([BUILD_GDAL] 

dnl Fits macro and test

dnl Check for the cfits library.
dnl Usage AC_CHECK_FITS action-if-found, action-if-not-found, default-path-to-check
dnl if default-path-to-check is given and the --with-cfits option(s) are not used,
dnl check the default path in addition to the current values of CPPFLAGS, etc.
dnl Defines CFITS_CPPFLAGS, CFITS_LDFLAGS, CFITS_LIBS.

AC_DEFUN([AC_CHECK_CFITS],
[

BES_DEPS_PREFIX=$3

AC_ARG_WITH([cfits],
            [AS_HELP_STRING([--with-cfits=ARG],[cfits directory])],
            [CFITS_PATH=${withval}], 
            [CFITS_PATH="$BES_DEPS_PREFIX"])

AC_ARG_WITH([cfits_inc],
            [AS_HELP_STRING([--with-cfits-inc=ARG],[cfits include directory])],
            [CFITS_PATH_INC=${withval}], 
            [CFITS_PATH_INC=""]) dnl was "/usr/include/cfitsio"

AC_ARG_WITH([cfits_libdir],
            [AS_HELP_STRING([--with-cfits-libdir=ARG],[cfits library directory])],
            [CFITS_PATH_LIBDIR=${withval}], 
            [CFITS_PATH_LIBDIR=""])  dnl was "/usr/lib"

AS_IF([test -n "$CFITS_PATH" -a -z "$CFITS_PATH_LIBDIR"],[CFITS_PATH_LIBDIR="$CFITS_PATH/lib"])
AS_IF([test -n "$CFITS_PATH" -a -z "$CFITS_PATH_INC"],[CFITS_PATH_INC="$CFITS_PATH/include"])


ac_cfits_save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -L$CFITS_PATH_LIBDIR"
AC_CHECK_LIB(cfitsio, fits_is_url_absolute, 

[ac_cfits_ok='yes'
CFITS_LIBS="-lcfitsio"
CFITS_LDFLAGS="-L${CFITS_PATH_LIBDIR}" 
AC_SUBST([CFITS_LDFLAGS])
AC_SUBST([CFITS_LIBS])],

[ac_cfits_ok='no'],
[-lm]
)

LDFLAGS="$ac_cfits_save_LDFLAGS"

CFITS_CPPFLAGS=
ac_cfits_h='no'
ac_cfits_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$CFITS_PATH_INC"
AC_CHECK_HEADERS([fitsio.h],
  [ac_cfits_h='yes'],
  [ac_cfits_h='no']
)
CPPFLAGS=$ac_cfits_save_CPPFLAGS

AS_IF([test "$ac_cfits_h" = 'yes' ], [CFITS_CPPFLAGS="-I$CFITS_PATH_INC"])

AC_SUBST([CFITS_CPPFLAGS])

AS_IF([test "$ac_cfits_ok" = 'no' -o "$ac_cfits_h" = 'no'],
  [m4_if([$2], [], [:], [$2])],
  [m4_if([$1], [], [:], [$1])]
)

])  dnl AC_DEFUN([AC_CHECK_CFITS],

AC_CHECK_CFITS(
   [AM_CONDITIONAL([BUILD_FITS],[true])],
   [AC_MSG_WARN([The cfits library and/or cfits headers not found; not building the fits handler.])
    AM_CONDITIONAL([BUILD_FITS],[false])],
   $ac_bes_dependencies_prefix
)

dnl tests for HDF4 and HDFEOS2

AC_CHECK_HDF4(
   [AM_CONDITIONAL([BUILD_HDF4],[true])
   ],
   [AC_MSG_WARN([The hdf4 library and/or hdf4 headers not found; not building the hdf4 handler.])
    AM_CONDITIONAL([BUILD_HDF4],[false])
    ],
   $ac_bes_dependencies_prefix
)
                              
dnl Test for HDF-EOS2. If found, subst hdfeos2 symbols                                                                           

AC_ARG_WITH([hdfeos2],
            [AS_HELP_STRING([--with-hdfeos2=DIR],[Specify path to external hdfeos2 library.])],
            [HDFEOS2_DIR=${withval}], 
            [HDFEOS2_DIR="$ac_bes_dependencies_prefix"])

dnl Check if the HDF-EOS2 library exists.
ac_hdfeos2_save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -I$HDFEOS2_DIR/include"
AC_CHECK_HEADERS([HDFEOSVersion.h],[],[HDFEOS2_DIR=])
CPPFLAGS="$ac_hdfeos2_save_CPPFLAGS"

ac_hdfeos2_save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS -L$HDFEOS2_DIR/lib"
AC_CHECK_LIB(hdfeos,SWopen,[HDFEOS2_LDFLAGS=],[HDFEOS2_DIR=],[-lmfhdf -ldf -lz -ljpeg -lm])
LDFLAGS="$ac_hdfeos2_save_LDFLAGS"

AS_IF([test -n "$HDFEOS2_DIR"],
    [HDFEOS2_CPPFLAGS="-I$HDFEOS2_DIR/include -DUSE_HDFEOS2_LIB"
    HDFEOS2_LDFLAGS="-L$HDFEOS2_DIR/lib"
    HDFEOS2_LIBS="-lhdfeos -lGctp"
    AC_MSG_NOTICE([HDFEOS2 set to true])
    AM_CONDITIONAL([HDFEOS2], [true])],

    [AC_MSG_NOTICE([HDFEOS2 set to false])
    AM_CONDITIONAL([HDFEOS2], [false])]
 )

    
AC_SUBST([HDFEOS2_CPPFLAGS])
AC_SUBST([HDFEOS2_LDFLAGS])
AC_SUBST([HDFEOS2_LIBS])
AC_SUBST([HDFEOS2_DIR])

dnl HDF5 tests; We pass null for the third argument - the interface number
AC_CHECK_HDF5(
  [AM_CONDITIONAL([BUILD_HDF5],[true])
  ],
  [AC_MSG_WARN([The hdf5 library and/or hdf5 headers not found; not building the hdf5 handler.])
   AM_CONDITIONAL([BUILD_HDF5],[false])
  ],
  [],
  $ac_bes_dependencies_prefix
)

AC_CHECK_STARE(
  [AM_CONDITIONAL([BUILD_STARE],[true])
  ],
  [AC_MSG_WARN([STARE library not found.])
  AM_CONDITIONAL([BUILD_STARE],[false])
  ],
  $ac_bes_dependencies_prefix
)

dnl NCML module test
dnl
dnl Look for the minimum version of the icu libs and headers
dnl This sets: ICU_CPPFLAGS with -Iicu-include-path and ICU_LIBS with 
dnl -Licu-lib-path -licu-lib-1 ... 
dnl Note: it also handles --with-icu-prefix=path_to_icu_install
dnl in case there are multiple icu development environments on the machine
dnl NOTE: LD_LIBRARY_PATH or DYLD_LIBRARY_PATH may need to be set for this
dnl to work, however.
AC_CHECK_ICU([3.6],
	[AM_CONDITIONAL([BUILD_NCML],[true])
	AC_MSG_NOTICE([ICU_CPPFLAGS=$ICU_CPPFLAGS])
	AC_MSG_NOTICE([ICU_LIBS=$ICU_LIBS])
	],
	[AC_MSG_WARN([The icu library and/or icu headers not found; not building the NCML handler.])
 	AM_CONDITIONAL([BUILD_NCML],[false])
	],
	$ac_bes_dependencies_prefix
)

dnl Look for gridfields

dnl When pkgconfig support is added, use this
libdap_libgridfields_module="libgridfields >= 1.0.3"
libgf_needed="1.0.3"

libgf_ok="no"

dnl NB: Hackery: if --with-gridfields is not given and ac_bes_dependencies_prefix 
dnl is non-zero, use that as if --with-... was given. jhrg 12/1/14
AC_ARG_WITH([gridfields],
    [AS_HELP_STRING([--with-gridfields=path],[Use the gridfields library at this location.])],
	[AS_IF([test x"$withval" = x"yes"],
		[AC_MSG_ERROR([You must supply a path when using --with-gridfields; try --enable-gridfields.])],
		[AC_MSG_NOTICE([Using $withval as the GridFields prefix directory.])
         GF_LIBS="-L$withval/lib -lgridfields"
         GF_CFLAGS="-I$withval/include"
 	 GF_PATH=$withval
         AC_DEFINE([GRIDFIELDS],[1],[define if gridfields lib is present])
         libgf_ok="yes"])],
    [ libgf_ok="no" ])

AC_ARG_ENABLE([gridfields],
    AS_HELP_STRING([--enable-gridfields],[Enable gridfields (default is YES)]),
    [ ],
    [enable_gridfields=yes])
 
AS_IF([test "x$libgf_ok" = "xno" -a "x$enable_gridfields" = "xyes"],
    [

    dnl save PATH     
    gridfields_save_path=$PATH

    dnl if --with-dependencies was used, look there too
    AS_IF([test -n "$ac_bes_dependencies_prefix"], [PATH=$ac_bes_dependencies_prefix/bin:$PATH])

    AC_MSG_CHECKING([for libgridfields $libgf_needed])

    dnl Use AS_VERSION_COMPARE to test version numbers. jhrg 9/9/15

    AS_IF([gridfields-config --version > /dev/null 2>&1],
        [
        version_libgf=`gridfields-config --version | sed 's@gridfields \(.*\)@\1@'`
        AS_VERSION_COMPARE($version_libgf, $libgf_needed, [libgf_ok=no], [libgf_ok=yes], [libgf_ok=yes])
        AS_IF([test x$libgf_ok = xno],
            [
            AC_MSG_ERROR([Must have libgf $libgf_needed or greater, found $version_libgf])
            ])

        GF_LIBS="`gridfields-config --libs`"
        GF_CFLAGS="`gridfields-config --cflags`"
        GF_PATH="`gridfields-config --prefix`"

        AC_DEFINE([GRIDFIELDS],[1],[define if gridfields lib is present])
        AC_MSG_RESULT([yes; used gridfields-config])
        ],
        [test -f $ac_bes_dependencies_prefix/lib/libgridfields.a -a -d $ac_bes_dependencies_prefix/include/gridfields/],
        [
        libgf_ok="yes"
        GF_LIBS="-L$ac_bes_dependencies_prefix/lib -lgridfields"
        GF_CFLAGS="-I$ac_bes_dependencies_prefix/include"
        GF_PATH=$ac_bes_dependencies_prefix
        AC_DEFINE([GRIDFIELDS],[1],[define if gridfields lib is present])
        AC_MSG_RESULT([yes; used --with-dependencies value ($ac_bes_dependencies_prefix)])
        ],
        [
        AC_MSG_NOTICE([I could not find libgridfields])
        ]
    )
    
    dnl Restore saved value of PATH
    export PATH=$gridfields_save_path
    ]
)

AC_SUBST([GF_LIBS])
AC_SUBST([GF_CFLAGS])
AC_SUBST([GF_PATH])

if test "$libgf_ok" = "yes"
then
    echo "Set gridfields CFLAGS to $GF_CFLAGS"
    echo "Set gridfields LIBS to $GF_LIBS"
    AM_CONDITIONAL([USING_GRIDFIELDS],[true])
else
    AM_CONDITIONAL([USING_GRIDFIELDS],[false])
fi

dnl End handler specific tests

],[ # else AM_COND_IF([DAP_BUILTIN_MODULES]

dnl These are needed because the symbols are used in each modules' Makefile.am
dnl files. It might be that if the generation of those Makefiles was also conditional,
dnl then we could remove this block of code. jhrg 11/30/14
AM_CONDITIONAL([OPENJPEG_FOUND], [false])
AM_CONDITIONAL([BUILD_GDAL], [false])
AM_CONDITIONAL([NETCDF4_TESTS], [false])
AM_CONDITIONAL([BUILD_NETCDF], [false])
AM_CONDITIONAL([BUILD_FITS], [false])
AM_CONDITIONAL([BUILD_HDF4], [false])
AM_CONDITIONAL([HDFEOS2], [false])
AM_CONDITIONAL([BUILD_HDF5], [false])
AM_CONDITIONAL([BUILD_NCML], [false])
AM_CONDITIONAL([USING_GRIDFIELDS], [false])
AM_CONDITIONAL([BUILD_STARE], [false])

AM_CONDITIONAL([WITH_DEPENDENCIES], [false])

])	# AM_COND_IF([DAP_BUILTIN_MODULES]

OPTIONAL_FEATURES=

AM_COND_IF([OPENJPEG_FOUND], [OPTIONAL_FEATURES="openjpeg"])
AM_COND_IF([BUILD_GDAL], [OPTIONAL_FEATURES="gdal $OPTIONAL_FEATURES"])
AM_COND_IF([USE_AWS_SDK], [OPTIONAL_FEATURES="AWS_C++_SDK $OPTIONAL_FEATURES"])
dnl NETCDF4_TESTS are not considered an 'optional feature' since netcdf is netCDF4
AM_COND_IF([BUILD_NETCDF], [OPTIONAL_FEATURES="netcdf $OPTIONAL_FEATURES"])
AM_COND_IF([BUILD_FITS], [OPTIONAL_FEATURES="fits $OPTIONAL_FEATURES"])
AM_COND_IF([BUILD_HDF4], [OPTIONAL_FEATURES="hdf4 $OPTIONAL_FEATURES"])
AM_COND_IF([HDFEOS2], [OPTIONAL_FEATURES="hdfeos2 $OPTIONAL_FEATURES"])
AM_COND_IF([BUILD_HDF5], [OPTIONAL_FEATURES="hdf5 $OPTIONAL_FEATURES"])
AM_COND_IF([BUILD_NCML], [OPTIONAL_FEATURES="ncml $OPTIONAL_FEATURES"])
AM_COND_IF([USING_GRIDFIELDS], [OPTIONAL_FEATURES="gridfields $OPTIONAL_FEATURES"])
AM_COND_IF([BUILD_STARE], [OPTIONAL_FEATURES="stare $OPTIONAL_FEATURES"])
AM_COND_IF([BUILD_S3], [OPTIONAL_FEATURES="s3_reader $OPTIONAL_FEATURES"])

AM_COND_IF([BES_DEVELOPER], [OPTIONAL_FEATURES="developer $OPTIONAL_FEATURES"])

AC_SUBST([OPTIONAL_FEATURES])

dnl autoheader macros; tack some text at the top and bottom of config.h.in
AH_TOP([#ifndef _config_h
#define _config_h])

AH_BOTTOM([

// These values are defined in configure.ac
#define ANNOTATION_SYSTEM 1

#if defined(__GNUG__) || defined(__GNUC__)
#define not_used __attribute__ ((unused))
#else
#define not_used 
#endif /* __GNUG__ || __GNUC__ */

#endif /* _config_h */])

AC_CONFIG_FILES([Makefile
		 bes_VERSION
		 bes.spec
		 bes.spec.all_static
		 abi_checker.xml
		 bes_dispatch.pc
		 bes_ppt.pc
		 bes_xml_command.pc
		 doxy.conf
		 main_page.doxygen
		 
		 dispatch/Makefile
		 dispatch/unit-tests/Makefile
		 dispatch/tests/Makefile
		 dispatch/tests/atlocal
		 
		 hello_world/Makefile
		 hello_world/tests/Makefile
		 hello_world/tests/atlocal

		 aws/Makefile
		 aws/unit-tests/Makefile

		 http/Makefile
		 http/unit-tests/Makefile
		 http/tests/Makefile
		 http/tests/atlocal

		 docs/Makefile
		 
		 xmlcommand/Makefile
		 xmlcommand/unit-tests/Makefile
		 xmlcommand/tests/Makefile
		 xmlcommand/tests/atlocal
		 
		 ppt/Makefile
		 ppt/unit-tests/Makefile
		 
		 cmdln/Makefile
		 cmdln/unit-tests/Makefile
		 cmdln/tests/Makefile
		 cmdln/tests/atlocal
		 
		 standalone/Makefile
		 
		 server/Makefile

		 bin/Makefile
		 
		 templates/Makefile
		 
		 dap/Makefile
		 dap/unit-tests/Makefile
		 dap/tests/Makefile
		 dap/tests/atlocal

		 dapreader/Makefile
		 dapreader/tests/Makefile
		 dapreader/tests/atlocal
		 ])

dnl These need to be moved into the AC_CONFIG_FILES above 
dnl when geo-functions is re-enabled.
dnl		 functions/geo-functions/Makefile
dnl		 functions/geo-functions/unit-tests/Makefile

AC_CONFIG_FILES([bes-config], [chmod +x bes-config]) 
AC_CONFIG_FILES([besd], [chmod +x besd]) 
AC_CONFIG_FILES([server/besctl], [chmod +x server/besctl])

dnl A better way to control starting a stopping hyrax is needed.
dnl For now, use besd (which can be run from startup scripts or
dnl using besctl) and tomcat. both bes and tomcat can be set to
dnl start at boot time. jhrg 9/27/17 
AC_CONFIG_FILES([server/hyraxctl], [chmod +x server/hyraxctl])

AC_CONFIG_TESTDIR(cmdln/tests, [standalone])
AC_CONFIG_TESTDIR(hello_world/tests, [standalone])
AC_CONFIG_TESTDIR(http/tests, [remote_resource_tester])
AC_CONFIG_TESTDIR(dapreader/tests, [standalone])
AC_CONFIG_TESTDIR(dap/tests, [standalone])
AC_CONFIG_TESTDIR(dispatch/tests, [dispatch/tests])
AC_CONFIG_TESTDIR(xmlcommand/tests, [standalone])

dnl Output files if building the modules too

dnl Removed these for HYRAX-1921. jhrg 10/29/25
dnl modules/cmr_module/tests/Makefile
dnl modules/cmr_module/tests/atlocal
dnl modules/cmr_module/unit-tests/Makefile

AM_COND_IF([DAP_BUILTIN_MODULES],
    [
    AC_CONFIG_FILES([modules/Makefile
    modules/configured_features.txt

    modules/common/Makefile

    modules/csv_handler/Makefile
    modules/csv_handler/tests/Makefile
    modules/csv_handler/tests/atlocal

    modules/usage/Makefile
    
    modules/asciival/Makefile
    modules/asciival/tests/Makefile
    modules/asciival/tests/atlocal
    modules/asciival/unit-tests/Makefile

    modules/cmr_module/Makefile

    modules/s3_reader/Makefile
    modules/s3_reader/tests/Makefile
    modules/s3_reader/tests/atlocal

    modules/httpd_catalog_module/Makefile
    modules/httpd_catalog_module/unit-tests/Makefile
    modules/httpd_catalog_module/tests/Makefile
    modules/httpd_catalog_module/tests/atlocal

    modules/debug_functions/Makefile
    modules/debug_functions/unit-tests/Makefile 
    modules/debug_functions/tests/Makefile 
    modules/debug_functions/tests/atlocal

    modules/dmrpp_module/Makefile
    modules/dmrpp_module/unit-tests/Makefile
    modules/dmrpp_module/tests/Makefile
    modules/dmrpp_module/tests/atlocal
    modules/dmrpp_module/tests_build_dmrpp/Makefile
    modules/dmrpp_module/tests_build_dmrpp/atlocal
    modules/dmrpp_module/data/Makefile
    modules/dmrpp_module/dmrpp_transmitter/Makefile

    modules/dmrpp_module/ngap_container/Makefile
    modules/dmrpp_module/ngap_container/unit-tests/Makefile

    modules/dmrpp_module/build_dmrpp_h4/Makefile
    modules/dmrpp_module/get_dmrpp/Makefile
    modules/dmrpp_module/get_dmrpp/unit-tests/Makefile

    modules/fileout_json/Makefile
    modules/fileout_json/unit-tests/Makefile 
    modules/fileout_json/tests/Makefile 
    modules/fileout_json/tests/atlocal

    modules/fileout_covjson/Makefile
    modules/fileout_covjson/unit-tests/Makefile 
    modules/fileout_covjson/tests/Makefile 
    modules/fileout_covjson/tests/atlocal
    
    modules/freeform_handler/Makefile
    modules/freeform_handler/tests/Makefile
    modules/freeform_handler/tests/atlocal
    modules/freeform_handler/FFND/Makefile

    modules/functions/Makefile
    modules/functions/tests/Makefile
    modules/functions/tests/atlocal
    modules/functions/unit-tests/Makefile
    modules/functions/stare/Makefile
    modules/functions/stare/unit-tests/Makefile
    modules/functions/stare/tests/Makefile
    modules/functions/stare/tests/atlocal

    modules/xml_data_handler/Makefile
    modules/xml_data_handler/unit-tests/Makefile
    modules/xml_data_handler/tests/Makefile
    modules/xml_data_handler/tests/atlocal
    
    modules/netcdf_handler/Makefile
    modules/netcdf_handler/tests/Makefile 
    modules/netcdf_handler/tests/atlocal 
	
    modules/fileout_netcdf/Makefile 	
    modules/fileout_netcdf/tests/Makefile 
    modules/fileout_netcdf/unit-tests/Makefile
    modules/fileout_netcdf/tests/atlocal
    modules/fileout_netcdf/data/build_test_data/Makefile 
	
    modules/gateway_module/Makefile 
    modules/gateway_module/tests/Makefile 
    modules/gateway_module/tests/atlocal 

    modules/gdal_module/Makefile
    modules/gdal_module/reader/Makefile
    modules/gdal_module/writer/Makefile
    modules/gdal_module/tests/Makefile
    modules/gdal_module/tests/atlocal

    modules/fits_handler/Makefile
    modules/fits_handler/tests/atlocal
    modules/fits_handler/tests/Makefile
    
    modules/hdf4_handler/Makefile
    modules/hdf4_handler/hdfclass/Makefile
    modules/hdf4_handler/bes-testsuite/Makefile
    modules/hdf4_handler/bes-testsuite/atlocal

    modules/hdf5_handler/Makefile
    modules/hdf5_handler/bes-testsuite/Makefile
    modules/hdf5_handler/bes-testsuite/atlocal
    modules/hdf5_handler/gctp/Makefile
    modules/hdf5_handler/gctp/src/Makefile
    
    modules/ncml_module/Makefile 
    modules/ncml_module/tests/Makefile 
    modules/ncml_module/tests/atlocal 

    modules/ugrid_functions/Makefile 
    modules/ugrid_functions/unit-tests/Makefile
    modules/ugrid_functions/tests/Makefile 
    modules/ugrid_functions/tests/atlocal
        
    modules/w10n_handler/Makefile 
    modules/w10n_handler/unit-tests/Makefile
    modules/w10n_handler/tests/Makefile 
    modules/w10n_handler/tests/atlocal])

    dnl Move these into the autotest.at files using an autotest option.
    AC_CONFIG_FILES([modules/xml_data_handler/tests/generate_metadata_baseline.sh],
	[chmod +x modules/xml_data_handler/tests/generate_metadata_baseline.sh])
		
    AC_CONFIG_FILES([modules/xml_data_handler/tests/generate_data_baseline.sh],
	[chmod +x modules/xml_data_handler/tests/generate_data_baseline.sh])

    AC_CONFIG_FILES([modules/fits_handler/tests/generate_metadata_baseline.sh],
	[chmod +x modules/fits_handler/tests/generate_metadata_baseline.sh])
    AC_CONFIG_FILES([modules/fits_handler/tests/generate_data_baseline.sh],
        [chmod +x modules/fits_handler/tests/generate_data_baseline.sh])
	
    AC_CONFIG_FILES([modules/hdf4_handler/bes-testsuite/generate_metadata_baseline.sh], 
        [chmod +x modules/hdf4_handler/bes-testsuite/generate_metadata_baseline.sh])

	dnl These can stay
    dnl AC_CONFIG_FILES([modules/www-interface/js2h.pl], [chmod +x modules/www-interface/js2h.pl])

    AC_CONFIG_TESTDIR(modules/csv_handler/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/asciival/tests, [standalone])
        
    AC_CONFIG_TESTDIR(modules/httpd_catalog_module/tests, [standalone])

    dnl AC_CONFIG_TESTDIR(modules/cmr_module/tests, [standalone]) HYRAX-1921 jhrg 10/29/25
    AC_CONFIG_TESTDIR(modules/s3_reader/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/fileout_json/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/fileout_covjson/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/debug_functions/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/dmrpp_module/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/dmrpp_module/tests_build_dmrpp, [modules/dmrpp_module])
    AC_CONFIG_TESTDIR(modules/freeform_handler/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/xml_data_handler/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/netcdf_handler/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/fileout_netcdf/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/gateway_module/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/fits_handler/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/gdal_module/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/ncml_module/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/ugrid_functions/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/w10n_handler/tests, [standalone])

    AC_CONFIG_TESTDIR(modules/functions/tests, [standalone])
    AC_CONFIG_TESTDIR(modules/functions/stare/tests, [standalone])

    AC_CONFIG_TESTDIR(modules/hdf4_handler/bes-testsuite, [standalone])
    AC_CONFIG_TESTDIR(modules/hdf5_handler/bes-testsuite, [standalone])
    ]
)

AC_OUTPUT

AC_MSG_NOTICE([***])
AC_MSG_NOTICE([Optional features: $OPTIONAL_FEATURES])
AC_MSG_NOTICE([***])
