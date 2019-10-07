
dnl Shamelessly copied from stackoverflow
dnl https://stackoverflow.com/questions/1354996/need-an-autoconf-macro-that-detects-if-m64-is-a-valid-compiler-option
dnl jhrg 3/22/19
dnl
dnl @synopsis CXX_FLAGS_CHECK [compiler flags] [ACTION-IF-SUPPORTED] [ACTION-IF-NOT-SUPPORTED]                                       
dnl @summary check whether compiler supports given C++ flags or not                   
AC_DEFUN([CXX_FLAGS_CHECK],                                                            
[dnl                                                                                  
  AC_MSG_CHECKING([if $CXX supports $1])
  AC_LANG_PUSH([C++])
  ac_saved_cxxflags="$CXXFLAGS"                                                       
  CXXFLAGS="-Werror $1"                                                               
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],                                            
    [AC_MSG_RESULT([yes])]
    $2,                                                           
    [AC_MSG_RESULT([no])]
    $3                                                              
  )                                                                                   
  CXXFLAGS="$ac_saved_cxxflags"                                                       
  AC_LANG_POP([C++])
])
