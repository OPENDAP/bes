
# 
# These macros are used for both the netcdf3 and netcdf4 tests.

AT_INIT([bes.conf besstandalone getdap getdap4])
# AT_COPYRIGHT([])

AT_TESTED([besstandalone])
AT_COLOR_TESTS

AT_ARG_OPTION_ARG([generate g],
    AS_HELP_STRING([--generate=arg, -g arg],[Build the baseline file for test 'arg'])
    [if besstandalone -c bes.conf  -i $at_arg_generate -f $at_arg_generate.baseline; then
         echo "Built baseline for $at_arg_generate"
     else
         echo "Could not generate baseline for $at_arg_generate"
     fi     
     exit],[])

AT_ARG_OPTION_ARG([dods o],
    AS_HELP_STRING([--dods=arg, -o arg],[Build the DAP2 data baseline file for test 'arg'])
    [if besstandalone -c bes.conf  -i $at_arg_dods | getdap -M - > $at_arg_dods.baseline; then
         echo "Built baseline for $at_arg_dods"
     else
         echo "Could not generate baseline for $at_arg_dods"
     fi     
     exit],[])

AT_ARG_OPTION_ARG([dap a],
    AS_HELP_STRING([--dap=arg, -a arg],[Build the DAP4 data baseline file for test 'arg'])
    [if besstandalone -c bes.conf  -i $at_arg_dap | getdap4 -M - > $at_arg_dap.baseline; then
         echo "Built baseline for $at_arg_dap"
     else
         echo "Could not generate baseline for $at_arg_dap"
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
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $1 || true], [], [stdout], [stderr])
AT_CHECK([diff -b -B $2 stdout || diff -b -B $2 stderr], [], [ignore],[],[])
AT_CLEANUP])

m4_define([_AT_BESCMD_BINARYDATA_TEST],   
[AT_SETUP([BESCMD $1])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $1 | getdap -M - || true], [], [stdout], [stderr])
AT_CHECK([diff -b -B $2 stdout || diff -b -B $2 stderr], [], [ignore],[],[])
AT_CLEANUP])

# Note that the 'pattern' source is the <test>.baseline file.

m4_define([_AT_BESCMD_PATTERN_DATA_TEST],   
[AT_SETUP([BESCMD $1])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $1 | getdap -M - || true], [], [stdout], [stderr])
AT_CHECK([grep -f $2 stdout], [], [ignore],[],[])
AT_CLEANUP])

# Use these to actually run the tests.
#
# Usage: AT_BESCMD_*(<test> <keyword>)

m4_define([AT_BESCMD_RESPONSE_TEST],
[AT_KEYWORD([$2])
_AT_BESCMD_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline])
])

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST],
[AT_KEYWORD([$2])
_AT_BESCMD_BINARYDATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline])
])

m4_define([AT_BESCMD_PATTERN_DATA_RESPONSE_TEST],
[AT_KEYWORD([$2])
_AT_BESCMD_PATTERN_DATA_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.baseline])
])
