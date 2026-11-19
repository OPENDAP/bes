#
# Autotest macros for the GeotiffToDmr command-line tool (../GeotiffToDmr.cc).
#
# These follow the conventions of modules/common/handler_tests_macros.m4 and
# must be included after it (they use REMOVE_VERSIONS and the $baselines and
# $bes_conf variables it sets up):
#
#     m4_include(AT_TOP_SRCDIR/modules/common/handler_tests_macros.m4)
#     m4_include([GeotiffToDmr_macros.m4])
#
# GeotiffToDmr is run from the build tree against the besstandalone and BES
# modules built in this tree. It is given the testsuite's bes.conf with
# --bes-conf, so the module paths come from bes.conf.in; GeotiffToDmr overrides
# only the catalog root and the TypeMatch rule.
#
# Inputs are named relative to modules/gdal_module (e.g. data/cea.tif) and
# baselines relative to this directory (e.g. GeotiffToDmr/SP27GTIF.TIF.dmr.baseline).
# Run the testsuite with --baselines=yes to write <baseline>.tmp files.

AT_TESTED([besstandalone])

# _AT_GEOTIFFTODMR_VARS: shell variables every GeotiffToDmr test uses.
m4_define([_AT_GEOTIFFTODMR_VARS], [dnl
    GeotiffToDmr=$abs_top_builddir/modules/gdal_module/GeotiffToDmr
    gtd_bes="--besstandalone $abs_top_builddir/standalone/besstandalone --bes-conf $abs_builddir/$bes_conf"
    gtd_data=$abs_top_srcdir/modules/gdal_module/data
])

# GEOTIFFTODMR_COPY_DATA(<file in modules/gdal_module/data>, [<new name>])
#
# Copy a data file into the test's working directory (writable, unlike srcdir).
m4_define([GEOTIFFTODMR_COPY_DATA], [dnl
    AT_CHECK([cp $gtd_data/$1 ./m4_default([$2], [$1])])
])

# GEOTIFFTODMR_MAKE_PLAIN_TIFF(<file>)
#
# Write a valid 1x1 8-bit TIFF that has no geotransform and no CRS.
m4_define([GEOTIFFTODMR_MAKE_PLAIN_TIFF], [dnl
    AT_CHECK([printf '\111\111\052\000\010\000\000\000\011\000\000\001\003\000\001\000\000\000\001\000\000\000\001\001\003\000\001\000\000\000\001\000\000\000\002\001\003\000\001\000\000\000\010\000\000\000\003\001\003\000\001\000\000\000\001\000\000\000\006\001\003\000\001\000\000\000\001\000\000\000\021\001\004\000\001\000\000\000\172\000\000\000\025\001\003\000\001\000\000\000\001\000\000\000\026\001\003\000\001\000\000\000\001\000\000\000\027\001\004\000\001\000\000\000\001\000\000\000\000\000\000\000\007' > $1])
])

# AT_GEOTIFFTODMR_RESPONSE_TEST(<input>, <baseline>, [pass|xfail])
#
# Run 'GeotiffToDmr -o - <input>' and diff the DMR against <baseline>. The
# baseline can be one of the handler's own gdal/*.dmr.bescmd.baseline files:
# GeotiffToDmr must return exactly what besstandalone returns for that file.
m4_define([AT_GEOTIFFTODMR_RESPONSE_TEST], [dnl

    AT_SETUP([GeotiffToDmr $1])
    AT_KEYWORDS([GeotiffToDmr response])

    _AT_GEOTIFFTODMR_VARS
    input=$abs_top_srcdir/modules/gdal_module/$1
    baseline=$abs_srcdir/$2
    AT_XFAIL_IF([test z$3 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: GeotiffToDmr -o - $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([$GeotiffToDmr $gtd_bes -o - $input], [0], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([$GeotiffToDmr $gtd_bes -o - $input], [0], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])

# AT_GEOTIFFTODMR_RESPONSE_PATTERN_TEST(<input>, <baseline>, [pass|xfail])
#
# Like AT_GEOTIFFTODMR_RESPONSE_TEST, but every line of <baseline> is a grep
# pattern (see gdal/README: spatial_ref floats vary with the GDAL version).
m4_define([AT_GEOTIFFTODMR_RESPONSE_PATTERN_TEST], [dnl

    AT_SETUP([GeotiffToDmr $1 (pattern)])
    AT_KEYWORDS([GeotiffToDmr response pattern])

    _AT_GEOTIFFTODMR_VARS
    input=$abs_top_srcdir/modules/gdal_module/$1
    baseline=$abs_srcdir/$2
    AT_XFAIL_IF([test z$3 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: GeotiffToDmr -o - $1"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
        [
        AT_CHECK([$GeotiffToDmr $gtd_bes -o - $input], [0], [stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([$GeotiffToDmr $gtd_bes -o - $input], [0], [stdout])
        AT_CHECK([grep -f $baseline stdout], [0], [ignore])
        ])

    AT_CLEANUP
])

# AT_GEOTIFFTODMR_BESCMD_COMPARE_TEST(<bescmd>, <input>, [pass|xfail])
#
# Run besstandalone on <bescmd> and GeotiffToDmr on <input> (the same file) and
# require identical DMRs. Needs no baseline, so it keeps passing when a GDAL
# upgrade changes the handler's output.
m4_define([AT_GEOTIFFTODMR_BESCMD_COMPARE_TEST], [dnl

    AT_SETUP([GeotiffToDmr $2 matches $1])
    AT_KEYWORDS([GeotiffToDmr compare])

    _AT_GEOTIFFTODMR_VARS
    bescmd=$abs_srcdir/$1
    input=$abs_top_srcdir/modules/gdal_module/$2
    AT_XFAIL_IF([test z$3 = zxfail])

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: besstandalone -i $1; GeotiffToDmr -o - $2"])

    AT_CHECK([besstandalone -c $abs_builddir/$bes_conf -i $bescmd > expected], [0])
    AT_CHECK([$GeotiffToDmr $gtd_bes -o - $input], [0], [stdout])
    AT_CHECK([diff -b -B expected stdout])

    AT_CLEANUP
])

# AT_GEOTIFFTODMR_ERROR_TEST(<title>, <setup>, <arguments>, <exit status>, <message>, [<checks>])
#
# Run <setup> (shell commands or GEOTIFFTODMR_* macros; may be empty), then
# 'GeotiffToDmr <arguments>' in the test's working directory. Require
# <exit status> (1 bad input/usage, 2 BES error, 3 timeout, 4 internal) and
# that stderr contains the fixed string <message>, then run <checks>.
# Arguments are passed as written: add $gtd_bes yourself when the run needs
# the BES.
m4_define([AT_GEOTIFFTODMR_ERROR_TEST], [dnl

    AT_SETUP([GeotiffToDmr error: $1])
    AT_KEYWORDS([GeotiffToDmr error])

    _AT_GEOTIFFTODMR_VARS
    $2

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: GeotiffToDmr $3"])

    AT_CHECK([$GeotiffToDmr $3], [$4], [ignore], [stderr])
    AT_CHECK([grep -F -e "$5" stderr], [0], [ignore])
    $6

    AT_CLEANUP
])

# AT_GEOTIFFTODMR_OUTPUT_TEST(<title>, <setup>, <arguments>, <exit status>, <dmr file>, [<checks>])
#
# Run <setup>, then 'GeotiffToDmr <arguments>'; require <exit status>, require
# that <dmr file> was written and holds a DAP4 DMR, then run <checks>.
m4_define([AT_GEOTIFFTODMR_OUTPUT_TEST], [dnl

    AT_SETUP([GeotiffToDmr output: $1])
    AT_KEYWORDS([GeotiffToDmr output])

    _AT_GEOTIFFTODMR_VARS
    $2

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: GeotiffToDmr $3"])

    AT_CHECK([$GeotiffToDmr $3], [$4], [ignore], [stderr])
    AT_CHECK([test -s $5])
    AT_CHECK([grep -F -e 'xmlns="http://xml.opendap.org/ns/DAP/4.0#"' $5], [0], [ignore])
    AT_CHECK([tail -n 1 $5 | grep -F -e '</Dataset>'], [0], [ignore])
    $6

    AT_CLEANUP
])

# AT_GEOTIFFTODMR_STDOUT_TEST(<title>, <arguments>, <exit status>, <message>)
#
# Run 'GeotiffToDmr <arguments>'; require <exit status> and that stdout contains
# the fixed string <message> (for --help and --version).
m4_define([AT_GEOTIFFTODMR_STDOUT_TEST], [dnl

    AT_SETUP([GeotiffToDmr: $1])
    AT_KEYWORDS([GeotiffToDmr usage])

    _AT_GEOTIFFTODMR_VARS

    AT_CHECK([$GeotiffToDmr $2], [$3], [stdout], [ignore])
    AT_CHECK([grep -F -e "$4" stdout], [0], [ignore])

    AT_CLEANUP
])
