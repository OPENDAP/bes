# Configure macros for OPeNDAP BES
# Patrick West 2006 based on libdap.m4 from Patrice Dumas 2005,
# based on freetype2.m4 from Marcelo Magallon 2001-10-26, 
# based on gtk.m4 by Owen Taylor

# AC_CHECK_BES([MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
# Test for BES and define BES_CFLAGS and BES_LIBS.
# Check that the version is above MINIMUM-VERSION 
# use when linking with a c++ aware linker, with a c linker you may also
# need -lstdc++

# Example use:
# AC_CHECK_BES([3.5.0],
#  [
#   LIBS="$LIBS $BES_LIBS"
#   CPPFLAGS="$CPPFLAGS $BES_CFLAGS"
#  ],
#  [ AC_MSG_ERROR([Cannot find OPeNDAP BES])
# ])


AC_DEFUN([AC_CHECK_BES],
[
  AC_ARG_WITH([bes],
              [AS_HELP_STRING([--with-bes=PATH],[bes install root directory])],
              [BES_PATH=${withval}], 
              [BES_PATH=""])
  BES_CONFIG=$BES_PATH/bin/bes-config
  if test -x "${BES_PATH}/bin/bes-config"
  then
    AM_CONDITIONAL([OPENDAPSERVER], [true])
    bes_min_version=m4_if([$1], [], [3.1.0], [$1])
    AC_MSG_CHECKING([for bes version >= $bes_min_version])
    bes_no=""
    bes_config_major_version=`$BES_CONFIG --version | sed 's/^bes \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\1/'`
    bes_config_minor_version=`$BES_CONFIG --version | sed 's/^bes \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\2/'`
    bes_config_micro_version=`$BES_CONFIG --version | sed 's/^bes \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\2/'`
    bes_min_major_version=`echo $bes_min_version | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    bes_min_minor_version=`echo $bes_min_version | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    bes_min_micro_version=`echo $bes_min_version | sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    bes_config_is_lt=""
    if test $bes_config_major_version -lt $bes_min_major_version ; then
      bes_config_is_lt=yes
    else
      if test $bes_config_major_version -eq $bes_min_major_version ; then
        if test $bes_config_minor_version -lt $bes_min_minor_version ; then
          bes_config_is_lt=yes
        else
          if test $bes_config_minor_version -eq $bes_min_minor_version ; then
            if test $bes_config_micro_version -lt $bes_min_micro_version ; then
              bes_config_is_lt=yes
            fi
          fi
        fi
      fi
    fi
    if test x$bes_config_is_lt = xyes ; then
      bes_no=yes
    else
      BES_LDFLAGS="`$BES_CONFIG --libs`"
      BES_CFLAGS="`$BES_CONFIG --cflags`"
      BES_MODULE_DIR="`$BES_CONFIG --prefix`/lib/bes"
      BES_LDADD="$BES_MODULE_DIR/opendap_commands.o $BES_MODULE_DIR/dods_module.o"
    fi
    if test x$bes_no = x ; then
      AC_MSG_RESULT([yes])
      m4_if([$2], [], [:], [$2])
    else
      AC_MSG_RESULT([no])
      if test "$BES_CONFIG" = "no" ; then
      AC_MSG_NOTICE([The bes-config script could not be found.])
      else
        if test x$bes_config_is_lt = xyes ; then
          AC_MSG_NOTICE([the installed bes library is too old.])
        fi
      fi
      BES_LDFLAGS=""
      BES_CFLAGS=""
      BES_LDADD=""
      BES_MODULE_DIR=""
      m4_if([$3], [], [:], [$3])
    fi
  else
    AM_CONDITIONAL([OPENDAPSERVER], [false])
    BES_LDFLAGS=""
    BES_CFLAGS=""
    BES_LDADD=""
    BES_MODULE_DIR=""
  fi
  AC_SUBST([BES_CFLAGS])
  AC_SUBST([BES_LDFLAGS])
  AC_SUBST([BES_LDADD])
  AC_SUBST([BES_MODULE_DIR])
])

