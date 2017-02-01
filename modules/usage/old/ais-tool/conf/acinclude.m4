# -*- autoconf -*-
#
# autoconf macros. 
#

# Added by Ethan, 1999/06/21
# Look for perl.
# 
# I modified the regexp below to remove any text that follows the version
# number. This extra text was hosing the test. 7/15/99 jhrg

AC_DEFUN([DODS_PROG_PERL], [dnl
    AC_CHECK_PROG([PERL], [perl], [`which perl`])
    case "$PERL" in
	*perl*)
	    perl_ver=`$PERL -v 2>&1 | awk '/This is perl/ {print}'`
	    perl_ver=`echo $perl_ver | sed 's/This is perl,[[^0-9]]*\([[0-9._]]*\).*/\1/'`
            perl_ver_main=`echo $perl_ver | sed 's/\([[0-9]]*\).*/\1/'`
	    if test -n "$perl_ver" && test $perl_ver_main -ge 5
	    then
		AC_MSG_RESULT([Found perl version ${perl_ver}.])
	    else
		AC_MSG_ERROR([perl version: found ${perl_ver} should be at least 5.000.])
	    fi
	    ;;
	*)
	    AC_MSG_WARN([perl is required.])
	    ;;
    esac

    AC_SUBST(PERL)])

AC_DEFUN([OPENDAP_DEBUG_OPTION], [dnl
    AC_ARG_ENABLE(debug, 
		  [  --enable-debug=ARG      Program instrumentation (1,2)],
		  DEBUG=$enableval, DEBUG=no)

    case "$DEBUG" in
    no) 
      ;;
    1)
      AC_MSG_RESULT(Setting debugging to level 1)
      AC_DEFINE(DODS_DEBUG)
      ;;
    2) 
      AC_MSG_RESULT(Setting debugging to level 2)
      AC_DEFINE(DODS_DEBUG, , [Set instrumentation to level 1 (see debug.h)])
      AC_DEFINE(DODS_DEBUG2, , [Set instrumentation to level 2])
      ;;
    *)
      AC_MSG_ERROR(Bad debug value)
      ;;
    esac])
