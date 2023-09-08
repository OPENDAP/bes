
# These macros are specific to the DMR++ handler for now. jhrg 9/8/23

# NB: AT_CHECK (commands, [status = ‘0’], [stdout], [stderr], [run-if-fail], [run-if-pass])

# @brief Run the given bes command file
#
# Test several responses from a single call to besstandalone. This macro
# runs the same bes command file three times. jhrg 9/8/23
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass
# @param $3 If 'repeat' or 'cached', run besstandalone using '-r 3'

m4_define([AT_BESCMD_MULTI_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd])

    # Don't know why this has to be here, inside a macro and not at the top level. jhrg 9/8/23
    response_split=$top_srcdir/standalone/response-split.py

    input=$abs_srcdir/$1
    baseline=$abs_srcdir/$1.baseline

    # AT_XFAIL_IF is always run first regardless where it appears. jhrg 5/27/22
    AT_XFAIL_IF([test z$2 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone $repeat -c $bes_conf -i $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([besstandalone -d cerr,dmrpp:cache -r 3 -c $abs_builddir/$bes_conf -i $input], [], [stdout], [stderr])
        # Split the response into separate files
        AT_CHECK([$response_split -i stdout -o ${baseline}_r.tmp])
        AT_CHECK([mv stderr ${baseline}_r.debug.tmp])
        ],
        [
        AT_CHECK([besstandalone -r 3 -c $abs_builddir/$bes_conf -i $input], [], [stdout])
        AT_CHECK([$response_split -i stdout -o response])

        # Check the individual responses
        AT_CHECK([diff -b -B response_1 ${baseline}_r_1])
        AT_CHECK([diff -b -B response_2 ${baseline}_r_2])
        AT_CHECK([diff -b -B response_3 ${baseline}_r_3])

        # Check that caching was used
        # NB: 'miss' will be 1 for dmr, but two for dds or das.
        AT_CHECK([test $(grep -c 'Cache miss' ${baseline}_r.debug) -le 2])
        AT_CHECK([test $(grep -c 'Cache hit' ${baseline}_r.debug) -eq 2])
        ])

    AT_CLEANUP
])

