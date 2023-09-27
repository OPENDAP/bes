
# These macros are specific to the DMR++ handler for now. jhrg 9/8/23

# NB: AT_CHECK (commands, [status = ‘0’], [stdout], [stderr], [run-if-fail], [run-if-pass])

# @brief Run the given bes command file several times
#
# Test several responses from a single call to besstandalone. This macro
# runs the same bes command file three times. jhrg 9/8/23
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass

m4_define([AT_BESCMD_MULTI_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd])

    # Don't know why this has to be here, inside a macro and not at the top level. jhrg 9/8/23
    response_split=$top_srcdir/standalone/response-split.py

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline

    # AT_XFAIL_IF is always run first regardless where it appears. jhrg 5/27/22
    AT_XFAIL_IF([test z$2 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone -d cerr,dmrpp:cache$repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -r 3 -c $abs_builddir/$bes_conf -i $input], [], [stdout], [stderr])

        # Split the response into separate files
        AT_CHECK([$response_split -i stdout -o ${baseline}_r.tmp])
        AT_CHECK([mv stderr ${baseline}_r.debug.tmp])
        ],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -r 3 -c $abs_builddir/$bes_conf -i $input], [], [stdout], [stderr])
        AT_CHECK([$response_split -i stdout -o response])

        # Check the individual responses
        AT_CHECK([diff -b -B response_1 ${baseline}_r_1])
        AT_CHECK([diff -b -B response_2 ${baseline}_r_2])
        AT_CHECK([diff -b -B response_3 ${baseline}_r_3])

        # Check that caching was used
        # NB: 'miss' will be 1 for dmr, but two for dds or das.
        AT_CHECK([test $(grep -c 'Cache miss' stderr) -le 2])
        AT_CHECK([test $(grep -c 'Cache hit' stderr) -eq 2])
        ])

    AT_CLEANUP
])

# @brief Run several commands that produce a DAP4 data response
#
# Test several responses from a single call to besstandalone. This macro
# runs the same bes command file three times. jhrg 9/8/23
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass

m4_define([AT_BINARY_DAP4_MULTI_RESPONSE_TEST],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data dap4 DAP4])

    # Don't know why this has to be here, inside a macro and not at the top level. jhrg 9/8/23
    response_split=$top_srcdir/standalone/response-split.py

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone -d cerr,dmrpp:cache $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -r 3 -c $abs_builddir/$bes_conf -i $input], [], [stdout], [stderr])
        # Split the response into separate files
        AT_CHECK([$response_split -i stdout -o ${baseline}_r.tmp])

        PROCESS_DAP4_RESPONSE([${baseline}_r.tmp_1])
        PROCESS_DAP4_RESPONSE([${baseline}_r.tmp_2])
        PROCESS_DAP4_RESPONSE([${baseline}_r.tmp_3])

        AT_CHECK([mv stderr ${baseline}_r.debug.tmp])
        ],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -r 3 -c $abs_builddir/$bes_conf -i $input], [], [stdout], [stderr])
        # Split the response into separate files
        AT_CHECK([$response_split -i stdout -o response])

        PROCESS_DAP4_RESPONSE([response_1])
        PROCESS_DAP4_RESPONSE([response_2])
        PROCESS_DAP4_RESPONSE([response_3])

        # Check the individual responses
        AT_CHECK([diff -b -B response_1 ${baseline}_r_1])
        AT_CHECK([diff -b -B response_2 ${baseline}_r_2])
        AT_CHECK([diff -b -B response_3 ${baseline}_r_3])

        # Check that caching was used
        # NB: 'miss' will be 1 for dmr, but two for dds or das.
        AT_CHECK([test $(grep -c 'Cache miss' stderr) -le 2])
        AT_CHECK([test $(grep -c 'Cache hit' stderr) -eq 2])
        ])

    AT_CLEANUP
])

# @brief Run several commands that produce a DAP4 data response
#
# Test several responses from a single call to besstandalone. This macro
# runs three different bes command files. jhrg 9/8/23
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 The command file, assumes that the baseline is $2.baseline
# @param $3 The command file, assumes that the baseline is $3.baseline
# @param $4 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass

m4_define([AT_BINARY_DAP4_ENUMERATED_RESPONSE_TEST],  [dnl

    AT_SETUP([$1, $2, $3])
    AT_KEYWORDS([bescmd data dap4 DAP4])

    # Don't know why this has to be here, inside a macro and not at the top level. jhrg 9/8/23
    response_split=$top_srcdir/standalone/response-split.py

    input_1=$abs_srcdir/$1
    baseline_1=$abs_srcdir/$1.baseline
    input_2=$abs_srcdir/$2
    baseline_2=$abs_srcdir/$2.baseline
    input_3=$abs_srcdir/$3
    baseline_3=$abs_srcdir/$3.baseline

    AT_XFAIL_IF([test z$4 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone -d cerr,dmrpp:cache -c $bes_conf -i $1 -i $1 -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -c $abs_builddir/$bes_conf -i $input_1 -i $input_2 -i $input_3], [], [stdout], [stderr])
        # Split the response into separate files
        AT_CHECK([$response_split -i stdout -o tmp])

        mv tmp_1 ${baseline_1}_e.tmp_1
        mv tmp_2 ${baseline_2}_e.tmp_2
        mv tmp_3 ${baseline_3}_e.tmp_3

        PROCESS_DAP4_RESPONSE([${baseline_1}_e.tmp_1])
        PROCESS_DAP4_RESPONSE([${baseline_2}_e.tmp_2])
        PROCESS_DAP4_RESPONSE([${baseline_3}_e.tmp_3])

        AT_CHECK([mv stderr ${baseline_1}_e.debug.tmp])
        ],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -c $abs_builddir/$bes_conf -i $input_1 -i $input_2 -i $input_3], [], [stdout], [stderr])
        # Split the response into separate files
        AT_CHECK([$response_split -i stdout -o response])

        PROCESS_DAP4_RESPONSE([response_1])
        PROCESS_DAP4_RESPONSE([response_2])
        PROCESS_DAP4_RESPONSE([response_3])

        # Check the individual responses
        AT_CHECK([diff -b -B response_1 ${baseline_1}_e_1])
        AT_CHECK([diff -b -B response_2 ${baseline_2}_e_2])
        AT_CHECK([diff -b -B response_3 ${baseline_3}_e_3])

        # Check that caching was used
        # NB: 'miss' will be 1 for dmr, but two for dds or das.
        AT_CHECK([test $(grep -c 'Cache miss' stderr) -le 2])
        AT_CHECK([test $(grep -c 'Cache hit' stderr) -eq 2])
        ])

    AT_CLEANUP
])


m4_define([PROCESS_DAP4_RESPONSE], [dnl
    PRINT_DAP4_DATA_RESPONSE([$1])
    REMOVE_DAP4_CHECKSUM([$1])
    REMOVE_DATE_TIME([$1])
    REMOVE_VERSIONS([$1])
])

# @brief Run several commands that produce a DAP2 data response
#
# Test several responses from a single call to besstandalone. This macro
# runs the same bes command file three times. jhrg 9/8/23
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass

m4_define([AT_BINARY_DAP2_MULTI_RESPONSE_TEST],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd data dap2 DAP2])

    # Don't know why this has to be here, inside a macro and not at the top level. jhrg 9/8/23
    response_split=$top_srcdir/standalone/response-split.py

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline
    AT_XFAIL_IF([test z$2 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone -d cerr,dmrpp:cache $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -r 3 -c $abs_builddir/$bes_conf -i $input], [], [stdout], [stderr])
        # Split the response into separate files
        AT_CHECK([$response_split -i stdout -o ${baseline}_r.tmp])

        PROCESS_DAP2_RESPONSE([${baseline}_r.tmp_1])
        PROCESS_DAP2_RESPONSE([${baseline}_r.tmp_2])
        PROCESS_DAP2_RESPONSE([${baseline}_r.tmp_3])

        AT_CHECK([mv stderr ${baseline}_r.debug.tmp])
        ],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -r 3 -c $abs_builddir/$bes_conf -i $input], [], [stdout], [stderr])
        # Split the response into separate files
        AT_CHECK([$response_split -i stdout -o response])

        PROCESS_DAP2_RESPONSE([response_1])
        PROCESS_DAP2_RESPONSE([response_2])
        PROCESS_DAP2_RESPONSE([response_3])

        # Check the individual responses
        AT_CHECK([diff -b -B response_1 ${baseline}_r_1])
        AT_CHECK([diff -b -B response_2 ${baseline}_r_2])
        AT_CHECK([diff -b -B response_3 ${baseline}_r_3])

        # Check that caching was used
        # NB: 'miss' will be 1 for dmr, but two for dds or das.
        AT_CHECK([test $(grep -c 'Cache miss' stderr) -le 2])
        AT_CHECK([test $(grep -c 'Cache hit' stderr) -eq 2])
        ])

    AT_CLEANUP
])

m4_define([PRINT_DAP2_DATA_RESPONSE], [dnl
    getdap -M -s $1 > $1.txt
    mv $1.txt $1
])

m4_define([PROCESS_DAP2_RESPONSE], [dnl
    PRINT_DAP2_DATA_RESPONSE([$1])
    REMOVE_DATE_TIME([$1])
    REMOVE_VERSIONS([$1])
])
