# -*- mode: autoconf -*-
# Configure macros for PPTCAPI
#
# Based on bes.m4 by Patrice Dumas, et al.

# AC_CHECK_PPTCAPI([MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
# Test for PPTCAPI and define PPTCAPI_CFLAGS and PPTCAPI_LIBS.
# Check that the version is above MINIMUM-VERSION 
# use when linking with a c++ aware linker, with a c linker you may also
# need -lstdc++

AC_DEFUN([AC_CHECK_PPTCAPI],
[
  pptcapi_min_version=m4_if([$1], [], [3.1.0], [$1])
  pptcapi_no=
  pptcapi_pkgconfig=yes
  PKG_CHECK_MODULES([BES_PPTCAPI],[bes_pptcapi >= $pptcapi_min_version],,
     [pptcapi_pkgconfig=no])


  if test $pptcapi_pkgconfig = yes; then
    PPTCAPI_LIBS="$PPTCAPI_LIBS"
    PPTCAPI_CPPFLAGS=$PPTCAPI_CFLAGS
  else
    AC_PATH_PROG([PPTCAPI_CONFIG], [pptcapi-config], [no])
    AC_MSG_CHECKING([for pptcapi version >= $pptcapi_min_version])
    if test "$PPTCAPI_CONFIG" = 'no' ; then
      pptcapi_no=yes
    else
      pptcapi_config_major_version=`$PPTCAPI_CONFIG --version | sed 's/^pptcapi \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\1/'`
      pptcapi_config_minor_version=`$PPTCAPI_CONFIG --version | sed 's/^pptcapi \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\2/'`
      pptcapi_config_micro_version=`$PPTCAPI_CONFIG --version | sed 's/^pptcapi \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\3/'`
      pptcapi_min_major_version=`echo $pptcapi_min_version | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
      pptcapi_min_minor_version=`echo $pptcapi_min_version | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
      pptcapi_min_micro_version=`echo $pptcapi_min_version | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

      pptcapi_config_is_lt=""
      if test $pptcapi_config_major_version -lt $pptcapi_min_major_version ; then
        pptcapi_config_is_lt=yes
      else
        if test $pptcapi_config_major_version -eq $pptcapi_min_major_version ; then
          if test $pptcapi_config_minor_version -lt $pptcapi_min_minor_version ; then
            pptcapi_config_is_lt=yes
          else
            if test $pptcapi_config_minor_version -eq $pptcapi_min_minor_version ; then
              if test $pptcapi_config_micro_version -lt $pptcapi_min_micro_version ; then
                pptcapi_config_is_lt=yes
              fi
            fi
          fi
        fi
      fi
      if test x$pptcapi_config_is_lt = xyes ; then
        pptcapi_no=yes
      else
        PPTCAPI_LIBS="`$PPTCAPI_CONFIG --libs`"
        PPTCAPI_CPPFLAGS="`$PPTCAPI_CONFIG --cflags`"
      fi
    fi
  fi
  if test x$pptcapi_no = x ; then
    AC_MSG_RESULT([yes])
    m4_if([$2], [], [:], [$2])
  else
    AC_MSG_RESULT([no])
    if test "$PPTCAPI_CONFIG" = 'no' ; then
      AC_MSG_NOTICE([The pptcapi-config script could not be found.])
    else
      if test x$pptcapi_config_is_lt = xyes ; then
        AC_MSG_NOTICE([the installed pptcapi is too old.])
      fi
    fi
    PPTCAPI_LIBS=
    PPTCAPI_CPPFLAGS=
    m4_if([$3], [], [:], [$3])
  fi
  AC_SUBST([PPTCAPI_LIBS])
  AC_SUBST([PPTCAPI_CPPFLAGS])
])
