# Check for the stare library.
# The STARE library is optional. If the user has installed STARE, the library should be located
# in $prefix/lib and all of its headers should be found in $prefix/include.
#
# Looked into using AC_SEARCH_LIBS instead of AC_CHECK_LIB, but stuck with AC_CHECK_LIB for consistency
# with our other .m4 files
# kln 4/20/19
AC_DEFUN([AC_CHECK_STARE],
[
	STARE_PATH_LIBDIR=$withval	
	
	AS_IF([test "z$STARE_PATH_LIBDIR" != "z"],
		STARE_LDFLAGS="$STARE_PATH_LIBDIR/lib"
		LDFLAGS="$LDFLAGS -L$STARE_LDFLAGS"
	
		AC_CHECK_LIB([STARE],[main],[STARE_INC=$STARE_PATH_LIBDIR/include; ac_stare_ok='yes'],[])
		#AC_SEARCH_LIBS([getIndex],[STARE],[STARE_INC=/Users/kodi/src/hyrax/hyrax-dependencies/src/STARE/include; STARE_LIB=/Users/kodi/src/hyrax/hyrax-dependencies/src/STARE/src],[])
	)
	
	AC_SUBST([STARE_INC])
	
	AC_SUBST([STARE_LIB])
	
	AS_IF([test "$ac_stare_ok" = 'no' -o "$ac_stare_header" = 'no'],
		[m4_if([$2], [], [:], [$2])],
		[m4_if([$1], [], [:], [$1])])
])
