AC_DEFUN([SIC_VAR_SYS_ERRLIST],
[AC_CACHE_CHECK([for sys_errlist],
sic_cv_var_sys_errlist,
[AC_TRY_LINK([int *p;], [extern int sys_errlist; p = &sys_errlist;],
            sic_cv_var_sys_errlist=yes, sic_cv_var_sys_errlist=no)])
if test x"$sic_cv_var_sys_errlist" = xyes; then
  AC_DEFINE(HAVE_SYS_ERRLIST, 1,
    [Define if your system libraries have a sys_errlist variable.])
fi])
