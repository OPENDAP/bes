dnl
dnl AM_PATH_CPPUNIT(MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl If configure is called using --with-cppunit-prefix=<path>, assume the version number is
dnl good enough (that the caller really wants a specific path and trust that). If the --with...
dnl is not used, use pkg-config, test for the version and set CPPUNIT_CFLAGS/LDFLAGS using
dnl the values it returns. If pkg-config cannot find 'cppunit' and --with... is not given,
dnl print a warning message. jhrg 4/29/25

AC_DEFUN([AM_PATH_CPPUNIT],
[
    AC_ARG_WITH([cppunit-prefix],
        [AS_HELP_STRING([--with-cppunit-prefix=PFX],
            [Prefix where CppUnit is installed (optional)])],
        [cppunit_prefix="$withval"],
        [cppunit_prefix=""])

    cppunit_req_ver=$1
    cppunit_ok=no
    CPPUNIT_CFLAGS=""
    CPPUNIT_LIBS=""

    AC_MSG_CHECKING([for cppunit $cppunit_req_ver or newer])

    AS_IF([test -n "$cppunit_prefix"],
        dnl then
        [cppunit_ok=yes
        CPPUNIT_CFLAGS="-I$cppunit_prefix/include"
        CPPUNIT_LIBS="-L$cppunit_prefix/lib -lcppunit"
        AC_MSG_RESULT(Using $cppunit_prefix for the CppUnit prefix)],

        dnl else if pkg-config exists and can find 'cppuint' at least version xyz
        [AS_IF([pkg-config --atleast-version=$cppunit_req_ver cppunit],
            [cppunit_ok=yes
            CPPUNIT_CFLAGS=$(pkg-config --cflags cppunit)
            CPPUNIT_LIBS=$(pkg-config --libs cppunit)
            AC_MSG_RESULT([yes; found version $(pkg-config --modversion cppunit)])],

            dnl else did not use --with... and pkg-config didn't find it
            [AC_MSG_RESULT([no, needed $cppunit_req_ver])])])

    AS_IF([test $cppunit_ok = yes],
        [ifelse([$2], , :, [$2])], dnl This idiom ensure that if $2 is empty, ':' is used
        [ifelse([$3], , :, [$3])])

    AC_SUBST(CPPUNIT_CFLAGS)
    AC_SUBST(CPPUNIT_LIBS)
])

dnl ORIGINAL VERSION HERE BELOW. jhrg 4/29/25
dnl
AC_DEFUN([DEPRECATED_AM_PATH_CPPUNIT],
[

AC_ARG_WITH(cppunit-prefix,[  --with-cppunit-prefix=PFX   Prefix where CppUnit is installed (optional)],
            cppunit_config_prefix="$withval", cppunit_config_prefix="")
AC_ARG_WITH(cppunit-exec-prefix,[  --with-cppunit-exec-prefix=PFX  Exec prefix where CppUnit is installed (optional)],
            cppunit_config_exec_prefix="$withval", cppunit_config_exec_prefix="")

  if test x$cppunit_config_exec_prefix != x ; then
     cppunit_config_args="$cppunit_config_args --exec-prefix=$cppunit_config_exec_prefix"
     if test x${CPPUNIT_CONFIG+set} != xset ; then
        CPPUNIT_CONFIG=$cppunit_config_exec_prefix/bin/cppunit-config
     fi
  fi
  if test x$cppunit_config_prefix != x ; then
     cppunit_config_args="$cppunit_config_args --prefix=$cppunit_config_prefix"
     if test x${CPPUNIT_CONFIG+set} != xset ; then
        CPPUNIT_CONFIG=$cppunit_config_prefix/bin/cppunit-config
     fi
  fi

  AC_PATH_PROG(CPPUNIT_CONFIG, cppunit-config, no)
  cppunit_version_min=$1

  AC_MSG_CHECKING(for Cppunit - version >= $cppunit_version_min)
  no_cppunit=""
  if test "$CPPUNIT_CONFIG" = "no" ; then
    AC_MSG_RESULT(no)
    no_cppunit=yes
  else
    CPPUNIT_CFLAGS=`$CPPUNIT_CONFIG --cflags`
    CPPUNIT_LIBS=`$CPPUNIT_CONFIG --libs`
    cppunit_version=`$CPPUNIT_CONFIG --version`

    cppunit_major_version=`echo $cppunit_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    cppunit_minor_version=`echo $cppunit_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    cppunit_micro_version=`echo $cppunit_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    cppunit_major_min=`echo $cppunit_version_min | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    if test "x${cppunit_major_min}" = "x" ; then
       cppunit_major_min=0
    fi
    
    cppunit_minor_min=`echo $cppunit_version_min | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    if test "x${cppunit_minor_min}" = "x" ; then
       cppunit_minor_min=0
    fi
    
    cppunit_micro_min=`echo $cppunit_version_min | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x${cppunit_micro_min}" = "x" ; then
       cppunit_micro_min=0
    fi

    cppunit_version_proper=`expr \
        $cppunit_major_version \> $cppunit_major_min \| \
        $cppunit_major_version \= $cppunit_major_min \& \
        $cppunit_minor_version \> $cppunit_minor_min \| \
        $cppunit_major_version \= $cppunit_major_min \& \
        $cppunit_minor_version \= $cppunit_minor_min \& \
        $cppunit_micro_version \>= $cppunit_micro_min `

    if test "$cppunit_version_proper" = "1" ; then
      AC_MSG_RESULT([$cppunit_major_version.$cppunit_minor_version.$cppunit_micro_version])
    else
      AC_MSG_RESULT(no)
      no_cppunit=yes
    fi
  fi

  if test "x$no_cppunit" = x ; then
     ifelse([$2], , :, [$2])     
  else
     CPPUNIT_CFLAGS=""
     CPPUNIT_LIBS=""
     ifelse([$3], , :, [$3])
  fi

  AC_SUBST(CPPUNIT_CFLAGS)
  AC_SUBST(CPPUNIT_LIBS)
])



