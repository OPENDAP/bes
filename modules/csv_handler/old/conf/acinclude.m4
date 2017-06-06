
# m4 macros from the Unidata netcdf 2.3.2 pl4 distribution. Modified for use
# with GNU Autoconf 2.1. I renamed these from UC_* to DODS_* so that there
# will not be confusion when porting future versions of netcdf into the DODS
# source distribution. Unidata, Inc. wrote the text of these macros and holds
# a copyright on them.
#
# jhrg 3/27/95
#
# Added some of my own macros (don't blame Unidata for them!) starting with
# DODS_PROG_LEX and down in the file. jhrg 2/11/96
#
# Much shortened version; specific to the netcdf handler for the OPeNDAP
# data servers. jhrg 5/23/05

# $Id: acinclude.m4 12451 2005-10-19 15:48:13Z jimg $

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

AC_DEFUN([OPENDAP_OS], [dnl
    AC_MSG_CHECKING(type of operating system)
    # I have removed the following test because some systems (e.g., SGI)
    # define OS in a way that breaks this code but that is close enough
    # to also be hard to detect. jhrg 3/23/97
    #  if test -z "$OS"; then
    #  fi 
    OS=`uname -s | tr '[[A-Z]]' '[[a-z]]' | sed 's;/;;g'`
    if test -z "$OS"; then
        AC_MSG_WARN(OS unknown!)
    fi
    case $OS in
        aix)
            ;;
        hp-ux)
            OS=hpux`uname -r | sed 's/[[A-Z.0]]*\([[0-9]]*\).*/\1/'`
            ;;
        irix)
            OS=${OS}`uname -r | sed 's/\..*//'`
            ;;
        # I added the following case because the `tr' command above *seems* 
	# to fail on Irix 5. I can get it to run just fine from the shell, 
	# but not in the configure script built using this macro. jhrg 8/27/97
        IRIX)
            OS=irix`uname -r | sed 's/\..*//'`
	    ;;
        osf*)
            ;;
        sn*)
            OS=unicos
            ;;
        sunos)
            OS_MAJOR=`uname -r | sed 's/\..*//'`
            OS=$OS$OS_MAJOR
            ;;
        ultrix)
            case `uname -m` in
            VAX)
                OS=vax-ultrix
                ;;
            esac
            ;;
        *)
            # On at least one UNICOS system, 'uname -s' returned the
            # hostname (sigh).
            if uname -a | grep CRAY >/dev/null; then
                OS=unicos
            fi
            ;;
    esac

    # Adjust OS for CRAY MPP environment.
    #
    case "$OS" in
    unicos)

        case "$CC$TARGET$CFLAGS" in
        *cray-t3*)
            OS=unicos-mpp
            ;;
        esac
        ;;
    esac

    AC_SUBST(OS)

    AC_MSG_RESULT($OS)])


AC_DEFUN([OPENDAP_MACHINE], [dnl
    AC_MSG_CHECKING(type of machine)

    if test -z "$MACHINE"; then
    MACHINE=`uname -m | tr '[[A-Z]]' '[[a-z]]'`
    case $OS in
        aix*)
            MACHINE=rs6000
            ;;
        hp*)
            MACHINE=hp`echo $MACHINE | sed 's%/.*%%'`
            ;;
        sunos*)
            case $MACHINE in
                sun4*)
                    MACHINE=sun4
                    ;;
            esac
            ;;
        irix*)
            case $MACHINE in
                ip2?)
                    MACHINE=sgi
                    ;;
            esac
            ;;
	ultrix*)
	    case $MACHINE in
		vax*)
		     case "$CC" in
        		/bin/cc*|cc*)
echo "changing C compiler to \`vcc' because \`cc' floating-point is broken"
            		CC=vcc
            		;;
		     esac
		     ;;
	    esac
	    ;;

    esac
    fi

    AC_SUBST(MACHINE)
    AC_MSG_RESULT($MACHINE)])
