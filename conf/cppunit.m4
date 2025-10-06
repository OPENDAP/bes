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
        [ifelse([$2], , :, [$2])], dnl This idiom ensures that if $2 is empty, ':' is used
        [ifelse([$3], , :, [$3])])

    AC_SUBST(CPPUNIT_CFLAGS)
    AC_SUBST(CPPUNIT_LIBS)
])
