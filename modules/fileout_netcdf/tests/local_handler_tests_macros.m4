#
# Additional macros for the fileout_netcdf handler to use a special bes.conf
# file tha include new parameters for HDF5. jhrg 3/18/2020

# This new macro expects that the bes.conf file used with besstandalone will be
# the second argument. This provides a more compact way to run tests with several
# different bes.conf files. Using the "BES.Include = <other file>" we can tweak
# parameters without copying the base bes.conf file. jhrg 3/11/22
#
# Usage: AT_BESCMD_BESCONF_RESPONSE_TEST([<bescmd file>], [<bes.conf>], [pass|xfail], [repeat|cached])
# The last two params are optional.
m4_define([AT_BESCMD_BESCONF_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd])

    input=$abs_srcdir/$1

    # Here the bes_conf var is set using parameter number 2. This shadows the
    # value that can be set using the optional -c (--conf) argument (see the top
    # of this file). We might improve on this! jhrg 3/11/22
    bes_conf=$abs_builddir/$2

    # The baseline needs to contain something to tie it to the bes conf file since
    # the same bescmd file may produce different output with a different bes conf.
    baseline=$abs_srcdir/$1.$2.baseline

    # Oddly, setting 'pass' to $3 and then using $pass in AT_XFAIL_IF() does not work,
    # but using $3 does. This might be a function of when the AT_XFAIL_IF() macro is
    # expanded. jhrg 3.20.20
    pass=$3
    repeat=$4

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $bes_conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $bes_conf -i $input], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$3 = zxfail])
        ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf3 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible.
dnl
dnl Modified to take a bex.conf file as the second (required) parameter. jhrg 3/18/22

# Usage: AT_BESCMD_BESCONF_NETCDF_RESPONSE_TEST([<bescmd file>], [<bes.conf>], [pass|xfail], [repeat|cached])
# The last two params are optional.
m4_define([AT_BESCMD_BESCONF_NETCDF_RESPONSE_TEST],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data netcdf])

    input=$abs_srcdir/$1

    dnl By making this just $2 we can use exactly the same text as the original macro
    dnl except for the bes_conf and baseline values - refactor.

    bes_conf=$2
    baseline=$abs_srcdir/$1.$2.baseline

    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

        dnl first get the version number, then the header, then the data
        AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
        AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
        REMOVE_DATE_TIME([$baseline.header.tmp])
        AT_CHECK([ncdump test.nc > $baseline.data.tmp])
        REMOVE_DATE_TIME([$baseline.data.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

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

dnl Add NC4 enhanced macros, mainly I have to use another BES conf for these tests.
dnl There may be a better approach. Handle them in the future if necessary. KY 2020-02-12
m4_define([AT_BESCMD_RESPONSE_TEST_NC4_ENHANCED], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
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
    bes_conf=bes.nc4.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
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
    bes_conf=bes.nc4.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap4 -C -D -M -s -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap4 -C -D -M -s -], [], [stdout])
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
    bes_conf=bes.nc4.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         dnl first get the version number, then the header, then the data
         AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         AT_CHECK([ncdump test.nc > $baseline.data.tmp])
         REMOVE_DATE_TIME([$baseline.data.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
        
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
    bes_conf=bes.nc4.grp.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
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
    bes_conf=bes.nc4.grp.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
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
    bes_conf=bes.nc4.grp.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap4 -C -D -M -s -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap4 -C -D -M -s -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf4 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible. The netcdf4 file is in netCDF-4 enhanced model.

dnl Support an optional additional baseline with a .m_proc suffix, to support e.g. floating point 
dnl rounding differences as generated by different processors. 

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED_GRP],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.grp.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         dnl first get the version number, then the header, then the data
         AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         AT_CHECK([ncdump test.nc > $baseline.data.tmp])
         REMOVE_DATE_TIME([$baseline.data.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
        
         AT_CHECK([ncdump -k test.nc > tmp])
         AT_CHECK([diff -b -B $baseline.ver tmp])
        
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
        
         AT_CHECK([ncdump test.nc > tmp])
         REMOVE_DATE_TIME([tmp])

         AS_IF([[ test -f $baseline.data && test -f $baseline.data.m_proc ]],
            [
                # If both baselines exist, hold off on printing any diffs until knowing that neither is a match
                AS_IF([[ diff -b -B $baseline.data.m_proc tmp > /dev/null ]],
                    [
                        AS_IF([test -z "$at_verbose"], [echo "diff -b -B \$baseline.data.m_proc tmp"]) 
                    ],
                    [
                        AT_CHECK([diff -b -B $baseline.data tmp])
                    ])
             ],
             [
                AT_CHECK([diff -b -B $baseline.data tmp])
             ])

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
    bes_conf=bes.nc4.grp.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         dnl the header
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
         AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP
])

dnl platform-specific test: on Mac, output will be expected faiure. This is 
dnl because of different attribute orders by HDF5 library on Mac and on Linux.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED_GRP_HDR_OS],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.grp.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         dnl the header
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
         AT_XFAIL_IF([grep -q "darwin" <<< AT_PACKAGE_HOST])
         #AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP
])

dnl platform-specific test: on Mac, output will be expected faiure. This is 
dnl because of different attribute orders by HDF5 library on Mac and on Linux.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED_GRP_REDUCE_DIM_HDR_OS],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.grp.reduce_dim.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         dnl the header
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
         AT_XFAIL_IF([grep -q "darwin" <<< AT_PACKAGE_HOST])
         #AT_XFAIL_IF([test z$2 = zxfail])
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
    shuffle="Shuffle"
    bes_conf=bes.nc4.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         AT_CHECK([ncdump -sh test.nc > tmp])
         AT_CHECK([grep -m 1 $compression tmp >$baseline.comp.tmp]) 
         AT_CHECK([grep -m 1 $shuffle tmp >>$baseline.comp.tmp]) 
 
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
         AT_CHECK([ncdump -sh test.nc > tmp])
         dnl only need to check if the deflate compression appears.
         AT_CHECK([grep -m 1 $compression tmp >tmp2]) 
         AT_CHECK([grep -m 1 $shuffle tmp >>tmp2]) 
         AT_CHECK([diff -b -B $baseline.comp tmp2])
        
         AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP

])


dnl Only check if the netcdf-4 file is compressed.
m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_COMPRESSION_2],  [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    compression="Deflate"
    shuffle="Shuffle"
    bes_conf=bes.nc4.grp.disable_dio.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         AT_CHECK([ncdump -sh test.nc > tmp])
         AT_CHECK([grep -m 1 $compression tmp >$baseline.comp.tmp]) 
         AT_CHECK([grep -m 1 $shuffle tmp >>$baseline.comp.tmp]) 
 
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
         AT_CHECK([ncdump -sh test.nc > tmp])
         dnl only need to check if the deflate compression appears.
         AT_CHECK([grep -m 1 $compression tmp >tmp2]) 
         AT_CHECK([grep -m 1 $shuffle tmp >>tmp2]) 
         AT_CHECK([diff -b -B $baseline.comp tmp2])
        
         AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP

])

# More macros for the CF DMR direct mapping in the HDF5 handler. 
# This is mainly used for the NASA tests. KY 2021-10-11

dnl Add NC4 enhanced macros, mainly I have to use another BES conf for these tests.
m4_define([AT_BESCMD_RESPONSE_TEST_NC4_ENHANCED_CFDMR], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.cfdmr.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST_NC4_ENHANCED_CFDMR],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary])
    
    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.cfdmr.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARY_DAP4_RESPONSE_TEST_NC4_ENHANCED_CFDMR],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary DAP4])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.cfdmr.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap4 -C -D -M -s -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap4 -C -D -M -s -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf4 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible. The netcdf4 file is in netCDF-4 enhanced model.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED_CFDMR],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.cfdmr.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         dnl first get the version number, then the header, then the data
         AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         AT_CHECK([ncdump test.nc > $baseline.data.tmp])
         REMOVE_DATE_TIME([$baseline.data.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
        
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

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED_CFDMR_HDR],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.cfdmr.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         dnl the header
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
         AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST_NC4_ENHANCED_CFDMR_HDR_OS],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    pass=$2
    repeat=$3
    bes_conf=bes.nc4.cfdmr.conf

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])

         dnl the header
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
         AT_XFAIL_IF([grep -q "darwin" <<< AT_PACKAGE_HOST])
         #AT_XFAIL_IF([test z$2 = zxfail])
         ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BESCONF_NETCDF_RESPONSE_TEST_NC4_ENHANCED_GRP_RD_HDR_OS],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([nc4 enhanced binary ncdump])

    input=$abs_srcdir/$1
    bes_conf=$abs_builddir/$2

    baseline=$abs_srcdir/$1.$2.baseline
    pass=$3
    repeat=$4

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
         [
         AT_CHECK([besstandalone -c $bes_conf -i $input > test.nc])

         dnl the header
         AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
         REMOVE_DATE_TIME([$baseline.header.tmp])
         ],
         [
         AT_CHECK([besstandalone -c $bes_conf -i $input > test.nc])
         AT_CHECK([ncdump -h test.nc > tmp])
         REMOVE_DATE_TIME([tmp])
         AT_CHECK([diff -b -B $baseline.header tmp])
         AT_XFAIL_IF([grep -q "darwin" <<< AT_PACKAGE_HOST])
         ])

    AT_CLEANUP
])


