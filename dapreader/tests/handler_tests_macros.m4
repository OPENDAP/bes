
# 

AT_INIT([bes.conf besstandalone getdap4])
# AT_COPYRIGHT([])

AT_TESTED([besstandalone])

AT_ARG_OPTION_ARG([generate g],
    [  -g arg, --generate=arg   Build the baseline file for test 'arg'],
    [if besstandalone -c bes.conf -i $at_arg_generate -f $at_arg_generate.baseline; then
         echo "Built baseline for $at_arg_generate"
     else
         echo "Could not generate baseline for $at_arg_generate"
     fi     
     exit],[])

dnl echo "besstandalone -c bes.conf -i $at_arg_generate_dap  | getdap4 -D -M - > $at_arg_generate_dap.baseline"

AT_ARG_OPTION_ARG([dap a],
    [  -a arg, --dap=arg   Build the baseline file for test 'arg'],
    [if besstandalone -c bes.conf -i $at_arg_generate_dap  | getdap4 -D -M - > $at_arg_generate_dap.baseline; then
         echo "Built baseline for $at_arg_generate_dap"
     else
         echo "Could not generate baseline for $at_arg_generate_dap"
     fi     
     exit],[])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>)

m4_define([AT_BESCMD_RESPONSE_TEST],
[AT_SETUP([BESCMD $1])
AT_KEYWORDS([dap2 dap4])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $abs_srcdir/$1 || true], [], [stdout], [stderr])
AT_CHECK([diff -b -B $abs_srcdir/$1.baseline stdout], [], [ignore],[],[])
# removed " || diff -b -B $abs_srcdir/$1.baseline stderr" from the above because
# it made the output hard to read/decipher.
AT_XFAIL_IF([test "$2" = "xfail"])
AT_CLEANUP])

m4_define([AT_BESCMD_PATTERN_RESPONSE_TEST],
[AT_SETUP([BESCMD $1])
AT_KEYWORDS([dap2 dap4])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $abs_srcdir/$1 || true], [], [stdout], [stderr])
AT_CHECK([grep -f $abs_srcdir/$1.baseline stdout], [], [ignore],[],[])
AT_XFAIL_IF([test "$2" = "xfail"])
AT_CLEANUP])

# DAP2 data responses

m4_define([AT_BESCMD_DATA_RESPONSE_TEST],
[AT_SETUP([BESCMD $1])
AT_KEYWORDS([dap2])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $abs_srcdir/$1 | getdap -M - || true], [], [stdout], [stderr])
AT_CHECK([diff -b -B $abs_srcdir/$1.baseline stdout], [], [ignore],[],[])
# See above re "|| diff -b -B $abs_srcdir/$1.baseline stderr"
AT_XFAIL_IF([test "$2" = "xfail"])
AT_CLEANUP])

m4_define([AT_BESCMD_PATTERN_DATA_RESPONSE_TEST],
[AT_SETUP([BESCMD $1])
AT_KEYWORDS([dap2])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $abs_srcdir/$1 | getdap -M - || true], [], [stdout], [stderr])
AT_CHECK([grep -f $abs_srcdir/$1.baseline stdout], [], [ignore],[],[])
AT_XFAIL_IF([test "$2" = "xfail"])
AT_CLEANUP])

# DAP4 Data responses

m4_define([AT_BESCMD_DAP_RESPONSE_TEST],
[AT_SETUP([BESCMD $1])
AT_KEYWORDS([dap4])
AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $abs_srcdir/$1 | getdap4 -D -M - || true], [], [stdout], [stderr])
AT_CHECK([diff -b -B $abs_srcdir/$1.baseline stdout], [], [ignore],[],[])
# See above ... "|| diff -b -B $abs_srcdir/$1.baseline stderr"
AT_XFAIL_IF([test "$2" = "xfail"])
AT_CLEANUP])
