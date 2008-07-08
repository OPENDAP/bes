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

  if test "$openssl" != "no"; then
    if test z"$openssl" != 'z' -a "$openssl" != "yes"; then
      if test -z "$openssl_includes"; then
         openssl_includes="$openssl/include"
      fi
      if test -z "$openssl_libs"; then
         openssl_libs="$openssl/lib"
      fi
    fi

dnl --with-openssl or --with-includes and --with-libs are given
    if test z"$openssl_libs" != 'z' -a z"$openssl_includes" != 'z'; then
      if test -f $openssl_includes/openssl/ssl.h  ; then
        OPENSSL_INCLUDE=-I$openssl_includes
      fi
      # Just to be safe, we test for ".so" anyway
      if test -f $openssl_libs/libssl.a || test -f $openssl_libs/libssl.so || test -f $openssl_libs/libssl.dylib || test -f $openssl_libs/libssl/dll
      then
        OPENSSL_LIB=$openssl_libs
      fi
      if test -z "$OPENSSL_LIB" -o -z "$OPENSSL_INCLUDE"; then
        if test z"$openssl" != "z"; then
          AC_MSG_ERROR([Unable to locate OpenSSL installation using --with-openssl specified])
        else
          AC_MSG_ERROR([Unable to locate OpenSSL libraries and headers using --with options])
        fi
      fi
      bes_openssl_found=yes

dnl an error: only includes or libs was given
    elif test z"$openssl_libs" != 'z' -o z"$openssl_includes" != 'z'; then
      AC_MSG_ERROR([if either 'includes' or 'libs' is specified, both must be specified])

dnl nothing was given, search in pkgconfig, default paths and in a list
    else
      bes_openssl_found=pkgconfig
      PKG_CHECK_MODULES([OPENSSL],[openssl],,[bes_openssl_found=no])
      if test $bes_openssl_found = 'pkgconfig'; then
         openssl_includes=$OPENSSL_CFLAGS
         openssl_libs=$OPENSSL_LIBS
         private_openssl='Requires.private: openssl'
      else
         BES_OLD_LIBS=$LIBS
         AC_CHECK_HEADERS([openssl/ssl.h],[
         AC_CHECK_LIB([ssl],[SSL_library_init], [bes_openssl_found=default])
         ])
         LIBS=$BES_OLD_LIBS
	 # Added this test. jhrg 7/8/08
	 if test $bes_openssl_found = 'default'; then
            openssl_libs='-lssl -lcrypto'
            private_openssl="Libs.private: $openssl_libs"
	 fi
      fi
      if test $bes_openssl_found = 'no'; then
        for d in /usr/ssl/include /usr/local/ssl/include /usr/include \
/usr/include/ssl /opt/ssl/include /opt/openssl/include \
/usr/local/ssl/include /usr/local/include /usr/freeware/include
        do
          if test -f $d/openssl/ssl.h
          then
            OPENSSL_INCLUDE=-I$d
            bes_openssl_found=yes
          fi
        done
        
        # headers were found in a list, now look for libs
        if test $bes_openssl_found = yes; then
          bes_openssl_found=no
          for d in /usr/ssl/lib /usr/local/ssl/lib /usr/lib/openssl \
/usr/lib /usr/lib64 /opt/ssl/lib /opt/openssl/lib \
/usr/freeware/lib32 /usr/local/lib/
          do
            # Just to be safe, we test for ".so" anyway
            if test -f $d/libssl.a || test -f $d/libssl.so || test -f $d/libssl.dylib || test -f $d/libssl.dll
            then
              OPENSSL_LIB=$d
              bes_openssl_found=yes
            fi
          done
        fi
      fi
    fi
dnl if bes_openssl_found = yes it means that the libs and headers were found
dnl but not with pkgconfig nor in default places, so flags have to be added
    if test "$bes_openssl_found" = 'yes'; then
      #force VIO use
      openssl_libs="-L$OPENSSL_LIB -lssl -lcrypto"
      private_openssl="Libs.private: $openssl_libs"
      # Don't set openssl_includes to /usr/include as this gives us a lot of
      # compiler warnings when using gcc 3.x
      openssl_includes=""
      if test "$OPENSSL_INCLUDE" != "-I/usr/include"; then
        openssl_includes="$OPENSSL_INCLUDE"
      fi
    fi

dnl --with-openssl=no was explicitely specified
  else
    if test ! -z "$openssl_includes"; then
      AC_MSG_ERROR(Can't have --with-openssl-includes without --with-openssl);
    fi
    if test ! -z "$openssl_libs"; then
      AC_MSG_ERROR(Can't have --with-openssl-libs without --with-openssl);
    fi
    bes_openssl_found=no
  fi

  AC_MSG_CHECKING([for OpenSSL])
  if test z"$bes_openssl_found" != 'zno'; then
    AC_MSG_RESULT(yes)
    AC_DEFINE([HAVE_OPENSSL], [1], [OpenSSL])
    AM_CONDITIONAL([HAVE_OPENSSL], [true])
  else
    AM_CONDITIONAL([HAVE_OPENSSL], [false])
    AC_MSG_RESULT(no)
  fi

  AC_SUBST(openssl_libs)
  AC_SUBST(openssl_includes)
  AC_SUBST(private_openssl)
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

