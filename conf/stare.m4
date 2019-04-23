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

# Check for the stare library.
# The stare library is optional. The user needs to provide the path to configure
# in order for us to find where/if they installed it. 
# kln 4/20/19
AC_DEFUN([AC_CHECK_STARE],
[
	#STARE_LDFLAGS=/Users/kodi/src/hyrax/hyrax-dependencies/src/STARE/
	#LDFLAGS="$LDFLAGS -L$STARE_LDFLAGS"

	STARE_INC=$STARE_FLAGS/include
	STARE_LIB=$STARE_FLAGS/src
	
	#AC_CHECK_LIB([STARE],[getIndex],[STARE_LIBS='-lSTARE -lhdf5_hl -lhdf5 -ldl'],[])
	#AC_SEARCH_LIBS([getIndex],[STARE],[STARE_INC=/Users/kodi/src/hyrax/hyrax-dependencies/src/STARE/include; STARE_LIB=/Users/kodi/src/hyrax/hyrax-dependencies/src/STARE/src],[])

	AC_SUBST([STARE_INC])
	
	AC_SUBST([STARE_LIB])
	
	#AS_IF([test "$ac_stare_ok" = 'no' -o "$ac_stare_header" = 'no'],
    #[m4_if([$2], [], [:], [$2])],
    #[m4_if([$1], [], [:], [$1])])
])
