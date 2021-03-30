
# Check for the staremaster library.

# The STAREmaster library is optional, and only of use if the STARE
# library is used. If the user has installed STAREmaster, the library
# should be located in $prefix/lib and all of its headers should be
# found in $prefix/include.
#
# Use AC_CHECK_STAREMASTER(action-if-found, action-if-not-found, bes-dependencies-prefix)
#
AC_DEFUN([AC_CHECK_STAREMASTER],
[
    BES_DEPS_PREFIX=$3
  
    AC_ARG_WITH([staremaster],
                [AS_HELP_STRING([--with-staremaster=ARG],[STAREMASTER install directory])],
                [STAREMASTER_PATH=$withval], 
                [STAREMASTER_PATH="$BES_DEPS_PREFIX"])
            
    STAREMASTER_LIBS=
    STAREMASTER_LDFLAGS=
    ac_staremaster_save_LDFLAGS=$LDFLAGS
    ac_staremaster_save_LIBS=$LIBS
    ac_check_staremaster_func_checked=STAREmaster_inq_libvers

    LDFLAGS="$LDFLAGS -L$STAREMASTER_PATH/lib"

    AC_LANG_PUSH([C++])
    
    AC_CHECK_LIB([staremaster],[$ac_check_staremaster_func_checked],
        [ac_staremaster_lib_ok='yes'
         STAREMASTER_LDFLAGS=-L$STAREMASTER_PATH/lib
         STAREMASTER_LIBS=-lstaremaster],
        [ac_staremaster_lib_ok='no'])
    
    LDFLAGS=$ac_staremaster_save_LDFLAGS

    STAREMASTER_INC=
    ac_check_staremaster_header_checked=GeoFile.h
    ac_staremaster_save_CPPFLAGS=$CPPFLAGS
    
    CPPFLAGS="$CPPFLAGS -I$STAREMASTER_PATH/include"
    
    AC_CHECK_HEADER($ac_check_staremaster_header_checked, 
        [ac_staremaster_include_ok='yes'
         STAREMASTER_INC=-I$STAREMASTER_PATH/include
         AC_DEFINE([HAVE_STAREMASTER], [1], [The STAREMASTER Library is present])],
        [ac_staremaster_include_ok='no'])

    AC_LANG_POP([C++])
    
    # echo "ac_staremaster_lib_ok: $ac_staremaster_lib_ok"
    # echo "ac_staremaster_include_ok: $ac_staremaster_include_ok" 
    
    AC_SUBST(STAREMASTER_LDFLAGS)
    AC_SUBST(STAREMASTER_LIBS)
    AC_SUBST(STAREMASTER_INC)
    	
    AS_IF([test "x$ac_staremaster_lib_ok" = "xyes" -a "x$ac_staremaster_include_ok" = "xyes"],
        [m4_if([$1], [], [:], [$1])],
        [m4_if([$2], [], [:], [$2])])
])
