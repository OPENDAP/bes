# Check for the hdf5 header 'hdf5.h'.
# AC_CHECK_HDF5_HEADER([INCLUDE-DIR],[ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# if interface number is given, check for a specific interface
# sets H5_CPPFLAGS.
AC_DEFUN([AC_CHECK_HDF5_HEADER],
[
  H5_CPPFLAGS=
  ac_hdf5_h='no'
  ac_hdf5_h_compile='no'
  ac_hdf5_h_preproc='no'
  ac_h5_include_dir=
  
  ac_h5_save_CPPFLAGS=$CPPFLAGS
  m4_if([$1],[],[:],[
    ac_h5_include_dir="$1"
    AS_IF([test "z$ac_h5_include_dir" != "z"],
       [CPPFLAGS="$CPPFLAGS -I$ac_h5_include_dir"])
  ])
  AC_MSG_CHECKING([for hdf5.h with compiler])
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <hdf5.h>]])],
    [
      AC_MSG_RESULT([yes])
      ac_hdf5_h_compile='yes'
    ],
    [
      AC_MSG_RESULT([no])
      ac_hdf5_h_compile='no'
    ])
    AC_MSG_CHECKING([for hdf5.h with preprocessor])
    AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <hdf5.h>]])],
    [
      AC_MSG_RESULT([yes])
      ac_hdf5_h_preproc='yes'
    ],
    [
      AC_MSG_RESULT([no])
      ac_hdf5_h_preproc='no'
    ])
  CPPFLAGS="$ac_h5_save_CPPFLAGS"

  AS_IF([test $ac_hdf5_h_compile = 'yes'], [ac_hdf5_h='yes'])
  AS_IF([test $ac_hdf5_h_preproc = 'yes'], [ac_hdf5_h='yes'])

  AS_IF([test "$ac_hdf5_h" = 'yes'],
    [
      AS_IF([test "z$ac_h5_include_dir" != "z"],
        [H5_CPPFLAGS="-I$ac_h5_include_dir"])
      m4_if([$2], [], [:], [$2])
    ],
    [m4_if([$3], [], [:], [$3])])

  AC_SUBST([H5_CPPFLAGS])
])
