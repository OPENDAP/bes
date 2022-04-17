dnl example of use
dnl AC_CHECK_HDF5(
dnl   [
dnl       LIBS="$LIBS $H5_LIBS"
dnl       LDFLAGS="$LDFLAGS $H5_LDFLAGS"
dnl       CPPFLAGS="$CPPFLAGS $H5_CPPFLAGS"
dnl   ],
dnl   [
dnl       echo "*** Use --with-hdf5 for the root hdf5 directory."
dnl       echo "*** Otherwise use --with-hdf5-include switch for includes directory"
dnl       echo "*** and --with-hdf5-libdir switch for libraries directory."
dnl       AC_MSG_ERROR([hdf5 library and hdf5 headers are required.])
dnl   ]
dnl )

# Check for the hdf5 library.
# AC_CHECK_HDF5([ACTION-IF-FOUND],[ACTION-IF-NOT-FOUND],[INTERFACE-NR])
# if interface number is given, check for a specific interface
# sets H5_LDFLAGS, H5_LIBS, and, by calling other macros
# H5_CPPFLAGS
AC_DEFUN([AC_CHECK_HDF5],
[
  AC_ARG_WITH([hdf5],
            [AS_HELP_STRING([--with-hdf5=ARG],[hdf5 directory])],
            [H5_PATH=$withval], 
            [H5_PATH=""])
            
  AC_SUBST([H5_PATH])

  AC_ARG_WITH([hdf5_include],
            [AS_HELP_STRING([--with-hdf5-include=ARG],[hdf5 include directory])],
            [H5_PATH_INC=$withval], 
            [H5_PATH_INC=""])

  AC_ARG_WITH([hdf5_libdir],
            [AS_HELP_STRING([--with-hdf5-libdir=ARG],[hdf5 library directory])],
            [H5_PATH_LIBDIR=$withval], 
            [H5_PATH_LIBDIR=""])

  AS_IF([test "z$H5_PATH" != "z"],
  [
    AS_IF([test "z$H5_PATH_LIBDIR" = "z"],[H5_PATH_LIBDIR="$H5_PATH/lib"])
    AS_IF([test "z$H5_PATH_INC" = "z"],[H5_PATH_INC="$H5_PATH/include"])
  ])

  ac_hdf5_ok='no'
  H5_LIBS=
  H5_LDFLAGS=
  ac_h5_save_LDFLAGS=$LDFLAGS
  ac_h5_save_LIBS=$LIBS
  ac_check_h5_func_checked='H5open'
  
  AS_IF([test "z$H5_PATH_LIBDIR" != "z"],
    [
      H5_LDFLAGS="-L$H5_PATH_LIBDIR"
      LDFLAGS="$LDFLAGS $H5_LDFLAGS"
dnl the autoconf internal cache isn't avoided because we really check for
dnl libhdf5, other libraries that implement the same api have other names
dnl  AC_LINK_IFELSE([AC_LANG_CALL([],[$ac_check_func_checked])],
      AC_CHECK_LIB([hdf5],[$ac_check_h5_func_checked],
        [
          H5_LIBS='-lhdf5'
          ac_hdf5_ok='yes'
        ])
    ],
    [
      for ac_hdf5_libdir in "." \
       /usr/local/hdf5/lib64 \
       /opt/hdf5/lib64 \
       /usr/hdf5/lib64 \
       /usr/local/lib64/hdf5 \
       /opt/lib64/hdf5 \
       /usr/lib64/hdf5 \
       /usr/local/hdf5/lib64 \
       /opt/hdf5/lib64 \
       /usr/hdf5/lib64 \
       /usr/local/lib64/hdf5 \
       /opt/lib64/hdf5 \
       /usr/lib64/hdf5 \
       /usr/local/hdf5/lib \
       /opt/hdf5/lib \
       /usr/hdf5/lib \
       /usr/local/lib/hdf5 \
       /opt/lib/hdf5 \
       /usr/lib/hdf5 \
       /usr/local/hdf5/lib \
       /opt/hdf5/lib \
       /usr/hdf5/lib \
       /usr/local/lib/hdf5 \
       /opt/lib/hdf5 \
       /usr/lib/hdf5 \
       /opt/local/lib /opt/local/include /sw/lib /sw/include ; do
        AS_IF([test "z$ac_hdf5_libdir" = 'z'],
          [H5_LDFLAGS=],
          [
            AC_MSG_CHECKING([for hdf5 libraries in $ac_hdf5_libdir])
            H5_LDFLAGS="-L$ac_hdf5_libdir"
          ])
        LDFLAGS="$LDFLAGS $H5_LDFLAGS"
        LIBS="$LIBS -lhdf5"
dnl we have to avoid the autoconf internal cache in that case
        AC_LINK_IFELSE([AC_LANG_CALL([],[$ac_check_h5_func_checked])],
          [
            H5_LIBS='-lhdf5'
            ac_hdf5_ok='yes'
            AS_IF([test "z$ac_hdf5_libdir" != 'z'],[AC_MSG_RESULT([yes])])
          ],
          [
            AS_IF([test "z$ac_hdf5_libdir" != 'z'],[AC_MSG_RESULT([no])])
          ])
        AS_IF([test $ac_hdf5_ok = 'yes'],[break])
        LDFLAGS=$ac_h5_save_LDFLAGS
        LIBS=$ac_h5_save_LIBS
      done
    ])
  LDFLAGS=$ac_h5_save_LDFLAGS
  LIBS=$ac_h5_save_LIBS

  AC_SUBST([H5_LDFLAGS])
  AC_SUBST([H5_LIBS])
  ac_hdf5_header='no'

  AS_IF([test "z$H5_PATH_INC" != "z"],
    [
      AC_CHECK_HDF5_HEADER([$H5_PATH_INC],
        [ac_hdf5_header='yes'],
        [ac_hdf5_header='no'])
    ],
    [
      for ac_hdf5_incdir in "." \
       /usr/local/hdf5/include \
       /opt/hdf5/include \ 
       /usr/hdf5/include \
       /usr/local/include/hdf5 \
       /opt/include/hdf5 \
       /usr/include/hdf5 \
       /usr/local/hdf5/include \
       /opt/hdf5/include \
       /usr/hdf5/include \
       /usr/local/include/hdf5 \
       /opt/include/hdf5 \
       /usr/include/hdf5 ; do
        AC_MSG_NOTICE([searching hdf5 includes in $ac_hdf5_incdir])
        AC_CHECK_HDF5_HEADER([$ac_hdf5_incdir],[ac_hdf5_header='yes'],
          [ac_hdf5_header='no'])
        AS_IF([test $ac_hdf5_header = 'yes'],[break])
      done
    ])

  AS_IF([test "$ac_hdf5_ok" = 'no' -o "$ac_hdf5_header" = 'no'],
    [m4_if([$2], [], [:], [$2])],
    [m4_if([$1], [], [:], [$1])])
])
