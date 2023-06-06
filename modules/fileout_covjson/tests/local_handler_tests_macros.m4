#

# This new macro expects that the bes.conf file used with besstandalone will be
# the second argument. This provides a more compact way to run tests with several
# different bes.conf files. Borrowed from the fileout netCDF module
#
# Usage: AT_BESCMD_BESCONF_RESPONSE_TEST([<bescmd file>], [<bes.conf>], [pass|xfail], [repeat|cached])
# The last two params are optional.
m4_define([AT_BESCMD_BESCONF_RESPONSE_TEST], [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([bescmd])

    input=$abs_srcdir/$1

    # Here the bes_conf var is set using parameter number 2. This shadows the
    # value that can be set using the optional -c (--conf) argument (see the top
    # of this file). We might improve on this! jhrg 3/11/22
    bes_conf=$abs_builddir/$2

    # The baseline needs to contain something to tie it to the bes conf file since
    # the same bescmd file may produce different output with a different bes conf.
    baseline=$abs_srcdir/$1.$2.baseline

    # Oddly, setting 'pass' to $3 and then using $pass in AT_XFAIL_IF() does not work,
    # but using $3 does. This might be a function of when the AT_XFAIL_IF() macro is
    # expanded. jhrg 3.20.20
    pass=$3
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
        AT_XFAIL_IF([test z$3 = zxfail])
        ])

    AT_CLEANUP
])

