# -*- mode: autoconf -*-
# Usage: AC_CHECK_ICU(minversion,action-if-found,action-if-not-found)
# Configure macros for including ICU external lib and headers
#
# Created: 1 April 2010 by Michael Johnson <m.johnson@opendap.org> 
# by using the AX_PATH_GENERIC macro (see ax_path_generic.m4)
#
# Sets ICU_CPPFLAGS and ICU_LIBS to contain the include search path 
# and library search path and linked libraries for icu, respecively.
#
# Also checks for the minimum version.
# This assumes the existence of the icu-config program.
#
# Note that --with-icu-exec-prefix=some_prefix_to_icu_config_bin
# can be used to specify which icu in the case of multiple ones.
# Equivalently, --with-icu-prefix=/path/to/icu/prefix can be used, where
# the icu-config is checked for in prefix/bin 
#
AC_DEFUN([AC_CHECK_ICU],
[
 icu_min_version=m4_if([$1], [], [3.6.0], [$1])
 AC_MSG_CHECKING([for icu version >= $icu_min_version])



# ------------------- Start mod ----------------------------
# I added this as a hack to get the automated packge build working. 
# It should be replaced with a more reliable method for identifying 
# the ICU install dir.
# 12/2/2010 ndp

  AC_ARG_WITH([icu-prefix],
            [],
            [ICU_PATH=$withval], 
            [ICU_PATH=""])

    AC_MSG_NOTICE([ICU_PATH: ] ICU_PATH)
    AC_SUBST(ICU_PATH)
# ------------------- End mod ------------------------------

dnl usages of path call:
dnl AX_PATH_GENERIC(LIBRARY,[MINIMUM-VERSION,[SED-EXPR-EXTRACTOR]],[ACTION-IF-FOUND],[ACTION-IF-NOT-FOUND],[CONFIG-SCRIPTS],[CFLAGS-ARG],[LIBS-ARG])
dnl We need to set CFLAGS-ARG and LIBS-ARG since icu-config doesn't use the expected arguments for what we need
AX_PATH_GENERIC([icu],[$icu_min_version],,[$2],[$3],,[--cppflags],[--ldflags])

dnl set CPPFLAGS since that's what we really wanted here (include path), not CFLAGS
dnl ICU_LIBS should have -L and -l entries.
ICU_CPPFLAGS=$ICU_CFLAGS
echo "icu.m4   ICU_LIBS:     " $ICU_LIBS
echo "icu.m4   ICU_CPPFLAGS: " $ICU_CPPFLAGS
AC_SUBST(ICU_CPPFLAGS)
])
