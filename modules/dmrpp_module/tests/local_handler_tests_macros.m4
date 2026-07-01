#
# Additional macros for the dmrpp tests. 

# The macro to test some options of the build_dmrpp_h4 utility. Currently it is used to test the -M option.
m4_define([AT_BUILD_DMRPP_H4_TEST_OPTIONS],  [dnl

    AT_SETUP([$1 $2])
    AT_KEYWORDS([build_dmrpp dmrpp data dap4 DAP4])

    input="${abs_srcdir}/$1"
    input2=$abs_srcdir/$1.dmr
    option=$2
    baseline=$abs_srcdir/$1.${option}.dmr.baseline

    AT_XFAIL_IF([test z$3 = zxfail])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
    [
        AT_CHECK([$abs_builddir/../build_dmrpp_h4/build_dmrpp_h4 -f $input -r $input2 -${option}], [], [stdout])
        REMOVE_VERSIONS([stdout])
        REMOVE_DATE_TIME([stdout])
        REMOVE_BUILD_DMRPP_H4_INVOCATION_ATTR([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([$abs_builddir/../build_dmrpp_h4/build_dmrpp_h4 -f $input -r $input2 -${option}], [], [stdout])
        REMOVE_VERSIONS([stdout])
        REMOVE_DATE_TIME([stdout])
        REMOVE_BUILD_DMRPP_H4_INVOCATION_ATTR([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])


# Replace the DAP Attribute named "invocation"; adapted from the build_dmrpp tests. 
# Currently it is only used to test the -M option. Note: the -M</Value>
m4_define([REMOVE_BUILD_DMRPP_H4_INVOCATION_ATTR], [dnl
    # This sed magic: '1h;2,$H;$!d;g' slurps up the entire file into a single line.
    # Courtesy of: https://unix.stackexchange.com/users/21763/antak
    #   Reference: https://unix.stackexchange.com/questions/26284/how-can-i-use-sed-to-replace-a-multi-line-string
    sed \
        -e '1h;2,$H;$!d;g' \
        -e 's@<Attribute name="invocation" type="String">.*-M</Value>@<Attribute name="Removed(invocation)">@' \
         < $1 > $1.sed
    mv $1.sed $1
])

