dnl AC_CHECK_HDF : Check for hdf4
dnl args :             action-if-yes, action-if-no

AC_DEFUN([AC_CHECK_HDF4],
[
  AC_ARG_WITH([hdf4],
            [AS_HELP_STRING([--with-hdf4=ARG],[hdf4 directory])],
            [HDF4_PATH=$withval], 
            [HDF4_PATH=""])

  AC_ARG_WITH([hdf4_include],
            [AS_HELP_STRING([--with-hdf4-include=ARG],[hdf 4 include directory])],
            [HDF4_PATH_INC=$withval],
            [HDF4_PATH_INC=""])

  AC_ARG_WITH([hdf4_libdir],
            [AS_HELP_STRING([--with-hdf4-libdir=ARG],[hdf 4 library directory])],
            [HDF4_PATH_LIBDIR=$withval], 
            [HDF4_PATH_LIBDIR=""])

  AS_IF([test "z$HDF4_PATH" != "z"],
  [
    AS_IF([test "z$HDF4_PATH_LIBDIR" = "z"],
      [HDF4_PATH_LIBDIR="$HDF4_PATH/lib"])  
    AS_IF([test "z$HDF4_PATH_INC" = "z"],
      [HDF4_PATH_INC="$HDF4_PATH/include"])  
  ])
  
  HDF4_LDFLAGS=
  AS_IF([test "z$HDF4_PATH_LIBDIR" != "z"],
    [HDF4_LDFLAGS="-L$HDF4_PATH_LIBDIR"])
  
  HDF4_LIBS=
  ac_hdf4_save_LDFLAGS=$LDFLAGS
  ac_hdf4_save_LIBS=$LIBS
  LDFLAGS="$LDFLAGS $HDF4_LDFLAGS"
  AC_CHECK_LIB([sz], [SZ_BufftoBuffCompress],
  [
      LIBS="$LIBS -lsz"
      HDF4_LIBS='-lsz'
  ])

dnl -lsz is not required because due to licencing it may not be present
dnl nor required everywhere
  ac_hdf4_lib='no'
  AC_CHECK_LIB([z],[deflate],
  [ AC_CHECK_LIB([jpeg],[jpeg_start_compress],
    [ AC_CHECK_LIB([df],[Hopen],
      [ AC_CHECK_LIB([mfhdf],[SDstart],
        [ ac_hdf4_lib="yes"
          HDF4_LIBS="-lmfhdf -ldf -ljpeg -lz $HDF4_LIBS"
        ],[],[-ldf -ljpeg -lz])
      ],[],[-ljpeg -lz])
    ])
  ])
  LDFLAGS=$ac_hdf4_save_LDFLAGS
  LIBS=$ac_hdf4_save_LIBS
  
  HDF4_CPPFLAGS=
  AS_IF([test "z$HDF4_PATH_INC" != "z"],
    [HDF4_CPPFLAGS="-I$HDF4_PATH_INC"])

  ac_hdf4_h='no'
  ac_hdf4_save_CPPFLAGS=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS $HDF4_CPPFLAGS"
  AC_CHECK_HEADER([mfhdf.h],[ac_hdf4_h='yes'])
  CPPFLAGS=$ac_hdf4_save_CPPFLAGS
  
  AS_IF([test "$ac_hdf4_h" = 'yes' -a "$ac_hdf4_lib" = 'yes'],
  [m4_if([$1], [], [:], [$1])],
  [m4_if([$2], [], [:], [$2])])

  AC_SUBST([HDF4_LIBS])
  AC_SUBST([HDF4_CPPFLAGS])
  AC_SUBST([HDF4_LDFLAGS])
])

dnl check for the netcdf 2 interface provided by hdf
AC_DEFUN([AC_CHECK_HDF4_NETCDF],
[
  ac_hdf4_netcdf_lib='no'
  ac_hdf4_netcdf_h='no'
  AC_CHECK_HDF4([
    ac_hdf4_netcdf_save_LDFLAGS=$LDFLAGS
    ac_hdf4_netcdf_save_LIBS=$LIBS
    LIBS="$LIBS $HDF4_LIBS"
    LDFLAGS="$LDFLAGS $HDF4_LDFLAGS"
    AC_MSG_CHECKING([for ncopen with hdf link flags])
    AC_LINK_IFELSE([AC_LANG_CALL([],[ncopen])],
      [
        AC_MSG_RESULT([yes])
        ac_hdf4_netcdf_lib='yes'
      ],
      [
        AC_MSG_RESULT([no])
        ac_hdf4_netcdf_lib='no'
      ])
    LDFLAGS=$ac_hdf4_netcdf_save_LDFLAGS
    LIBS=$ac_hdf4_netcdf_save_LIBS

    ac_hdf4_netcdf_save_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $HDF4_CPPFLAGS"
    AC_CHECK_NETCDF_HEADER([],[ac_hdf4_netcdf_h='yes'])
    CPPFLAGS=$ac_hdf4_netcdf_save_CPPFLAGS
  ])

  AS_IF([test $ac_hdf4_netcdf_lib = 'yes' -a $ac_hdf4_netcdf_h = 'yes'],
  [m4_if([$1], [], [:], [$1])],
  [m4_if([$2], [], [:], [$2])])
  
])
