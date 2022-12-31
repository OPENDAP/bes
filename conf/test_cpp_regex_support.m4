# -*- mode: autoconf -*-
# Configure macros for BES
#
# Based on bes.m4 by Patrice Dumas, et al.

dnl From https://stackoverflow.com/questions/12530406/is-gcc-4-8-or-earlier-buggy-about-regular-expressions
dnl jhrg 12/30/22

cat << EOF | g++ --std=c++11 -x c++ - && ./a.out
      #include <regex>

      #if __cplusplus >= 201103L &&                             \
          (!defined(__GLIBCXX__) || (__cplusplus >= 201402L) || \
              (defined(_GLIBCXX_REGEX_DFS_QUANTIFIERS_LIMIT) || \
               defined(_GLIBCXX_REGEX_STATE_LIMIT)           || \
                   (defined(_GLIBCXX_RELEASE)                && \
                   _GLIBCXX_RELEASE > 4)))
      #define HAVE_WORKING_REGEX 1
      #else
      #define HAVE_WORKING_REGEX 0
      #endif

      #include <iostream>

      int main() {
        const std::regex regex(".*");
        const std::string string = "This should match!";
        const auto result = std::regex_search(string, regex);
      #if HAVE_WORKING_REGEX
        std::cerr << "<regex> works, look: " << std::boolalpha << result << std::endl;
      #else
        std::cerr << "<regex> doesn't work, look: " << std::boolalpha << result << std::endl;
      #endif
        return result ? EXIT_SUCCESS : EXIT_FAILURE;
      }
EOF


# AC_CHECK_WORKING_CPP_REGEX(ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND])
# Test for working regex code in the C++ STL.

AC_DEFUN([AC_CHECK_WORKING_CPP_REGEX],
[
  AC_TRY_RUN (program, [action-if-true], [action-if-false], [action-if-cross-compiling])
  bes_min_version=m4_if([$1], [], [3.1.0], [$1])
  bes_no=
  bes_pkgconfig_bes=yes
  PKG_CHECK_MODULES([BES_DISPATCH],[bes_dispatch >= $bes_min_version],,
     [bes_pkgconfig_bes=no])
  PKG_CHECK_MODULES([BES_PPT],[bes_ppt >= $bes_min_version],,
     [bes_pkgconfig_bes=no])
  PKG_CHECK_MODULES([BES_COMMAND],[bes_command >= $bes_min_version],,
     [bes_pkgconfig_bes=no])

  AC_PATH_PROG([BES_CONFIG], [bes-config], [no])
  if test "$BES_CONFIG" != 'no' ; then
    BES_MODULE_DIR="`$BES_CONFIG --modulesdir`"
  fi

  if test $bes_pkgconfig_bes = yes; then
    BES_LIBS="$BES_DISPATCH_LIBS $BES_COMMAND_LIBS $BES_PPT_LIBS"
    BES_CPPFLAGS=$BES_DISPATCH_CFLAGS
  else
    AC_MSG_CHECKING([for bes version >= $bes_min_version])
    if test "$BES_CONFIG" = 'no' ; then
      bes_no=yes
    else
      bes_config_major_version=`$BES_CONFIG --version | sed 's/^bes \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\1/'`
      bes_config_minor_version=`$BES_CONFIG --version | sed 's/^bes \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\2/'`
      bes_config_micro_version=`$BES_CONFIG --version | sed 's/^bes \([[0-9]]\)*\.\([[0-9]]*\)\.\([[0-9]]*\)$/\3/'`
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
        BES_LIBS="`$BES_CONFIG --libs`"
        BES_DAP_LIBS="`$BES_CONFIG --dap-libs`"
        BES_DISPATCH_LIBS=$BES_LIBS
        BES_COMMAND_LIBS=$BES_LIBS
        BES_PPT_LIBS=$BES_LIBS
        BES_CPPFLAGS="`$BES_CONFIG --cflags`"
      fi
    fi
  fi
  if test x$bes_no = x ; then
    AC_MSG_RESULT([yes])
    m4_if([$2], [], [:], [$2])
  else
    AC_MSG_RESULT([no])
    if test "$BES_CONFIG" = 'no' ; then
      AC_MSG_NOTICE([The bes-config script could not be found.])
    else
      if test x$bes_config_is_lt = xyes ; then
        AC_MSG_NOTICE([the installed bes is too old.])
      fi
    fi
    BES_LIBS=
    BES_DAP_LIBS=
    BES_DISPATCH_LIBS=
    BES_PPT_LIBS=
    BES_COMMAND_LIBS=
    BES_CPPFLAGS=
    BES_MODULE_DIR=
    m4_if([$3], [], [:], [$3])
  fi
  AC_SUBST([BES_LIBS])
  AC_SUBST([BES_DAP_LIBS])
  AC_SUBST([BES_DISPATCH_LIBS])
  AC_SUBST([BES_PPT_LIBS])
  AC_SUBST([BES_COMMAND_LIBS])
  AC_SUBST([BES_CPPFLAGS])
  AC_SUBST([BES_MODULE_DIR])
])
