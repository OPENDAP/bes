#
# These macros represent the best way I've found to incorporate building baselines
# into autotest testsuites. Until Hyrax/BES has a comprehensive way to make these
# kinds of tests - using a single set of macros from one source, copy this into
# the places it's needed and hack. If substantial changes are needed, try to copy
# them back into this file. jhrg 12/14/15 

# Include this using 'm4_include(AT_TOP_SRCDIR/modules/handler_tests_macros.m4)'
# or similar. Add 	"echo 'm4_define([AT_TOP_SRCDIR], 		[@top_srcdir@])'; \"
# to the package.m4 target in Makefile.am.

# Before including these, use AT_INIT([ <name> ]) in the testsuite.at file. By including
# the pathname to the test directory in the AC_INIT() macro, you will make it much easier
# to identify the tests in a large build like the CI builds. jhrg 4/25/18

AT_TESTED([besstandalone])

AT_ARG_OPTION_ARG([baselines b],
    [--baselines=yes|no   Build the baseline file for parser test 'arg'],
    [echo "baselines set to $at_arg_baselines";
     baselines=$at_arg_baselines],[baselines=])

AT_ARG_OPTION_ARG([conf c],
    [--conf=<file>   Use <file> for the bes.conf file],
    [echo "bes configuration file set to $at_arg_conf"; bes_conf=$at_arg_conf],
    [bes_conf=bes.conf])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>, <xpass/xfail> [default is xpass] <repeat|cached> [default is no])

# @brief Run the given bes command file
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass
# @param $3 If 'repeat' or 'cached', run besstandalone using '-r 3'

m4_define([AT_BESCMD_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    # AT_XFAIL_IF is always run first regardless where it appears. jhrg 5/27/22
    AT_XFAIL_IF([test z$2 = zxfail])
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])

# This new macro expects that the bes.conf file used with besstandalone will be
# the second argument. This provides a more compact way to run tests with several
# different bes.conf files. Using the "BES.Include = <other file>" we can tweak
# parameters without copying the base bes.conf file. jhrg 3/11/22
#
# AT_BESCMD_BESCONF_RESPONSE_TEST(<bescmd file>, <bes.conf file>, [pass|xfail], [repeat|cached])
#
m4_define([AT_BESCMD_BESCONF_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    # Here the bes_conf var is set using parameter number 2. This shadows the
    # value that can be set using the optional -c (--conf) argument (see the top
    # of this file). We might improve on this! jhrg 3/11/22
    bes_conf=$abs_builddir/$2
    AT_XFAIL_IF([test z$3 = zxfail])
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
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_REPEAT_RESPONSE_TEST],
[AT_BESCMD_RESPONSE_TEST([$1], [$2], [repeat])])

# @brief Run the given bes command file
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass

m4_define([AT_BESCMD_RESPONSE_SCRUB_DATES_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"],[echo "COMMANDER KIRK: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        REMOVE_VERSIONS([stdout])
        REMOVE_DATE_TIME([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone $repeat -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        REMOVE_VERSIONS([stdout])
        REMOVE_DATE_TIME([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])
        
    AT_CLEANUP
])

# @brief Run the given bes command file.
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass

m4_define([AT_BESCMD_RESPONSE_SCRUB_BES_CONF_LINES_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])

    AS_IF([test -z "$at_verbose"],[echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        REMOVE_BES_CONF_LINES([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        REMOVE_BES_CONF_LINES([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])

dnl Simple pattern test. The baseline file holds a set of patterns, one per line,
dnl and the test will pass if any pattern matches with the test result.
dnl In many ways it is just a better version of _AT_BESCMD_ERROR_TEST below

m4_define([AT_BESCMD_RESPONSE_PATTERN_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd pattern])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [0], [stdout])
        AT_CHECK([grep -f $baseline stdout], [0], [ignore])
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_ERROR_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd error])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

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
        ])

    AT_CLEANUP
])

# Usage: AT_BESCMD_BESCONF_ERROR_RESPONSE_TEST([<bescmd file>], [<bes.conf>], [pass|xfail], [repeat|cached])
# The last two params are optional.
m4_define([AT_BESCMD_BESCONF_ERROR_RESPONSE_TEST], [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([bescmd error])

    input=$abs_srcdir/$1

    dnl By making this just $2 we can use exactly the same text as the original macro
    dnl except for the bes_conf and baseline values - refactor.

    bes_conf=$2
    baseline=$abs_srcdir/$1.$2.baseline
    AT_XFAIL_IF([test z$3 = zxfail])
    repeat=$4

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

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
        ])

    AT_CLEANUP
])

dnl Support an optional additional baseline with a .m_proc suffix, to support e.g. floating point 
dnl rounding differences as generated by different processors. 

m4_define([AT_BESCMD_BINARY_DAP2_RESPONSE_TEST],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data dap2 DAP2])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline

    AT_XFAIL_IF([test z$2 = zxfail])
    feature=$3

    dnl Hack - if 'feature' is not present in the configured_features.txt file, skip
    dnl this test. jhrg 6/2/22

    AS_IF([test -n "$feature"],
        [AT_SKIP_IF([test -z "$(grep $feature $top_srcdir/modules/configured_features.txt)"])])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone -c $bes_conf -i $1; skip if not $repeat"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input | getdap -Ms -], [0], [stdout])

        AS_IF([[ test -f $baseline && test -f $baseline.m_proc ]],
            [
                # If both baselines exist, hold off on printing any diffs until knowing that neither is a match
                AS_IF([[ diff -b -B $baseline.m_proc stdout > /dev/null ]],
                    [
                        AS_IF([test -z "$at_verbose"], [echo "diff -b -B \$baseline.m_proc stdout"]) 
                    ],
                    [
                        AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
                    ])
            ],
            [
                AT_CHECK([diff -b -B $baseline stdout], [0], [ignore])
            ])
        ])

    AT_CLEANUP
])

dnl Old name for the above macro

m4_define([AT_BESCMD_BINARYDATA_RESPONSE_TEST],
[AT_BESCMD_BINARY_DAP2_RESPONSE_TEST([$1], [$2], [$3])])

dnl Support an optional additional baseline with a .m_proc suffix, to support e.g. floating point 
dnl rounding differences as generated by different processors. 

m4_define([AT_BESCMD_BINARY_DAP4_RESPONSE_TEST],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data dap4 DAP4])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        PRINT_DAP4_DATA_RESPONSE([stdout])
        REMOVE_DAP4_CHECKSUM([stdout])
        REMOVE_DATE_TIME([stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        PRINT_DAP4_DATA_RESPONSE([stdout])
        REMOVE_DAP4_CHECKSUM([stdout])
        REMOVE_DATE_TIME([stdout])
        REMOVE_VERSIONS([stdout])

        AS_IF([[ test -f $baseline && test -f $baseline.m_proc ]],
        [
            # If both baselines exist, hold off on printing any diffs until knowing that neither is a match
            AS_IF([[ diff -b -B $baseline.m_proc stdout > /dev/null ]],
                [
                    AS_IF([test -z "$at_verbose"], [echo "diff -b -B \$baseline.m_proc stdout"]) 
                ],
                [
                    AT_CHECK([diff -b -B $baseline.m_proc stdout])
                ])
            ],
            [
                AT_CHECK([diff -b -B $baseline stdout])
            ])
        ])

    AT_CLEANUP
])

#
# AT_BESCMD_BESCONF_BINARY_DAP4_RESPONSE_TEST(<bescmd file>, <bes.conf file>, [pass|xfail], [repeat|cached])
#
m4_define([AT_BESCMD_BESCONF_BINARY_DAP4_RESPONSE_TEST],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data dap4 DAP4])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline

    # Here the bes_conf var is set using parameter number 2. This shadows the
    # value that can be set using the optional -c (--conf) argument (see the top
    # of this file). We might improve on this! jhrg 3/11/22
    bes_conf=$abs_builddir/$2

    AT_XFAIL_IF([test z$3 = zxfail])

    repeat=$4

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $bes_conf -i $input], [], [stdout])
        PRINT_DAP4_DATA_RESPONSE([stdout])
        REMOVE_DAP4_CHECKSUM([stdout])
        REMOVE_DATE_TIME([stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $bes_conf -i $input], [], [stdout])
        PRINT_DAP4_DATA_RESPONSE([stdout])
        REMOVE_DAP4_CHECKSUM([stdout])
        REMOVE_DATE_TIME([stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf3 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible.

m4_define([AT_BESCMD_BUILD_DMRPP_RESPONSE_TEST],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data dap4 DAP4])

    input=$abs_top_srcdir/$1
    dmr=$abs_top_srcdir/$1.dmr
    baseline=$abs_top_srcdir/$1.dmrpp.baseline
    AT_XFAIL_IF([test z$2 = zxfail])
    repeat=$3

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: build_dmrpp -f $input -r $dmr"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
    [
        AT_CHECK([build_dmrpp -f $input -r $dmr], [], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([build_dmrpp -f $input -r $dmr], [], [stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])

dnl This is similar to the "binary data" macro above, but instead assumes the
dnl output of besstandalone is a netcdf3 file. The binary stream is read using
dnl ncdump and the output of that is compared to a baseline. Of course, this
dnl requires ncdump be accessible.

m4_define([AT_BESCMD_NETCDF_RESPONSE_TEST],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data netcdf])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])
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
        ])

    AT_CLEANUP
])

m4_define([AT_BESCMD_DAP_FUNCTION_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([functions])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])
    repeat=$3

    AS_IF([test -n "$repeat" -a x$repeat = xrepeat -o x$repeat = xcached], [repeat="-r 3"])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        PRINT_DAP4_DATA_RESPONSE([stdout])
        REMOVE_DAP4_CHECKSUM([stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        PRINT_DAP4_DATA_RESPONSE([stdout])
        REMOVE_DAP4_CHECKSUM([stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP])

dnl This macro is called using:
dnl AT_BESCMD_BINARY_FILE_RESPONSE_TEST(bescmd, extension, expected)
dnl E.G.: AT_...(mybescmd, tif, xfail)
dnl and expects the baseline to be mybescmd.tif
dnl

dnl m4_define([AT_BESCMD_BINARY_FILE_RESPONSE_TEST],
dnl     [_AT_BESCMD_BINARY_FILE_RESPONSE_TEST([$abs_srcdir/$1], [$abs_srcdir/$1.$2], [$3])]
dnl )

dnl Use this to test responses from handlers that build files like jpeg2000,
dnl geoTIFF, etc.
dnl
dnl jhrg 2016

dnl Support an optional additional baseline with a .m_proc suffix, to support e.g. floating point 
dnl rounding differences as generated by different processors. 

m4_define([AT_BESCMD_GDAL_BINARY_FILE_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([file])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.$2
    AT_XFAIL_IF([test z$3 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input > tmp], [ignore], [ignore])
        GET_GDAL_INFO([tmp])
        AT_CHECK([mv tmp $baseline.tmp])
        ],
        [
        AT_CHECK([besstandalone -c $abs_builddir/bes.conf -i $input > tmp], [0], [stdout])
        GET_GDAL_INFO([tmp])

        AS_IF([[ test -f $baseline && test -f $baseline.m_proc ]],
        [
            # If both baselines exist, hold off on printing any diffs until knowing that neither is a match
            AS_IF([[ diff -b $baseline.m_proc tmp > /dev/null ]],
                [
                    AS_IF([test -z "$at_verbose"], [echo "diff -b \$baseline.m_proc tmp"]) 
                ],
                [
                    AT_CHECK([diff -b $baseline tmp], [ignore],)
                ])
            ],
            [
                AT_CHECK([diff -b $baseline tmp], [ignore], )
            ])
        ])

    AT_CLEANUP
])


dnl Test to make sure the h4 files can build a dmrpp when build_dmrpp_h4 is run
dnl
dnl kln 2/27/24
m4_define([AT_BUILD_DMRPP_H4_TEST], [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([build_dmrpp_h4])

    input=$abs_srcdir/$1
    input2=$abs_srcdir/$1.dmr
    baseline=$abs_srcdir/$1.dmr.baseline

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
            AT_CHECK([$abs_builddir/../build_dmrpp_h4/build_dmrpp_h4 -f $input -r $input2], [], [stdout])
            REMOVE_VERSIONS([stdout])
            AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
            AT_CHECK([$abs_builddir/../build_dmrpp_h4/build_dmrpp_h4 -f $input -r $input2], [], [stdout])
            REMOVE_VERSIONS([stdout])
            AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])


dnl build a dmrpp file without generating the missing data
m4_define([AT_BUILD_DMRPP_H4_TEST_NO_MISSING_DATA], [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([build_dmrpp_h4])

    input=$abs_srcdir/$1
    input2=$abs_srcdir/$1.dmr
    baseline=$abs_srcdir/$1.dmr.nmd.baseline

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
            AT_CHECK([$abs_builddir/../build_dmrpp_h4/build_dmrpp_h4 -f $input -r $input2 -D], [], [stdout])
            REMOVE_VERSIONS([stdout])
            AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
            AT_CHECK([$abs_builddir/../build_dmrpp_h4/build_dmrpp_h4 -f $input -r $input2 -D], [], [stdout])
            REMOVE_VERSIONS([stdout])
            AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])

dnl build a dmrpp file that users can add the location(URL) in the dmrpp file.
m4_define([AT_BUILD_DMRPP_H4_TEST_U], [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([build_dmrpp_h4])

    input=$abs_srcdir/$1
    input2=$abs_srcdir/$1.dmr
    baseline=$abs_srcdir/$1.dmr.u.baseline
    input3=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
            AT_CHECK([$abs_builddir/../build_dmrpp_h4/build_dmrpp_h4 -f $input -r $input2 -u $input3], [], [stdout])
            REMOVE_VERSIONS([stdout])
            AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
            AT_CHECK([$abs_builddir/../build_dmrpp_h4/build_dmrpp_h4 -f $input -r $input2 -u $input3], [], [stdout])
            REMOVE_VERSIONS([stdout])
            AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])

m4_define([AT_CHECK_DMRPP_TEST], [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([check_dmrpp])

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.missvars.baseline
    dap2_form=$2

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
            AT_CHECK([ls >tmp])
            AT_CHECK([$abs_builddir/../check_dmrpp $input tmp $dap2_form], [], [stdout])
            AT_CHECK([mv tmp $baseline.tmp])
        ],
        [
            AT_CHECK([ls >tmp])
            AT_CHECK([$abs_builddir/../check_dmrpp $input tmp $dap2_form], [], [stdout])
            AT_CHECK([diff -b -B $baseline tmp])
        ])

    AT_CLEANUP
])

dnl this macro tests for those dmrpp files that are supposed not to have any missing variables.
dnl If any missing variable comes up, the test will fail.
dnl We deliberately add one test that will generate missing data variables.
dnl That test will fail expectedly. If it gets passed, something is wrong.
m4_define([AT_CHECK_DMRPP_TEST_NO_MISSING_VARS], [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([check_dmrpp])

    input=$abs_srcdir/$1
    AT_XFAIL_IF([test z$2 = zxfail])
    AT_CHECK([cp -f $input tmp],[],[stdout])
    AT_CHECK([chmod u+w tmp],[],[stdout])
    AT_CHECK([$abs_builddir/../check_dmrpp $input tmp], [], [stdout])
    AT_CHECK([diff -b -B $input tmp])

    AT_CLEANUP
])

m4_define([AT_MERGE_DMRPP_TEST], [dnl
    AT_SETUP([$1])
    AT_KEYWORDS([merge_dmrpp])

    mvs_dmrpp=$abs_srcdir/$1
    orig_dmrpp=$abs_srcdir/$2
    file_path=$3
    mvs_list=$abs_srcdir/$4
    
    baseline=$abs_srcdir/$2.mrg.baseline

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
            AT_CHECK([cp -f $orig_dmrpp tmp],[],[stdout])
            AT_CHECK([chmod u+w tmp],[],[stdout])
            AT_CHECK([$abs_builddir/../merge_dmrpp $mvs_dmrpp tmp $file_path $mvs_list], [], [stdout])
            REMOVE_VERSIONS([tmp])
            AT_CHECK([mv tmp $baseline.tmp])
        ],
        [
            AT_CHECK([cp -f $orig_dmrpp tmp],[],[stdout])
            AT_CHECK([chmod u+w tmp],[],[stdout])
            AT_CHECK([$abs_builddir/../merge_dmrpp $mvs_dmrpp tmp $file_path $mvs_list], [], [stdout])
            REMOVE_VERSIONS([tmp])
            AT_CHECK([diff -b -B $baseline tmp])
        ])

    AT_CLEANUP
])


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
dnl
dnl Here is a correct (and lenient) regex for ISO-8601 date-time:
dnl [0-9]{4}-[0-9]{2}-[0-9]{2}(T| )[0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]{3})?([a-zA-Z])?([a-zA-Z])?([a-zA-Z])?([a-zA-Z])?([a-zA-Z])?
dnl
dnl Unfortunately sed won't run that so I have had to dissemble the regex into a shoddy spectre of its full
dnl glory.
dnl
dnl Modified to include time zones, then modified to include four character time zones.
dnl Most recently modified to include no time zone (this is all in the last of the
dnl four expressions). jhrg 3/18/2020

m4_define([REMOVE_DATE_TIME], [dnl
    sed -e 's@[[0-9]]\{4\}-[[0-9]]\{2\}-[[0-9]]\{2\}T[[0-9]]\{2\}:[[0-9]]\{2\}:[[0-9]]\{2\}@_DATE_TIME_SUB_@g' \
    -e 's@[[0-9]]\{4\}-[[0-9]]\{2\}-[[0-9]]\{2\} [[0-9]]\{2\}:[[0-9]]\{2\}:[[0-9]]\{2\}@_DATE_TIME_SUB_@g' \
    -e 's@_DATE_TIME_SUB_.[[0-9]]\{3,6\}@_DATE_TIME_SUB_@g' \
    -e 's@_DATE_TIME_SUB_[[ a-zA-Z]]\{0,5\}@_DATE_TIME_SUB_@g' \
    -e 's@_DATE_TIME_SUB_\(Hyrax-[[-0-9a-zA-Z.]]*\)*@removed date-time@g' \
    < $1 > $1.sed
    mv $1.sed $1
])

dnl Given a filename, remove any version string of the form <Value>3.20.9</Value>
dnl or <Value>libdap-3.20.8</Value> in that file and put "removed_version" in its
dnl place. This hack keeps the baselines more or less true to form without the
dnl obvious issue of baselines being broken when versions of the software are changed.
dnl
dnl Added support for 'dmrpp:version="3.20.9"' in the root node of the dmrpp.
dnl
dnl Added support for 'dmr:version="1.2"'.
dnl
dnl Note that the macro depends on the baseline being a file.
dnl
dnl jhrg 12/29/21

m4_define([REMOVE_VERSIONS], [dnl
  awk '{
    gsub(/<Value>[[0-9]+]\.[[0-9]+]\.[[0-9]+](-[[0-9]+])?<\/Value>/, "<Value>removed_version</Value>");
    gsub(/<Value>[[a-zA-Z._]+]-[[0-9]+]\.[[0-9]+]\.[[0-9]+](-[[0-9]+])?<\/Value>/, "<Value>removed_version</Value>");
    gsub(/[[0-9]+]\.[[0-9]+]\.[[0-9]+]-[[0-9]+]/, "removed_version");
    gsub(/dmrpp:version="[[0-9]+]\.[[0-9]+]\.[[0-9]+](-[[0-9]+])?"/, "dmrpp:version=\"removed\"");
    gsub(/dmrVersion="[[0-9]+]\.[[0-9]+]"/, "dmrVersion=\"removed\"");
    print
  }' < $1 > $1.awk
  mv $1.awk $1
])


dnl Given a filename, remove the <Value> element of a DAP4 data response as
dnl printed by getdap4 so that we dont have issues with comparing data values
dnl on big- and little-endian machines. The value of the checksum is a function
dnl of the bytes, so different word orders produce different checksums. jhrg 4/25/18

m4_define([REMOVE_DAP4_CHECKSUM], [dnl
    sed 's@<Value>[[0-9a-f]]\{8\}</Value>@removed checksum@g' < $1 > $1.sed
    dnl '
    mv $1.sed $1
])

dnl Given a filename, remove the <File> element of a response. Useful for testing errors.
dnl jhrg 10/23/19

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

dnl Remove BES.Catalog.catalog.RootDirectory and BES.module.* from the baseline or returned
dnl DMR++ response. jhrg 6/12/23
dnl
dnl Note: Using '$@' in a macro definition confuses M $@ is replaced with a list of all arguments
dnl where each argument is quoted ($* does not quote the arguments). So, I switched to using '|'
dnl as the delimiter for the sed expressions in the macro below. jhrg 6/12/23
dnl
m4_define([REMOVE_BES_CONF_LINES], [dnl
    sed -e 's|^BES\.Catalog\.catalog\.RootDirectory=.*$|removed line|g' \
        -e 's|^BES\.module\..*=.*$|removed line|g' < $1 > $1.sed
    mv $1.sed $1
])

dnl This enables the besstandalone to run by itself in AT_CHECK so we can see
dnl Error messages more easily. The conversion from a binary response to text
dnl is done here and then the text is used with diff against a text baseline.
dnl jhrg 8/1/18
 
m4_define([PRINT_DAP4_DATA_RESPONSE], [dnl
    getdap4 -C -D -M -s $1 > $1.txt
    mv $1.txt $1
])

dnl Filter these from the gdalinfo output since they vary by gdal version
dnl Upper Left  (  21.0000000,  89.0000000) ( 21d 0' 0.00"E, 89d 0' 0.00"N)

m4_define([GET_GDAL_INFO], [dnl
    gdalinfo $1 | sed 's@^\([[A-z ]]*(.*)\) (.*)@\1@g' > $1.txt
    AS_IF([test -z "$at_verbose"], [echo "gdalinfo: $1.txt"; more $1.txt])
    mv $1.txt $1
])
