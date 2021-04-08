#
# Additional macros for the fileout_netcdf handler to use a special bes.conf
# file tha include new parameters for HDF5. jhrg 3/18/2020

dnl Add NC4 enhanced macros, mainly I have to use another BES conf for these tests.
dnl There may be a better approach. Handle them in the future if necessary. KY 2020-02-12
m4_define([AT_BESCMD_RESPONSE_TEST_NC4_ENHANCED], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/bes.nc4.conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/bes.nc4.conf -i $input], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST_NC4_ENHANCED],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary])
    
    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARY_DAP4_RESPONSE_TEST_NC4_ENHANCED],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary DAP4])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf4 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.conf -i $input > test.nc])

         dnl first get the version number, then the header, then the data
         AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         AT_CHECK([ncdump test.nc > $baseline.data.tmp])
         REMOVE_DATE_TIME([$baseline.data.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.conf -i $input > test.nc])
        
         AT_CHECK([ncdump -k test.nc > tmp])
         AT_CHECK([diff -b -B $baseline.ver tmp])
        
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
        
         AT_CHECK([ncdump test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.data tmp])
        
         AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP
])

#
# Additional macros for the fileout_netcdf handler to use a special bes.conf
# file that supports the group hierarchy.  KY 2020-06-16

dnl Add NC4 enhanced macros, mainly I have to use another BES conf for these tests.
m4_define([AT_BESCMD_RESPONSE_TEST_NC4_ENHANCED_GRP], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/bes.nc4.grp.conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/bes.nc4.grp.conf -i $input], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST_NC4_ENHANCED_GRP],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary])
    
    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.grp.conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.grp.conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARY_DAP4_RESPONSE_TEST_NC4_ENHANCED_GRP],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary DAP4])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.grp.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.grp.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf4 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible. The netcdf4 file is in netCDF-4 enhanced model.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED_GRP],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.grp.conf -i $input > test.nc])

         dnl first get the version number, then the header, then the data
         AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         AT_CHECK([ncdump test.nc > $baseline.data.tmp])
         REMOVE_DATE_TIME([$baseline.data.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.grp.conf -i $input > test.nc])
        
         AT_CHECK([ncdump -k test.nc > tmp])
         AT_CHECK([diff -b -B $baseline.ver tmp])
        
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
        
         AT_CHECK([ncdump test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.data tmp])
        
         AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf4 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible. This one only checks the header of ncdump.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED_GRP_HDR],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.grp.conf -i $input > test.nc])

         dnl the header
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         dnl REMOVE_DATE_TIME([$baseline.header.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.nc4.grp.conf -i $input > test.nc])
         AT_CHECK([ncdump -h test.nc > tmp])
         dnl REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
         AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP
])

dnl Only check if the netcdf-4 file is compressed.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_COMPRESSION],  [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    compression="Deflate"

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input > test.nc])

         AT_CHECK([ncdump -sh test.nc > tmp])
         AT_CHECK([grep -m 1 $compression tmp >$baseline.comp.tmp]) 
 
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input > test.nc])
         AT_CHECK([ncdump -sh test.nc > tmp])
         dnl only need to check if the deflate compression appears.
         AT_CHECK([grep -m 1 $compression tmp >tmp2]) 
         AT_CHECK([diff -b -B $baseline.comp tmp2])
        
         AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP

])


