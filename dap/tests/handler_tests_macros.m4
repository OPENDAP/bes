#
# These macros represent the best way I've found to incorporate building baselines
# into autotest testsuites. Until Hyrax/BES has a comprehensive way to make these
# kinds of tests - using a single set of macros from one source, copy this into
# the places it's needed and hack. If substantial changes are needed, try to copy
# them back into this file. jhrg 12/14/15 

# Before including these, use AT_INIT([ <name> ]) in the testsuite.at file. jhrg 4/25/18

AT_TESTED([besstandalone])

AT_ARG_OPTION_ARG([baselines],
    [--baselines=yes|no   Build the baseline file for parser test 'arg'],
    [echo "baselines set to $at_arg_baselines";
     baselines=$at_arg_baselines],[baselines=])

AT_ARG_OPTION_ARG([conf],
    [--conf=<file>   Use <file> for the bes.conf file],
    [echo "baselines set to $at_arg_baselines";
     bes_conf=$at_arg_conf],[bes_conf=bes.conf])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>, <xpass/xfail> [default is xpass] <repeat|cached> [default is no])

m4_define([_AT_BESCMD_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2
    pass=$3
    repeat=$4
    
    echo "PASS: $pass"
    
    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test "z$pass" = "zxfail"])
        ])

    AT_CLEANUP
])

m4_define([_AT_BESCMD_SCRUB_DATES_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2
    pass=$3
        
    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        REMOVE_DATE_TIME([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        REMOVE_DATE_TIME([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$pass = zxfail])
        ])
        
    AT_CLEANUP
    
])

dnl This PATTREN test reads a set of patterns from a file and if any
dnl of those patterns match, the test passes. In many ways it's just
dnl a better version of _AT_BESCMD_ERROR_TEST below

m4_define([_AT_BESCMD_PATTERN_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [0], [stdout])
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
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [ignore], [stdout], [ignore])
        REMOVE_ERROR_FILE([stdout])
        REMOVE_ERROR_LINE([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [ignore], [stdout], [ignore])
        REMOVE_ERROR_FILE([stdout])
        REMOVE_ERROR_LINE([stdout])
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
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])
    
dnl OLD - Do not use this. See the macro below. jhrg 8/1/18
m4_define([_AT_BESCMD_DAP4_BINARYDATA_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([binary])
    
    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

m4_define([_AT_NEW_BESCMD_DAP4_BINARYDATA_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([binary])
    
    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        PRINT_DAP4_DATA_RESPONSE([stdout])
        REMOVE_DAP4_CHECKSUM([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        PRINT_DAP4_DATA_RESPONSE([stdout])
        REMOVE_DAP4_CHECKSUM([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

dnl AT_CHECK (commands, [status = `0'], [stdout = `'], [stderr = `'], [run-if-fail], [run-if-pass])

dnl Given a filename, remove any date-time string of the form "yyyy-mm-dd hh:mm:ss" 
dnl in that file and put "removed date-time" in its place. This hack keeps the baselines
dnl more or less true to form without the obvious issue of baselines being broken 
dnl one second after they are written.
dnl  
dnl Note that the macro depends on the baseline being a file.
dnl
dnl jhrg 6/3/16
dnl
dnl
dnl I replaced this function with the one following which is somewhat more lenient w.r.t. format and time zone. 
dnl ndp - 8/29/18
dnl m4_define([REMOVE_DATE_TIME], [dnl
dnl    sed 's@[[0-9]]\{4\}-[[0-9]]\{2\}-[[0-9]]\{2\} [[0-9]]\{2\}:[[0-9]]\{2\}:[[0-9]]\{2\}@removed date-time@g' < $1 > $1.sed
dnl    dnl '
dnl    mv $1.sed $1
dnl])


dnl Here is a correct (and lenient) regex for ISO-8601 date-time:
dnl [0-9]{4}-[0-9]{2}-[0-9]{2}(T| )[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]{3})?([a-zA-Z])?([a-zA-Z])?([a-zA-Z])?([a-zA-Z])?([a-zA-Z])?
dnl
dnl Unfortunately sed won't run that so I have had to dissemble the regex into a shoddy spectre of its full
dnl glory.
dnl
m4_define([REMOVE_DATE_TIME], [dnl
    sed -e 's@[[0-9]]\{4\}-[[0-9]]\{2\}-[[0-9]]\{2\}T[[0-9]]\{2\}:[[0-9]]\{2\}:[[0-9]]\{2\}@_DATE_TIME_SUB_@g' \
    -e 's@[[0-9]]\{4\}-[[0-9]]\{2\}-[[0-9]]\{2\} [[0-9]]\{2\}:[[0-9]]\{2\}:[[0-9]]\{2\}@_DATE_TIME_SUB_@g' \
    -e 's@_DATE_TIME_SUB_.[[0-9]]\{3\}@_DATE_TIME_SUB_@g' \
    -e 's@_DATE_TIME_SUB_[[a-zA-Z]]\{1,3\}@removed date-time@g' \
    < $1 > $1.sed
    mv $1.sed $1
])

m4_define([REMOVE_ERROR_FILE], [dnl
    sed -e 's@<File>[[0-9a-zA-Z/._]]*</File>@removed file@g' \
        -e 's@File:.*@removed file@g' < $1 > $1.sed
    dnl '
    mv $1.sed $1
])

m4_define([REMOVE_ERROR_LINE], [dnl
    sed -e 's@<Line>[[0-9]]*</Line>@removed line@g' \
        -e 's@Line:.*@removed line@g' < $1 > $1.sed
    dnl '
    mv $1.sed $1
])




dnl Given a filename, remove the <Value> element of a DAP4 data response as
dnl printed by getdap4 so that we don't have issues with comparing data values
dnl on big- and little-endian machines. The value of the checksum is a function
dnl of the bytes, so different word orders produce different checksums. jhrg 4/25/18

m4_define([REMOVE_DAP4_CHECKSUM], [dnl
    sed 's@<Value>[[0-9a-f]]\{8\}</Value>@removed checksum@g' < $1 > $1.sed
    dnl '
    mv $1.sed $1
])

dnl This enables the besstandalone to run by itself in AT_CHECK so we can see 
dnl Error messages more easily. The conversion from a binry response to text
dnl is done here and then the text is used with diff against a text baseline.
dnl jhrg 8/1/18
 
m4_define([PRINT_DAP4_DATA_RESPONSE], [dnl
    getdap4 -D -M -s $1 > $1.txt
    mv $1.txt $1
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf3 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible.

m4_define([_AT_BESCMD_NETCDF_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([netcdf])
    
    input=$1
    baseline=$2

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
        
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

dnl OLD jhrg 9/21/16

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf3 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible.

m4_define([OLD_AT_BESCMD_NETCDF_TEST_OLD],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([netcdf])
    
    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
        
        dnl first get the version number, then the header, then the data
        AT_CHECK([ncdump -k test.nc > $baseline.ver.tmp])
        AT_CHECK([ncdump -h test.nc > $baseline.header.tmp])
        AT_CHECK([ncdump test.nc > $baseline.data.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input > test.nc])
        
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

dnl END OLD. jhrg 9/21/16
    
m4_define([AT_BESCMD_RESPONSE_TEST],
[_AT_BESCMD_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
])

m4_define([AT_BESCMD_RESPONSE_SCRUB_DATES_TEST],
[_AT_BESCMD_SCRUB_DATES_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
])


m4_define([AT_BESCMD_REPEAT_RESPONSE_TEST],
[_AT_BESCMD_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2], [$3])
])

dnl Simple pattern tests. The baseline file holds a set of patterns, one per line,
dnl and the test will pass if any pattern matches with the test result. 
m4_define([AT_BESCMD_RESPONSE_PATTERN_TEST],
[_AT_BESCMD_PATTERN_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
])

m4_define([AT_BESCMD_ERROR_RESPONSE_TEST],
[_AT_BESCMD_ERROR_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
])

dnl Depreacted
m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST],
[_AT_BESCMD_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

m4_define([AT_BESCMD_BINARY_DAP2_RESPONSE_TEST],
[_AT_BESCMD_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

m4_define([AT_BESCMD_BINARY_DAP4_RESPONSE_TEST],
[_AT_NEW_BESCMD_DAP4_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST],
[_AT_BESCMD_NETCDF_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])
