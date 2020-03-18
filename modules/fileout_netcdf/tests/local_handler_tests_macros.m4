#
# Additional macros for the fileout_netcdf handler to use a special bes.conf
# file tha include new parameters for HDF5. jhrg 3/18/2020

dnl Add NC4 enhanced macros, mainly I have to use another BES conf for these tests.
dnl There may be a better approach. Handle them in the future if necessary. KY 2020-02-12
m4_define([AT_BESCMD_RESPONSE_TEST_NC4_ENHANCED], [dnl

    AT_SETUP([besstandalone -c bes.nc4_enhanced.conf -i $1])
    AT_KEYWORDS([nc4 enhanced])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/bes.nc4_enhanced.conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/bes.nc4_enhanced.conf -i $input], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST_NC4_ENHANCED],  [dnl

    AT_SETUP([besstandalone -c bes.nc4_enhanced.conf -i $1])
    AT_KEYWORDS([nc4 enhanced binary])
    
    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4_enhanced.conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4_enhanced.conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARY_DAP4_RESPONSE_TEST_NC4_ENHANCED],  [dnl

    AT_SETUP([besstandalone -c bes.nc4_enhanced.conf -i $1])
    AT_KEYWORDS([nc4 enhanced binary DAP4])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4_enhanced.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4_enhanced.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf3 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED],  [dnl

    AT_SETUP([besstandalone -c bes.nc4_enhanced.conf -i $1 > test.nc])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.nc4_enhanced.conf -i $input > test.nc])

         dnl first get the version number, then the header, then the data
         AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         AT_CHECK([ncdump test.nc > $baseline.data.tmp])
         REMOVE_DATE_TIME([$baseline.data.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.nc4_enhanced.conf -i $input > test.nc])
        
         AT_CHECK([ncdump -k test.nc > tmp])
         AT_CHECK([diff -b -B $baseline.ver tmp])
        
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
        
         AT_CHECK([ncdump test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.data tmp])
        
         AT_XFAIL_IF([test z$pass = zxfail])
         ])

    AT_CLEANUP
])
