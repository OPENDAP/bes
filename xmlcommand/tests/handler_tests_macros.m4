#
# These macros represent the best way I've found to incorporate building baselines
# into autotest testsuites. Until Hyrax/BES has a comprehensive way to make these
# kinds of tests - using a single set of macros from one source, copy this into
# the places it's needed and hack. If substantial changes are needed, try to copy
# them back into this file. jhrg 12/14/15 

AT_INIT([bes.conf besstandalone])
# AT_COPYRIGHT([])

AT_TESTED([besstandalone])

AT_ARG_OPTION_ARG([baselines],
    [--baselines=yes|no   Build the baseline file for parser test 'arg'],
    [echo "baselines set to $at_arg_baselines";
     baselines=$at_arg_baselines],[baselines=])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>, <xpass/xfail> [default is xpass])

m4_define([AT_BESCMD_RESPONSE_TEST],
[_AT_BESCMD_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
])

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

m4_define([AT_BESCMD_NO_DATE_RESPONSE_TEST],
[_AT_BESCMD_NO_DATE_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
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
    
m4_define([REMOVE_DATE_TIME], [dnl
    sed -i -e 's@[[0-9]]\{4\}-[[0-9]]\{2\}-[[0-9]]\{2\}T[[0-9]]\{2\}:[[0-9]]\{2\}:[[0-9]]\{2\}@removed date-time@g' $1
    sed -i -e 's@UTC@@g' $1
    sed -i -e 's@GMT@@g' $1
    dnl ' Added the preceding quote to quiet the Eclipse syntax checker. jhrg 3.2.18
])

m4_define([_AT_BESCMD_NO_DATE_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [0], [stdout])
        REMOVE_DATE_TIME([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [0], [stdout])
        REMOVE_DATE_TIME([stdout])
        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_RESPONSE_PATTERN_TEST],
[_AT_BESCMD_PATTERN_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
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

m4_define([AT_BESCMD_ERROR_RESPONSE_TEST],
[_AT_BESCMD_ERROR_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
])

m4_define([REMOVE_FILE_AND_LINE], [dnl
    sed -i -e 's/<Line>.*$//g' $1
    sed -i -e 's/<File>.*$//g' $1
    dnl ' Added the preceding quote to quiet the Eclipse syntax checker.
])

m4_define([_AT_BESCMD_ERROR_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([errors])

    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [ignore], [stdout], [ignore])
        dnl This removed the <File> And <Line> elements from an error response.
        REMOVE_FILE_AND_LINE([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [ignore], [stdout], [ignore])
        REMOVE_FILE_AND_LINE([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST],
[_AT_BESCMD_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

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
    
m4_define([AT_BESCMD_BINARY_DAP4_RESPONSE_TEST],
[_AT_BESCMD_DAP4_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])

m4_define([_AT_BESCMD_DAP4_BINARYDATA_TEST],  [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([binary])
    
    input=$1
    baseline=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input | getdap4 -D -M -s -], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test "$3" = "xfail"])
        ])

    AT_CLEANUP
])

dnl When commands write a file as a side effect, test that the file is written.
dnl The 'output pathname' is the relative path to the new file made by the command(s)
dnl in 'bescmd'. The optional diff | lines argumnet controls whether the baseline 
dnl content is compared with teh new file or just the number of lines (default is diff).
dnl
dnl args: bescmd, output pathname, [diff | lines], [pass | xfail] 
dnl
m4_define([AT_BESCMD_RESPONSE_AND_FILE_TEST],
[_AT_BESCMD_AND_FILE_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2], [$3], [$4])])

m4_define([COMPARE_FILE_LINE_LENGTHS], [dnl
    length1=`wc -l $1 | sed 's@[[ ]]*\([[0-9]]*\).*@\1@g'`
    length2=`wc -l $2 | sed 's@[[ ]]*\([[0-9]]*\).*@\1@g'`

    echo "testing: $length1 -eq $length2"
    test $length1 -eq $length2
])

dnl args: input bescmd, test baseline, output file, [diff|lines], [pass|xfail]
m4_define([_AT_BESCMD_AND_FILE_TEST], [dnl

    AT_SETUP([BESCMD $1])
    AT_KEYWORDS([bescmd])

    input=$1
    baseline=$2
    output=$abs_builddir/$3

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [0], [ignore])
        AT_CHECK([mv $output $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input], [0], [ignore])
        
        AS_IF([test "x$4" = "xlines"],
            [AT_CHECK([COMPARE_FILE_LINE_LENGTHS([$baseline], [$output])], [0], [ignore])  ],
            [AT_CHECK([diff -b -B $baseline $output], [0], [ignore])])

        AT_XFAIL_IF([test "$5" = "xfail"])
        ])

    AT_CLEANUP
])


dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf3 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST],
[_AT_BESCMD_NETCDF_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])])
    
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
   

