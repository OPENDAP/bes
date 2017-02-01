
AT_INIT([bes.conf besstandalone getdap])
# AT_COPYRIGHT([])

AT_TESTED([besstandalone])

AT_ARG_OPTION_ARG([baselines],
    [--baselines=yes|no   Build the baseline file for parser test 'arg'],
    [echo "baselines set to $at_arg_baselines";
     baselines=$at_arg_baselines],[baselines=])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>, <xpass/xfail> [default is xpass])

m4_define([_AT_BESCMD_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [0], [stdout])
        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

m4_define([_AT_BESCMD_PATTERN_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [0], [stdout])
        AT_CHECK([grep -f $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

m4_define([_AT_BESCMD_ERROR_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [ignore], [stdout], [ignore])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [ignore], [stdout], [ignore])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

m4_define([_AT_BESCMD_BINARYDATA_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])
    
    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])
    
m4_define([AT_BESCMD_RESPONSE_TEST],
[_AT_BESCMD_TEST([$abs_srcdir/bescmd/$1], [$abs_srcdir/bescmd/$1.baseline])
])

m4_define([AT_BESCMD_RESPONSE_PATTERN_TEST],
[_AT_BESCMD_PATTERN_TEST([$abs_srcdir/bescmd/$1], [$abs_srcdir/bescmd/$1.baseline])
])

m4_define([AT_BESCMD_ERROR_RESPONSE_TEST],
[_AT_BESCMD_ERROR_TEST([$abs_srcdir/bescmd/$1], [$abs_srcdir/bescmd/$1.baseline])
])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST],
[_AT_BESCMD_BINARYDATA_TEST([$abs_srcdir/bescmd/$1], [$abs_srcdir/bescmd/$1.baseline])
])

