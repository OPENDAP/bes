
# 
# These macros are used for both the netcdf3 and netcdf4 tests.

AT_INIT([bes.conf besstandalone getdap])
# AT_COPYRIGHT([])

AT_TESTED([besstandalone])

AT_ARG_OPTION_ARG([generate g],
    [  -g arg, --generate=arg   Build the baseline file for test 'arg'],
    [if ./generate_metadata_baseline.sh $at_arg_generate; then
         echo "Built baseline for $at_arg_generate"
     else
         echo "Could not generate baseline for $at_arg_generate"
     fi     
     exit],[])

AT_ARG_OPTION_ARG([generate-data a],
    [  -a arg, --generate-data=arg   Build the baseline file for test 'arg'],
    [if ./generate_data_baseline.sh $at_arg_generate_data; then
         echo "Built baseline for $at_arg_generate_data"
     else
         echo "Could not generate baseline for $at_arg_generate_data"
     fi     
     exit],[])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>)

m4_define([_AT_BESCMD_TEST],   
[AT_SETUP([BESCMD $1])
AT_KEYWORDS([bescmd])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $1 || true], [], [stdout], [stderr])
AT_CHECK([diff -b -B $2 stdout || diff -b -B $2 stderr], [], [ignore],[],[])
AT_CLEANUP])

m4_define([_AT_BESCMD_BINARYDATA_TEST],   
[AT_SETUP([BESCMD $1])
AT_KEYWORDS([bescmd])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $1 | getdap -M - || true], [], [stdout], [stderr])
AT_CHECK([diff -b -B $2 stdout || diff -b -B $2 stderr], [], [ignore],[],[])
AT_XFAIL_IF([test "$3" = "xfail"])
AT_CLEANUP])

m4_define([_AT_BESCMD_PATTERN_DATA_TEST],   
[AT_SETUP([BESCMD $1])
AT_KEYWORDS([bescmd])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $1 | getdap -M - || true], [], [stdout], [stderr])
AT_CHECK([grep -f $2 stdout], [], [ignore],[],[])
AT_CLEANUP])

m4_define([AT_BESCMD_RESPONSE_TEST],
[_AT_BESCMD_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline])
])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST],
[_AT_BESCMD_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline], [$2])
])

m4_define([AT_BESCMD_PATTERN_DATA_RESPONSE_TEST],
[_AT_BESCMD_PATTERN_DATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline])
])
