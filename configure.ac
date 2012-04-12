
dnl -*- automake -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

dnl Update version here and below at LIB_CURRENT, ..., if needed.
AC_INIT(bes, 3.10.2, support@opendap.org)
AC_CONFIG_SRCDIR([configure.ac])
AM_CONFIG_HEADER([config.h])

PACKAGE_MAJOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\1@'`
PACKAGE_MINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\2@'`
PACKAGE_SUBMINOR_VERSION=`echo $PACKAGE_VERSION | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\3@'`
AC_MSG_NOTICE(Package Major Version:     $PACKAGE_MAJOR_VERSION)
AC_MSG_NOTICE(Package Minor Version:     $PACKAGE_MINOR_VERSION)
AC_MSG_NOTICE(Package SubMinor Version:  $PACKAGE_SUBMINOR_VERSION)
AC_SUBST(PACKAGE_MAJOR_VERSION)
AC_SUBST(PACKAGE_MINOR_VERSION)
AC_SUBST(PACKAGE_SUBMINOR_VERSION)

AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([conf])

AM_INIT_AUTOMAKE(1.10)
AC_CONFIG_TESTDIR(cmdln/testsuite, [.])
AC_CONFIG_TESTDIR(dapreader/bes-testsuite, [.])

dnl flags for the compilers and linkers - set these before locating the
dnl actual tools since some of the AC_PROG macros set these `flag variables'
dnl to default values otherwise.

AC_CANONICAL_HOST
AC_SUBST(host)

dnl library visioning: Update these when the interface changes. Generally,
dnl assume that the interface tracks the major and minor release numbers.
LIB_DIS_CURRENT=9
LIB_DIS_AGE=1
LIB_DIS_REVISION=4
AC_SUBST(LIB_DIS_CURRENT)
AC_SUBST(LIB_DIS_AGE)
AC_SUBST(LIB_DIS_REVISION)
LIBDISPATCH_VERSION="$LIB_DIS_CURRENT:$LIB_DIS_REVISION:$LIB_DIS_AGE"
AC_SUBST(LIBDISPATCH_VERSION)

LIB_BESDAP_CURRENT=4
LIB_BESDAP_AGE=1
LIB_BESDAP_REVISION=5
AC_SUBST(LIB_BESDAP_CURRENT)
AC_SUBST(LIB_BESDAP_AGE)
AC_SUBST(LIB_BESDAP_REVISION)
LIBBES_DAP_VERSION="$LIB_BESDAP_CURRENT:$LIB_BESDAP_REVISION:$LIB_BESDAP_AGE"
AC_SUBST(LIBBES_DAP_VERSION)

LIB_PPT_CURRENT=4
LIB_PPT_AGE=0
LIB_PPT_REVISION=4
AC_SUBST(LIB_PPT_CURRENT)
AC_SUBST(LIB_PPT_AGE)
AC_SUBST(LIB_PPT_REVISION)
LIBPPT_VERSION="$LIB_PPT_CURRENT:$LIB_PPT_REVISION:$LIB_PPT_AGE"
AC_SUBST(LIBPPT_VERSION)

LIB_XML_CMD_CURRENT=2
LIB_XML_CMD_AGE=1
LIB_XML_CMD_REVISION=3
AC_SUBST(LIB_XML_CMD_CURRENT)
AC_SUBST(LIB_XML_CMD_AGE)
AC_SUBST(LIB_XML_CMD_REVISION)
LIBXMLCOMMAND_VERSION="$LIB_XML_CMD_CURRENT:$LIB_XML_CMD_REVISION:$LIB_XML_CMD_AGE"
AC_SUBST(LIBXMLCOMMAND_VERSION)

AC_DEFINE([DAPREADER_PACKAGE], ["dapreader_module"], [dapreader_module])
AC_DEFINE([DAPREADER_VERSION], ["0.0.1"], [0.0.1])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_INSTALL

dnl Use libtool instead?
dnl AC_PROG_RANLIB
AC_PROG_LIBTOOL

AC_CHECK_PROG(VALGRIND, valgrind, [valgrind --logfile=memcheck])

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h limits.h unistd.h pthread.h bzlib.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(strdup strftime timegm mktime)
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
	version_M=`echo $version_libxml2 | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\1@'`
	version_m=`echo $version_libxml2 | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\2@'`
	version_m_m=`echo $version_libxml2 | sed 's@^\([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$@\3@'`
	
	dnl Test for several different versions of libxml2. We can use 2.6.16 
	dnl or newer, but part of the SAX interface changes depending on the
	dnl version.
	
	dnl First test the minimum
	if test $version_M -lt 2
	then
            AC_MSG_RESULT([no])
	    AC_MSG_ERROR([must have libxml2 $xml_required_version or greater, found $version_libxml2])
	else
	    if test $version_M -eq 2
	    then
		if test $version_m -lt 6
		then
                    AC_MSG_RESULT([no])
		    AC_MSG_ERROR([must have libxml2 $xml_required_version or greater, found $version_libxml2])
		else
		    if test $version_m -eq 6
		    then
			if test $version_m_m -lt 16
			then
                            AC_MSG_RESULT([no])
			    AC_MSG_ERROR([must have libxml2 $xml_required_version or greater, found $version_libxml2])
			fi
		    fi
		fi
	    fi
	fi
	
	XML2_LIBS="`xml2-config --libs`"
	XML2_STATIC_LIBS=$XML2_LIBS
	XML2_CFLAGS="`xml2-config --cflags`"
	xmlprivatelibs="`xml2-config --libs`"
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

AM_PATH_CPPUNIT(1.12.0,
	[AM_CONDITIONAL([CPPUNIT], [true])],
	[AM_CONDITIONAL([CPPUNIT], [false])])

AC_CONFIG_FILES([Makefile
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
		 dapreader/Makefile
		 dapreader/bes-testsuite/Makefile
		 dapreader/bes-testsuite/atlocal
		 bes_dispatch.pc
		 bes_ppt.pc
		 bes_dap.pc
		 bes_xml_command.pc
])

AC_CONFIG_FILES([bes-config], [chmod +x bes-config]) 
AC_CONFIG_FILES([besd], [chmod +x besd]) 
AC_CONFIG_FILES([server/besctl], [chmod +x server/besctl]) 
AC_CONFIG_FILES([server/hyraxctl], [chmod +x server/hyraxctl]) 
		 
AC_OUTPUT

