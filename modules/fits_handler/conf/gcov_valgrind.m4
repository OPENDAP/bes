# Support for coverage analysis via gcov:
# from http://fragglet.livejournal.com/14291.html
 
AC_DEFUN([DODS_GCOV_VALGRIND],
[
               
coverage=no
AC_ARG_ENABLE(coverage,
[  --enable-coverage       Enable coverage testing. ],
[ coverage=yes ])
              
if [[ "$coverage" = "yes" ]]; then
    if [[ "$GCC" = "yes" ]]; then
        CFLAGS="-fprofile-arcs -ftest-coverage $CFLAGS"
    else
        AC_MSG_ERROR([Can only enable coverage when using gcc.])
    fi
fi
               
# Support for running test cases using valgrind:
               
use_valgrind=false
AC_ARG_ENABLE(valgrind,
[  --enable-valgrind       Use valgrind when running unit tests. ],
[ use_valgrind=true ])
               
if [[ "$use_valgrind" = "true" ]]; then
    AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)
             
    if [[ "$HAVE_VALGRIND" = "no" ]]; then
        AC_MSG_ERROR([Valgrind not found in PATH. ])
    fi
fi
               
AM_CONDITIONAL(USE_VALGRIND, $use_valgrind)
])
