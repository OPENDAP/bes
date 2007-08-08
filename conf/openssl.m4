AC_DEFUN([BES_FIND_OPENSSL], [
  incs="$1"
  libs="$2"
  case "$incs---$libs" in
    ---)
      for d in /usr/ssl/include /usr/local/ssl/include /usr/include \
/usr/include/ssl /opt/ssl/include /opt/openssl/include \
/usr/local/ssl/include /usr/local/include /usr/freeware/include
      do
        if test -f $d/openssl/ssl.h
        then
          OPENSSL_INCLUDE=-I$d
        fi
      done

      for d in /usr/ssl/lib /usr/local/ssl/lib /usr/lib/openssl \
/usr/lib /usr/lib64 /opt/ssl/lib /opt/openssl/lib \
/usr/freeware/lib32 /usr/local/lib/
      do
        # Just to be safe, we test for ".so" anyway
        if test -f $d/libssl.a || test -f $d/libssl.so || test -f $d/libssl.dylib || test -f $d/libssl.dll
	then
          OPENSSL_LIB=$d
        fi
      done
      ;;
    ---* | *---)
      AC_MSG_ERROR([if either 'includes' or 'libs' is specified, both must be specified])
      ;;
    * )
      if test -f $incs/openssl/ssl.h  ; then
        OPENSSL_INCLUDE=-I$incs
      fi
      # Just to be safe, we test for ".so" anyway
      if test -f $libs/libssl.a || test -f $libs/libssl.so || test -f $libs/libssl.dylib || test -f $libs/libssl/dll
      then
        OPENSSL_LIB=$libs
      fi
      ;;
  esac

# if test -z "$OPENSSL_LIB" -o -z "$OPENSSL_INCLUDE" ; then
#   echo "Could not find an installation of OpenSSL"
#   if test -n "$OPENSSL_LIB" ; then
#    if test "$TARGET_LINUX" = "true"; then
#      echo "Looks like you've forgotten to install OpenSSL development RPM"
#    fi
#   fi
#  exit 1
# fi

])

AC_DEFUN([BES_CHECK_OPENSSL], [
  AC_ARG_WITH([openssl],
              [  --with-openssl[=DIR]    Include the OpenSSL support],
              [openssl="$withval"],
              [openssl=""])

  AC_ARG_WITH([openssl-includes],
              [
  --with-openssl-includes=DIR
                          Find OpenSSL headers in DIR],
              [openssl_includes="$withval"],
              [openssl_includes=""])

  AC_ARG_WITH([openssl-libs],
              [
  --with-openssl-libs=DIR
                          Find OpenSSL libraries in DIR],
              [openssl_libs="$withval"],
              [openssl_libs=""])

  if test "$openssl" != "no"
  then
    if test z"$openssl" = 'z'; then
      bes_openssl_found=yes
      PKG_CHECK_MODULES([OPENSSL],[openssl],,[bes_openssl_found=no])
      if test $bes_openssl_found = 'yes'; then
         openssl_includes=$OPENSSL_CFLAGS
         openssl_libs=$OPENSSL_LIBS
      else
         bes_openssl_found=no
         BES_OLD_LIBS=$LIBS
         AC_CHECK_HEADERS([openssl/ssl.h],[
         AC_CHECK_LIB([ssl],[SSL_library_init], [bes_openssl_found=yes])
         ])
         LIBS=$BES_OLD_LIBS
         openssl_libs='-lssl -lcrypto'
      fi
	elif test "$openssl" != "yes"
	then
		if test -z "$openssl_includes" 
		then
			openssl_includes="$openssl/include"
		fi
		if test -z "$openssl_libs" 
		then
			openssl_libs="$openssl/lib"
		fi
	fi
    if test z"$bes_openssl_found" != 'zyes'; then
      BES_FIND_OPENSSL([$openssl_includes], [$openssl_libs])
      if test -z "$OPENSSL_LIB" -o -z "$OPENSSL_INCLUDE"
      then
	if test "$openssl" != ""
	then
	    AC_MSG_ERROR(Unable to locate OpenSSL installation using --with-openssl specified);
	fi
	bes_openssl_found=no
    else
	bes_openssl_found=yes
	#force VIO use
	openssl_libs="-L$OPENSSL_LIB -lssl -lcrypto"
	# Don't set openssl_includes to /usr/include as this gives us a lot of
	# compiler warnings when using gcc 3.x
	openssl_includes=""
	if test "$OPENSSL_INCLUDE" != "-I/usr/include"
	then
	    openssl_includes="$OPENSSL_INCLUDE"
	fi
    fi
   fi

  else
	bes_openssl_found=no
	if test ! -z "$openssl_includes"
	then
		AC_MSG_ERROR(Can't have --with-openssl-includes without --with-openssl);
	fi
	if test ! -z "$openssl_libs"
	then
		AC_MSG_ERROR(Can't have --with-openssl-libs without --with-openssl);
	fi
  fi

  AC_MSG_CHECKING(for OpenSSL)
  if test z"$bes_openssl_found" = 'zyes'; then
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_OPENSSL], [1], [OpenSSL])
    AM_CONDITIONAL([HAVE_OPENSSL], [true])
  else
    AM_CONDITIONAL([HAVE_OPENSSL], [false])
    AC_MSG_RESULT(no)
  fi

  AC_SUBST(openssl_libs)
  AC_SUBST(openssl_includes)
])

AC_DEFUN([BES_CHECK_KERBEROS], [
AC_MSG_CHECKING(for Kerberos Includes)
  AC_ARG_WITH([kerberos],
              [  --with-kerberos[=DIR]    Include the Kerberos support],
              [kerberos="$withval"],
              [kerberos=no])

  if test "$kerberos" = "no"
  then
      # On RedHat 9 we need kerberos to compile openssl
      for d in /usr/kerberos/include
      do
       if test -f $d/krb5.h  ; then
	 kerberos="$d"
       fi
      done
  fi
  if test "$kerberos"
  then
    AC_MSG_RESULT(yes)
    kerberos_includes="-I$kerberos"
  fi

  AC_SUBST(kerberos_includes)
])

