# -*- mode: autoconf -*-
# Usage: AC_CHECK_ICU(minversion,action-if-found,action-if-not-found,bes-deps-prefix)
# Configure macros for including ICU external lib and headers
#
# Created: 1 April 2010 by Michael Johnson <m.johnson@opendap.org> 
#
# Sets ICU_CPPFLAGS and ICU_LIBS to contain the include search path 
# and library search path and linked libraries for icu, respectively.
#
# Also checks for the minimum version.
# This uses pkg-config when available and falls back to the icu-config program.
#
# Note that --with-icu-prefix=/path/to/icu/prefix can be used, where
# the icu-config is checked for in prefix/bin 
#
# Lastly, if bes-deps-prefix (the fourth argument is given), then 
# that value is used as if it was passed to --with-icu-prefix. This
# is a hack added for the bes combined build.
#
# NB: as of 5/19/26 we have dropped ICU from the hyrax-dependencies builds.
# jhrg 5/19/26
#
AC_DEFUN([AC_CHECK_ICU],
[
 AC_REQUIRE([AC_PROG_CXX])
 AC_REQUIRE([AC_PROG_SED])
 AC_REQUIRE([PKG_PROG_PKG_CONFIG])

 icu_min_version=m4_if([$1], [], [3.6.0], [$1])
 AC_MSG_NOTICE([checking for ICU version >= $icu_min_version])

 BES_DEPS_PREFIX=$4
 ICU_PATH=
 icu_found=no

 AC_ARG_WITH([icu-prefix],
    [AS_HELP_STRING([--with-icu-prefix=PREFIX], [Prefix where ICU is installed])],
    [AS_IF([test "x$withval" = xyes -o "x$withval" = xno],
        [AC_MSG_ERROR([--with-icu-prefix requires an ICU installation prefix])],
        [ICU_PATH=$withval])],
    [ICU_PATH=$BES_DEPS_PREFIX])
 AC_SUBST([ICU_PATH])

 save_PATH=$PATH
 save_PKG_CONFIG_PATH=$PKG_CONFIG_PATH

 AS_IF([test -n "$ICU_PATH"],
    [AS_IF([test -d "$ICU_PATH/bin"],
        [PATH=$ICU_PATH/bin:$PATH])
     AS_IF([test -d "$ICU_PATH/lib/pkgconfig"],
        [PKG_CONFIG_PATH=$ICU_PATH/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}])
     AS_IF([test -d "$ICU_PATH/lib64/pkgconfig"],
        [PKG_CONFIG_PATH=$ICU_PATH/lib64/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}])
     export PKG_CONFIG_PATH
     AC_MSG_NOTICE([checking ICU under prefix $ICU_PATH])])

 PKG_CHECK_MODULES([ICU], [icu-i18n >= $icu_min_version icu-uc >= $icu_min_version],
    [icu_found=yes
     ICU_CPPFLAGS=$ICU_CFLAGS
     ICU_CXXFLAGS=`$PKG_CONFIG --variable=CXXFLAGS icu-i18n 2>/dev/null`
     AC_MSG_NOTICE([ICU found using pkg-config])],
    [icu_found=no])

 AS_IF([test "x$icu_found" != xyes],
    [AC_PATH_PROGS([ICU_CONFIG], [icu-config], [no])
     AS_IF([test "x$ICU_CONFIG" != xno],
        [icu_config_version=`$ICU_CONFIG --version | $SED -e 's/^\ *\(.*\)\ *$/\1/'`
         AC_MSG_CHECKING([for icu-config version $icu_config_version >= $icu_min_version])
         AS_VERSION_COMPARE(["$icu_config_version"], ["$icu_min_version"],
            [AC_MSG_RESULT([no])],
            [icu_found=yes
             AC_MSG_RESULT([yes])],
            [icu_found=yes
             AC_MSG_RESULT([yes])])
         AS_IF([test "x$icu_found" = xyes],
            [ICU_CFLAGS=`$ICU_CONFIG --cppflags`
             ICU_CPPFLAGS=$ICU_CFLAGS
             ICU_CXXFLAGS=`$ICU_CONFIG --cxxflags 2>/dev/null`
             ICU_LIBS=`$ICU_CONFIG --ldflags`
             AC_MSG_NOTICE([ICU found using $ICU_CONFIG])])])])

 AS_IF([test "x$icu_found" = xyes],
    [bes_save_CPPFLAGS=$CPPFLAGS
     bes_save_CXXFLAGS=$CXXFLAGS
     bes_save_LIBS=$LIBS
     CPPFLAGS="$CPPFLAGS $ICU_CPPFLAGS"
     CXXFLAGS="$CXXFLAGS $ICU_CXXFLAGS"
     LIBS="$ICU_LIBS $LIBS"
     AC_LANG_PUSH([C++])
     AC_MSG_CHECKING([whether ICU headers and libraries are usable])
     AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <unicode/smpdtfmt.h>
#include <unicode/timezone.h>
#include <unicode/utypes.h>]],
            [[UErrorCode status = U_ZERO_ERROR;
icu::SimpleDateFormat formatter(status);
formatter.setTimeZone(*icu::TimeZone::getGMT());
return U_FAILURE(status);]])],
        [AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])
         icu_found=no])
     AC_LANG_POP
     CPPFLAGS=$bes_save_CPPFLAGS
     CXXFLAGS=$bes_save_CXXFLAGS
     LIBS=$bes_save_LIBS])

 PATH=$save_PATH
 PKG_CONFIG_PATH=$save_PKG_CONFIG_PATH
 export PKG_CONFIG_PATH

 AS_IF([test "x$icu_found" = xyes],
    [AC_SUBST([ICU_CFLAGS])
     AC_SUBST([ICU_CPPFLAGS])
     AC_SUBST([ICU_CXXFLAGS])
     AC_SUBST([ICU_LIBS])
     $2],
    [$3])
])
