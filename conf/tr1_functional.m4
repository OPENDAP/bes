# AC_HEADER_TR1_FUNCTIONAL
AC_DEFUN([AC_HEADER_TR1_FUNCTIONAL], [
  AC_CACHE_CHECK(for tr1/functional,
  ac_cv_cxx_tr1_functional,
  [AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([#include <tr1/functional>], [using std::tr1::functional;],ac_cv_cxx_tr1_functional=yes, ac_cv_cxx_tr1_functional=no)
  AC_LANG_RESTORE
  ])
  if test "$ac_cv_cxx_tr1_functional" = yes; then
    AC_DEFINE(HAVE_TR1_FUNCTIONAL,,[Define if tr1/functional is present. ])
  fi
])
