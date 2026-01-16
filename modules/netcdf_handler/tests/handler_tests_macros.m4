#
# These macros represent the best way I've found to incorporate builiding baselines
# into autotest testsuites. Until Hyrax/BES has a comprehensive way to make these
# kinds of tests - using a single set of macros from one source, copy this into
# the places it's needed and hack. If substantial changes are needed, try to copy
# them back into this file. jhrg 12/14/15 

# AT_INIT([bes.conf besstandalone])
# AT_COPYRIGHT([])

AT_TESTED([besstandalone])

m4_include([../../common/handler_tests_macros.m4])

AT_ARG_OPTION_ARG([baselines],
    [--baselines=yes|no   Build the baseline file for parser test 'arg'],
    [echo "baselines set to $at_arg_baselines";
     baselines=$at_arg_baselines],[baselines=])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>, <xpass/xfail> [default is xpass] <cached> [default is no])

m4_define([_AT_BESCMD_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2
    pass=$3
    cached=$4
    AS_IF([test -n "$cached" -a x$cached = xcached], [cached="-r 3"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $cached -c $abs_builddir/bes.conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        REMOVE_VERSIONS([stdout])
        ],
        [
        AT_CHECK([besstandalone $cached -c $abs_builddir/bes.conf -i $input], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

m4_define([_AT_BESCMD_BINARYDATA_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([binary])
    
    input=$1
    baseline=$2
    pass=$3

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input | getdap -Ms -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input | getdap -Ms -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

m4_define([_AT_BESCMD_DAP4_BINARYDATA_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([binary])
    
    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

# Clone the AT_BESCMD_TEST and AT_BESCMD_BINARYDATA_TEST above. Just change bes.conf to bes_mds.conf
# KY 2019-11-05
m4_define([_AT_BESCMD_MDS_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2
    pass=$3
    cached=$4
    AS_IF([test -n "$cached" -a x$cached = xcached], [cached="-r 3"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $cached -c $abs_builddir/bes_mds.conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $cached -c $abs_builddir/bes_mds.conf -i $input], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

m4_define([_AT_BESCMD_MDS_BINARYDATA_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([binary])
    
    input=$1
    baseline=$2
    pass=$3

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes_mds.conf -i $input | getdap -Ms -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes_mds.conf -i $input | getdap -Ms -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

# Clone the AT_BESCMD_TEST and AT_BESCMD_BINARYDATA_TEST above. Just change bes.conf to bes_byte2short.conf
# KY 2021-12-22
m4_define([_AT_BESCMD_BTS_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2
    pass=$3
    cached=$4
    AS_IF([test -n "$cached" -a x$cached = xcached], [cached="-r 3"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $cached -c $abs_builddir/bes_byte2short.conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $cached -c $abs_builddir/bes_byte2short.conf -i $input], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

m4_define([_AT_BESCMD_BTS_BINARYDATA_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([binary])
    
    input=$1
    baseline=$2
    pass=$3

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes_byte2short.conf -i $input | getdap -Ms -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes_byte2short.conf -i $input | getdap -Ms -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

dnl AT_CHECK (commands, [status = `0'], [stdout = `'], [stderr = `'], [run-if-fail], [run-if-pass])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf3 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of couse, this
dnl requires ncdump be accessible.

m4_define([_AT_BESCMD_NETCDF_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([netcdf])
    
    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input > test.nc])
        
        dnl first get the version number, then the header, then the data
        AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
        AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
        AT_CHECK([ncdump test.nc > $baseline.data.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input > test.nc])
        
        AT_CHECK([ncdump -k test.nc > tmp])
        AT_CHECK([diff -b -B $baseline.ver tmp])
        
        AT_CHECK([ncdump -h test.nc > tmp])
        AT_CHECK([diff -b -B $baseline.header tmp])
        
        AT_CHECK([ncdump test.nc > tmp])
        AT_CHECK([diff -b -B $baseline.data tmp])
        
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])
    
m4_define([AT_BESCMD_RESPONSE_TEST],
[_AT_BESCMD_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline])])

m4_define([AT_BESCMD_CACHED_RESPONSE_TEST],
[_AT_BESCMD_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2], [cached])])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST],
[_AT_BESCMD_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

m4_define([AT_BESCMD_BINARY_DAP4_RESPONSE_TEST],
[_AT_BESCMD_DAP4_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

m4_define([AT_BESCMD_MDS_RESPONSE_TEST],
[_AT_BESCMD_MDS_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline])])

m4_define([AT_BESCMD_MDS_BINARYDATA_RESPONSE_TEST],
[_AT_BESCMD_MDS_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

m4_define([AT_BESCMD_BTS_RESPONSE_TEST],
[_AT_BESCMD_BTS_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline])])

m4_define([AT_BESCMD_BTS_BINARYDATA_RESPONSE_TEST],
[_AT_BESCMD_BTS_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST],
[_AT_BESCMD_NETCDF_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])
