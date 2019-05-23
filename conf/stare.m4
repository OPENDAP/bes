
# Check for the stare library.
# The STARE library is optional. If the user has installed STARE, the library should be located
# in $prefix/lib and all of its headers should be found in $prefix/include.
#
# Use AC_CHECK_STARE(action-if-found, action-if-not-found, bes-dependencies-prefix)
#
# Looked into using AC_SEARCH_LIBS instead of AC_CHECK_LIB, but stuck with AC_CHECK_LIB for consistency
# with our other .m4 files
# kln 4/20/19
#
# I modified this a bit to include a test for the header.
AC_DEFUN([AC_CHECK_STARE],
[
    BES_DEPS_PREFIX=$3
  
    AC_ARG_WITH([stare],
                [AS_HELP_STRING([--with-stare=ARG],[STARE install directory])],
                [STARE_PATH=$withval], 
                [STARE_PATH="$BES_DEPS_PREFIX"])
            
    STARE_LIBS=
    STARE_LDFLAGS=
    ac_stare_save_LDFLAGS=$LDFLAGS
    ac_stare_save_LIBS=$LIBS
    ac_check_stare_func_checked=STARE_version

    LDFLAGS="$LDFLAGS -L$STARE_PATH/lib"

    AC_LANG_PUSH([C++])
    
    AC_CHECK_LIB([STARE],[$ac_check_stare_func_checked],
        [ac_stare_lib_ok='yes'
         STARE_LDFLAGS=-L$STARE_PATH/lib
         STARE_LIBS=-lSTARE],
        [ac_stare_lib_ok='no'])
    
    LDFLAGS=$ac_stare_save_LDFLAGS

    ac_check_stare_header_checked=SpatialGeneral.h
    ac_stare_save_CPPFLAGS=$CPPFLAGS
    
    CPPFLAGS="$CPPFLAGS -I$STARE_PATH/include"
    
    AC_CHECK_HEADER($ac_check_stare_header_checked, 
        [ac_stare_include_ok='yes'
         STARE_INC=-I$STARE_PATH/include], 
        [ac_stare_include_ok='no'], [])
    
    AC_LANG_POP([C++])
    
    echo "ac_stare_lib_ok: $ac_stare_lib_ok"
    echo "ac_stare_include_ok: $ac_stare_include_ok" 
    
    AS_IF([test "x$ac_stare_lib_ok" = "xyes" -a "x$ac_stare_include_ok" = "xyes"],
        [AC_SUBST(STARE_LDFLAGS)
         AC_SUBST(STARE_LIBS)
         AC_SUBST(STARE_INC)],
        [])
    	
    AS_IF([test "x$ac_stare_lib_ok" = "xyes" -a "x$ac_stare_include_ok" = "xyes"],
        [m4_if([$1], [], [:], [$1])],
        [m4_if([$2], [], [:], [$2])])
])
