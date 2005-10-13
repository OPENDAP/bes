
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
# I've added a table of contents for this file. jhrg 2/2/98
# 
# 1. Unidata-derived macros. 
# 2. Macros for finding libraries used by the core software.
# 3. Macros used to test things about the compiler
# 4. Macros for locating various systems (Matlab, etc.)
# 5. Macros used to test things about the computer/OS/hardware
#
# $Id$

# 5. Misc stuff
#---------------------------------------------------------------------------

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

