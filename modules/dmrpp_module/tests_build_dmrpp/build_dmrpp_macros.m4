#
# These macros represent the best way I've found to incorporate building baselines
# into autotest testsuites. Until Hyrax/BES has a comprehensive way to make these
# kinds of tests - using a single set of macros from one source, copy this into
# the places it's needed and hack. If substantial changes are needed, try to copy
# them back into this file. jhrg 12/14/15 

# Include this using 'm4_include([../../handler_tests_macros.m4])' or similar.

# Before including these, use AT_INIT([ <name> ]) in the testsuite.at file. By including
# the pathname to the test directory in the AC_INIT() macro, you will make it much easier
# to identify the tests in a large build like the CI builds. jhrg 4/25/18

# I added this to pull in the REMOVE_VERSIONS() macro. jhrg 12/29/21
# including handler_tests_macros.m4 broke the distcheck on Ubuntu because it could not
# find 'standalone,' try not including it... jhrg 12/29/21
# m4_include([../../handler_tests_macros.m4])

AT_TESTED([build_dmrpp])

AT_ARG_OPTION_ARG([baselines],
    [--baselines=yes|no   Build the baseline file for parser test 'arg'],
    [echo "baselines set to $at_arg_baselines";
     baselines=$at_arg_baselines],[baselines=])

AT_ARG_OPTION_ARG([conf],
    [--conf=<file>   Use <file> for the bes.conf file],
    [echo "bes_conf set to $at_arg_conf"; bes_conf=$at_arg_conf],
    [bes_conf=bes.conf])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>, <xpass/xfail> [default is xpass] <repeat|cached> [default is no])

# @brief Run the given bes command file
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass
# @param $3 If 'repeat' or 'cached', run besstandalone using '-r 3'


m4_define([AT_BUILD_DMRPP],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data dap4 DAP4])

    input=$abs_top_srcdir/$1
    dmr=$abs_top_srcdir/$1.dmr
    baseline=$abs_top_srcdir/$1.dmrpp.baseline
    repeat=$3

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: build_dmrpp -f $input -r $dmr"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
    [
        AT_CHECK([build_dmrpp -f $input -r $dmr], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([build_dmrpp -f $input -r $dmr], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

m4_define([AT_BUILD_DMRPP_M],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data dap4 DAP4])

    input=$abs_top_srcdir/$1
    dmr=$abs_top_srcdir/$1.dmr
    baseline=$abs_top_srcdir/$1.dmrpp.M.baseline

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: build_dmrpp -M -f $input -r $dmr"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
    [
        AT_CHECK([build_dmrpp -M -f $input -r $dmr], [], [stdout])
        NORMAILZE_EXEC_NAME([stdout])
        REMOVE_PATH_COMPONENTS([stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([build_dmrpp -M -f $input -r $dmr], [], [stdout])
        NORMAILZE_EXEC_NAME([stdout])
        REMOVE_PATH_COMPONENTS([stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        AT_XFAIL_IF([test z$2 = zxfail])
        ])

    AT_CLEANUP
])

dnl Remove path components of DAP DMR Attributes that may vary with builds.
dml jhrg 11/22/21
dnl Usage: REMOVE_PATH_COMPONENTS(file_name)
m4_define([REMOVE_PATH_COMPONENTS], [dnl
    sed -e 's@/[[A-z0-9]][[-A-z0-9_/.]]*/dmrpp_module/@/path_removed/@g' < $1 > $1.sed
    mv $1.sed $1
])

dnl Normalize binary name. Sometime the build_dmrpp program is named 'build_dmrpp,'
dnl other times it is named 'lt-build_dmrpp.' This ensure it always has the same name
dnl in the baselines and test output.
dml jhrg 11/22/21
dnl Usage: NORMAILZE_EXEC_NAME(file_name)
m4_define([NORMAILZE_EXEC_NAME], [dnl
    sed -e 's@/[[A-z0-9]][[-A-z0-9_/.]]*build_dmrpp @build_dmrpp @g' < $1 > $1.sed
    mv $1.sed $1
])

dnl Given a filename, remove any version string of the form <Value>3.20.9</Value>
dnl or <Value>libdap-3.20.8</Value> in that file and put "removed version" in its
dnl place. This hack keeps the baselines more or less true to form without the
dnl obvious issue of baselines being broken when versions of the software are changed.
dnl
dnl Added support for 'dmrpp:version="3.20.9"' in the root node of the dmrpp.
dnl
dnl Note that the macro depends on the baseline being a file.
dnl
dnl jhrg 12/29/21

m4_define([REMOVE_VERSIONS], [dnl
      sed -e 's@<Value>[[0-9]]*\.[[0-9]]*\.[[0-9]]*</Value>@<Value>removed version</Value>@g' \
      -e 's@<Value>[[A-z_.]]*-[[0-9]]*\.[[0-9]]*\.[[0-9]]*</Value>@<Value>removed version</Value>@g' \
      -e 's@dmrpp:version="[[0-9]]*\.[[0-9]]*\.[[0-9]]*"@removed dmrpp:version@g' \
      < $1 > $1.sed
      mv $1.sed $1
  ])
