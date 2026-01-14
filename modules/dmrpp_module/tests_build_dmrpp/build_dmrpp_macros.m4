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

AT_ARG_OPTION_ARG([s3tests],
    [--s3tests=yes|no   Run the tests that read/write to the opendap.tests S3 bucket (creds required)],
    [echo "s3tests set to $at_arg_s3tests"; s3tests=$at_arg_s3tests],
    [s3tests=no])

# Usage: _AT_TEST_*(<bescmd source>, <baseline file>, <xpass/xfail> [default is xpass] <repeat|cached> [default is no])

# @brief Run the given bes command file
#
# @param $1 The command file, assumes that the baseline is $1.baseline
# @param $2 If not null, 'xfail' means the test is expected to fail, 'xpass' ... pass
# @param $3 If 'repeat' or 'cached', run besstandalone using '-r 3'

m4_define([AT_BUILD_DMRPP],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([build_dmrpp dmrpp data dap4 DAP4])

    input="${abs_top_srcdir}/$1"
    AT_XFAIL_IF([test z$2 = zxfail])
    repeat=$3

    dmr="${input}.dmr"
    baseline="${input}.dmrpp.baseline"

    build_dmrpp_app="${abs_top_builddir}/modules/dmrpp_module/build_dmrpp"
    build_dmrpp_cmd="${build_dmrpp_app} -f ${input} -r ${dmr}"

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: ${build_dmrpp_cmd}"])


    AS_IF([test -n "$baselines" -a x$baselines = xyes],
    [
        AT_CHECK([${build_dmrpp_cmd}], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([${build_dmrpp_cmd}], [], [stdout])
        REMOVE_VERSIONS([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])

m4_define([AT_BUILD_DMRPP_M],  [dnl

    AT_SETUP([$1])
    AT_KEYWORDS([build_dmrpp dmrpp data dap4 DAP4])

    input="${abs_top_srcdir}/$1"
    AT_XFAIL_IF([test z$2 = zxfail])

    dmr="${input}.dmr"
    baseline="${input}.dmrpp.M.baseline"

    build_dmrpp_app="${abs_top_builddir}/modules/dmrpp_module/build_dmrpp"
    build_dmrpp_cmd="${build_dmrpp_app} -M -f ${input} -r ${dmr}"

    AS_IF([test -z "$at_verbose"], [echo "COMMAND: ${build_dmrpp_cmd}"])

    AS_IF([test -n "$baselines" -a x$baselines = xyes],
    [
        AT_CHECK([${build_dmrpp_cmd}], [], [stdout])
        NORMALIZE_EXEC_NAME([stdout])
        REMOVE_PATH_COMPONENTS([stdout])
        REMOVE_VERSIONS([stdout])
        REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([stdout])
        AT_CHECK([mv stdout $baseline.tmp])
        ],
        [
        AT_CHECK([${build_dmrpp_cmd}], [], [stdout])
        NORMALIZE_EXEC_NAME([stdout])
        REMOVE_PATH_COMPONENTS([stdout])
        REMOVE_VERSIONS([stdout])
        REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([stdout])
        AT_CHECK([diff -b -B $baseline stdout])
        ])

    AT_CLEANUP
])


dnl Remove path components of DAP DMR Attributes that may vary with builds.
dml jhrg 11/22/21
dnl Usage: REMOVE_PATH_COMPONENTS(file_name)
m4_define([REMOVE_PATH_COMPONENTS], [dnl
    sed -e 's@/[[A-z0-9]][[-A-z0-9_/.]]*/dmrpp_module/@/path_removed/@g' \
        -e 's@/[[A-z0-9][-A-z0-9_/.]]*\.so@< library_path_removed >@g' \
        -e 's@BES.Catalog.catalog.RootDirectory=/[[A-z0-9][-A-z0-9_/.]]*@BES.Catalog.catalog.RootDirectory=< path_removed >@g' < $1 > $1.sed
    mv $1.sed $1
])

dnl Normalize binary name. Sometimes the build_dmrpp program is named 'build_dmrpp,'
dnl other times it is named 'lt-build_dmrpp.' This macro ensures it always has the 
dnl same name in the baselines and test output.
dml jhrg 11/22/21
dnl Usage: NORMALIZE_EXEC_NAME(file_name)
m4_define([NORMALIZE_EXEC_NAME], [dnl
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
dnl Supports versions with build number suffix, e.g., both 3.20.8 and 3.20.8-123
dnl
dnl Note that the macro depends on the baseline being a file.
dnl
dnl jhrg 12/29/21

m4_define([REMOVE_VERSIONS], [dnl
  awk '{
    gsub(/<Value>[[0-9]+]\.[[0-9]+]\.[[0-9]+](-[[0-9]+])?<\/Value>/, "<Value>removed version</Value>");
    gsub(/<Value>[[a-zA-Z._]+]-[[0-9]+]\.[[0-9]+]\.[[0-9]+](-[[0-9]+])?<\/Value>/, "<Value>removed version</Value>");
    gsub(/dmrpp:version="[[0-9]+]\.[[0-9]+]\.[[0-9]+](-[[0-9]+])?"/, "removed dmrpp:version");
    print
  }' < $1 > $1.awk
  mv $1.awk $1
])

# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
#
# GET_DMRPP_3_20
#
# Usage for get_dmrpp commandline:
#
# get_dmrpp -h
#
# [get_dmrpp-3.20.10]
#
# Usage: /Users/ndp/OPeNDAP/hyrax/build/bin/get_dmrpp [options] <hdf5 file>
#
# Write the DMR++ for hdf5_file to stdout
#
# By default the BES Data Root directory is set to /tmp. This
# utility will add an entry to the bes.log specified in the
# configuration file. The DMR++ is built using the DMR as returned
# by the HDF5 handler, using options as set in the bes
# configuration file found here.
#
# -h: Show help
# -z: Show version information (Verbose got here first.)
# -v: Verbose: Print the DMR too
# -V: Very Verbose: print the DMR, the command and the configuration
#     file used to build the DMR, and do not remove temporary files.
# -D: Just print the DMR that will be used to build the DMR++
# -u: The binary object URL for use in the DMR++ file. If option '-u' is
#     not used; then dap4:Dataset/@dmrpp:href attribute will contain the template string
#     OPeNDAP_DMRpp_DATA_ACCESS_URL which can be replaced at runtime.
# -b: The fully qualified path to the BES_DATA_ROOT directory. May not be "/" or "/etc".
#     The default value is /tmp if a value is not provided
# -c: The path to the bes configuration file to use.
# -s: The path to an optional addendum configuration file which will be appended to the
#     default BES configuration. Much like the site.conf file works for the full server
#     deployment it will be loaded last and the settings there-in will have an override
#     effect on the default configuration.
# -o: The name of the dmr++ file to create.
# -e: The name of pre-existing dmr++ file to test.
# -T: Run ALL hyrax tests on the resulting dmr++ file and compare the responses
#     the ones generated by the source hdf5 file.
# -I: Run hyrax inventory tests on the resulting dmr++ file and compare the responses
#     the ones generated by the source hdf5 file.
# -F: Run hyrax value probe tests on the resulting dmr++ file and compare the responses
#     the ones generated by the source hdf5 file.
# -M: Create and merge missing CF coordinate domain variables into the dmrpp. If there are
#     missing variables, a sidecar file named <input_file_name>.missing will be created
#     in the same directory location as the input_data_file.
#     If option 'p' is not used; missing variable chunk href will contain OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL.
#     If option 'p' is selected; missing variable chunk href will contain the argument provided to that option
# -p: The value to use for each missing variable's dmrpp:chunk/@dmrpp:href attribute.  If option '-p' is
#     not used; the missing variable dmrpp:chunk/@dmrpp:href attributes will contain the template string
#     OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL which can be replaced at runtime.
# -r: The path to the file that contains missing variable information for sets of input data files that share
#     common missing variables. The file will be created if it doesn't exist and the result may be used in subsequent
#     invocations of get_dmrpp (using -r) to identify the missing variable file.
# -U: If present, and if the input data file is an AWS S3 URL (s3://...), and if the output data file has been set (using
#     the -o option) the presence of this parameter will cause get_dmrpp to copy the finished dmr++ file to the same S3
#     bucket location as the input data file.
#
# Limitations:
# * The name of the hdf5 file must be expressed relative to the BES_DATA_ROOT, or as an S3 URL (s3://...)
#


##########################################################################################
# AT_GET_DMRPP_3_20()
#
#
m4_define([AT_GET_DMRPP_3_20],  [dnl

AT_SETUP([get_dmrpp $1])
AT_KEYWORDS([get_dmrpp data dap4 DAP4])

GET_DMRPP="${abs_top_builddir}/modules/dmrpp_module/data/get_dmrpp"
chmod +x "${GET_DMRPP}"
ls -l "${GET_DMRPP}"

DATA_DIR="modules/dmrpp_module/data/dmrpp"
BASELINES_DIR="${abs_srcdir}/get_dmrpp_baselines"
BES_DATA_ROOT=$(readlink -f "${abs_top_srcdir}")

test_name=$1
input_file=$2
baseline=${BASELINES_DIR}/$3
params="$4"
output_file="$5"
AT_XFAIL_IF([test z$6 = zxfail]) # This is always run FIRST

echo "${input_file}" | grep "s3://"
if test $? -eq 0
then
    # We're here because it's an S3 Test
    # Only run the S3 tests if specifically instructed to do so.
    AT_SKIP_IF([test x$s3tests = xno])
else
    # It's a file test so we need to amend the input_file
    # name to reference the correct thing.
    input_file="${DATA_DIR}/${input_file}"
fi

if test -n "${output_file}"
then
    params="${params} -o ${output_file}"
else
    output_file=stdout
fi

# Amend the PATH to pick up besstandalone
export PATH=${abs_top_builddir}/standalone:$PATH

gd_verbose=""
AS_IF([test -z "$at_verbose"], [ gd_verbose="-v -X" ])

TEST_CMD="${GET_DMRPP} -A -b ${BES_DATA_ROOT} ${params} ${gd_verbose} ${input_file}"

# at_verbose=""

AS_IF([test -z "$at_verbose"], [
    echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    echo "#   abs_top_srcdir: '${abs_top_srcdir}'"
    echo "#       abs_srcdir: '${abs_srcdir}'"
    echo "#       top_srcdir: '${top_srcdir}'"
    echo "#         builddir: '${builddir}'"
    echo "#     abs_builddir: '${abs_builddir}'"
    echo "#     top_builddir: '${top_builddir}'"
    echo "# top_build_prefix: '${top_build_prefix}'"
    echo "# abs_top_builddir: '${abs_top_builddir}'"
    echo "#        GET_DMRPP: '${GET_DMRPP}'"
    echo "#    BES_DATA_ROOT: '${BES_DATA_ROOT}'"
    echo "#         DATA_DIR: '${DATA_DIR}'"
    echo "#    BASELINES_DIR: '${BASELINES_DIR}'"
    echo "#"
    echo "# AT_GET_DMRPP_3_20() arguments: "
    echo "#           arg #1: '${1}'"
    echo "#           arg #2: '${2}'"
    echo "#           arg #3: '${3}'"
    echo "#           arg #4: '${4}'"
    echo "#           arg #5: '${5}'"
    echo "#           arg #6: '${6}'"
    echo "#        test_name: '${test_name}'"
    echo "#       input_file: '${input_file}'"
    echo "#         baseline: '${baseline}'"
    echo "#      xfail_param: '${xfail_param}'"
    echo "#           params: '${params}'"
    echo "#      output_file: '${output_file}'"
    echo "#       gd_verbose: '${gd_verbose}'"
    echo "#         TEST_CMD: '${TEST_CMD}'"
])

AS_IF([test -n "$baselines" -a x$baselines = xyes],
[
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Calling get_dmrpp application."])
    AT_CHECK([${TEST_CMD}], [], [stdout], [stderr])

    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines - output_file: ${output_file}"])

    NORMALIZE_EXEC_NAME([${output_file}])
    REMOVE_PATH_COMPONENTS([${output_file}])
    REMOVE_VERSIONS([${output_file}])
    REMOVE_BUILD_DMRPP_INVOCATION_ATTR([${output_file}])
    REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([${output_file}])
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Copying result to ${baseline}.tmp"])
    AT_CHECK([mv ${output_file} ${baseline}.tmp])
],
[
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp: Calling get_dmrpp application."])
    AT_CHECK([${TEST_CMD}], [], [stdout], [stderr])
    NORMALIZE_EXEC_NAME([${output_file}])
    REMOVE_PATH_COMPONENTS([${output_file}])
    REMOVE_VERSIONS([${output_file}])
    REMOVE_BUILD_DMRPP_INVOCATION_ATTR([${output_file}])
    REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([${output_file}])
    AS_IF([test -z "$at_verbose"], [
        echo ""
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
        echo "# get_dmrpp: Filtered ${output_file} BEGIN"
        echo "#"
        cat ${output_file};
        echo "#"
        echo "# get_dmrpp: Filtered ${output_file} END"
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    ])
    AT_CHECK([diff -b -B ${baseline} ${output_file}])
])

AT_CLEANUP
])

##########################################################################################
# AT_GET_DMRPP_3_20_MAKE_MISSING()
#
#
m4_define([AT_GET_DMRPP_3_20_MAKE_MISSING],  [dnl

AT_SETUP([get_dmrpp $1])
AT_KEYWORDS([get_dmrpp data dap4 DAP4])

GET_DMRPP="${abs_top_builddir}/modules/dmrpp_module/data/get_dmrpp"
chmod +x "${GET_DMRPP}"
ls -l "${GET_DMRPP}"

BASELINES_DIR="${abs_srcdir}/get_dmrpp_baselines"
BES_DATA_ROOT=$(readlink -f "${abs_builddir}")

test_name="$1"
input_file="$2"
dmrpp_baseline="${BASELINES_DIR}/$3"
missing_baseline="${BASELINES_DIR}/$4"
params="$5"
output_file="$6"
AT_XFAIL_IF([test z$7 = zxfail]) # This is always run FIRST

# Amend the PATH to pick up besstandalone
export PATH=${abs_top_builddir}/standalone:$PATH

missing_data_file="${input_file}.missing"

echo "${input_file}" | grep "s3://"
if test $? -eq 0
then
    # We're here because it's an S3 Test
    # Only run the S3 tests if specifically instructed to do so.
    AT_SKIP_IF([test x$s3tests = xno])
    # We reset the missing data file name because we know that
    # the $input_file is an S3 URL
    missing_data_file=$(basename "${input_file}").missing
fi

AT_CHECK([test -n "${output_file}"])
params="${params} -o ${output_file}"

gd_verbose=""
AS_IF([test -z "$at_verbose"], [ gd_verbose="-v -X" ])

TEST_CMD="${GET_DMRPP} -A -b ${BES_DATA_ROOT} -M ${params} ${gd_verbose} ${input_file}"

# at_verbose=""

AS_IF([test -z "$at_verbose"], [
    echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    echo "#   abs_top_srcdir: ${abs_top_srcdir}"
    echo "#       abs_srcdir: ${abs_srcdir}"
    echo "#       top_srcdir: ${top_srcdir}"
    echo "#         builddir: ${builddir}"
    echo "#     abs_builddir: ${abs_builddir}"
    echo "#     top_builddir: ${top_builddir}"
    echo "# top_build_prefix: ${top_build_prefix}"
    echo "# abs_top_builddir: ${abs_top_builddir}"
    echo "#        GET_DMRPP: ${GET_DMRPP}"
    echo "#    BES_DATA_ROOT: ${BES_DATA_ROOT}"
    echo "#    BASELINES_DIR: ${BASELINES_DIR}"
    echo "#"
    echo "# AT_GET_DMRPP_3_20() arguments: "
    echo "#            arg #1: "$1
    echo "#            arg #2: "$2
    echo "#            arg #3: "$3
    echo "#            arg #4: "$4
    echo "#            arg #5: "$5
    echo "#            arg #6: "$6
    echo "#            arg #7: "$7
    echo "#          test_name: ${test_name}"
    echo "#        input_file: ${input_file}"
    echo "#    dmrpp_baseline: ${dmrpp_baseline}"
    echo "#  missing_baseline: ${missing_baseline}"
    echo "#            params: ${params}"
    echo "#       output_file: ${output_file}"
    echo "#       xfail_param: ${xfail_param}"
    echo "# missing_data_file: ${missing_data_file}"
    echo "#       gd_verbose: ${gd_verbose}"
    echo "#          TEST_CMD: ${TEST_CMD}"
])

AS_IF([test -n "$baselines" -a x$baselines = xyes],
[
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Calling get_dmrpp application."])
    AT_CHECK([${TEST_CMD}], [], [stdout], [stderr])
    NORMALIZE_EXEC_NAME([${output_file}])
    REMOVE_PATH_COMPONENTS([${output_file}])
    REMOVE_VERSIONS([${output_file}])
    REMOVE_BUILD_DMRPP_INVOCATION_ATTR([${output_file}])
    REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([${output_file}])
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Copying missing data dmrpp result to ${dmrpp_baseline}.tmp"])
    AT_CHECK([mv ${output_file} ${dmrpp_baseline}.tmp])
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Copying missing data result to ${missing_baseline}.missing.tmp"])
    AT_CHECK([ncdump ${abs_builddir}/${missing_data_file} > ${missing_baseline}.missing.tmp])
    rm -f ${abs_builddir}/${missing_data_file}
],
[
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp: Calling get_dmrpp application."])
    AT_CHECK([${TEST_CMD}], [], [stdout], [stderr])
    NORMALIZE_EXEC_NAME([${output_file}])
    REMOVE_PATH_COMPONENTS([${output_file}])
    REMOVE_VERSIONS([${output_file}])
    REMOVE_BUILD_DMRPP_INVOCATION_ATTR([${output_file}])
    REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([${output_file}])
    AS_IF([test -z "$at_verbose"], [
        echo ""
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
        echo "# get_dmrpp: Filtered ${output_file} BEGIN"
        echo "#"
        cat ${output_file};
        echo "#"
        echo "# get_dmrpp: Filtered ${output_file} END"
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    ])
    AT_CHECK([diff -b -B ${dmrpp_baseline} ${output_file}])

    AT_CHECK([ncdump ${abs_builddir}/${missing_data_file} > tmp])
    AS_IF([test -z "$at_verbose"], [
        echo ""
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
        echo "# get_dmrpp: missing data file BEGIN"
        echo "#"
        cat tmp;
        echo "#"
        echo "# get_dmrpp: missing data file END"
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    ])
    AT_CHECK([diff -b -B ${missing_baseline} tmp])
    rm -f ${abs_builddir}/${missing_data_file}
])

AT_CLEANUP
])

##########################################################################################
# AT_GET_DMRPP_3_20_MAKE_MISSING_GROUP()
#
#
m4_define([AT_GET_DMRPP_3_20_MAKE_MISSING_GROUP],  [dnl

AT_SETUP([get_dmrpp $1])
AT_KEYWORDS([get_dmrpp data dap4 DAP4])

GET_DMRPP="${abs_top_builddir}/modules/dmrpp_module/data/get_dmrpp"
chmod +x "${GET_DMRPP}"
ls -l "${GET_DMRPP}"

BASELINES_DIR="${abs_srcdir}/get_dmrpp_baselines"
BES_DATA_ROOT=$(readlink -f "${abs_builddir}")

test_name="$1"
input_file="$2"
dmrpp_baseline="${BASELINES_DIR}/$3"
missing_baseline="${BASELINES_DIR}/$4"
output_file="$5"
AT_XFAIL_IF([test z$6 = zxfail]) # This is always run FIRST

# Amend the PATH to pick up besstandalone
export PATH=${abs_top_builddir}/standalone:$PATH

missing_data_file="${input_file}.missing"

echo "${input_file}" | grep "s3://"
if test $? -eq 0
then
    # We're here because it's an S3 Test
    # Only run the S3 tests if specifically instructed to do so.
    AT_SKIP_IF([test x$s3tests = xno])
    # We reset the missing data file name because we know that
    # the $input_file is an S3 URL
    missing_data_file=$(basename "${input_file}").missing
fi

AT_CHECK([test -n "${output_file}"])
#Note: the input params is not used for this version.
params="-o ${output_file}"

gd_verbose=""
AS_IF([test -z "$at_verbose"], [ gd_verbose="-v -X" ])

TEST_CMD="${GET_DMRPP} -A -b ${BES_DATA_ROOT} -m ${params} ${gd_verbose} ${input_file}"

# at_verbose=""

AS_IF([test -z "$at_verbose"], [
    echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    echo "#   abs_top_srcdir: ${abs_top_srcdir}"
    echo "#       abs_srcdir: ${abs_srcdir}"
    echo "#       top_srcdir: ${top_srcdir}"
    echo "#         builddir: ${builddir}"
    echo "#     abs_builddir: ${abs_builddir}"
    echo "#     top_builddir: ${top_builddir}"
    echo "# top_build_prefix: ${top_build_prefix}"
    echo "# abs_top_builddir: ${abs_top_builddir}"
    echo "#        GET_DMRPP: ${GET_DMRPP}"
    echo "#    BES_DATA_ROOT: ${BES_DATA_ROOT}"
    echo "#    BASELINES_DIR: ${BASELINES_DIR}"
    echo "#"
    echo "# AT_GET_DMRPP_3_20() arguments: "
    echo "#            arg #1: "$1
    echo "#            arg #2: "$2
    echo "#            arg #3: "$3
    echo "#            arg #4: "$4
    echo "#            arg #5: "$5
    echo "#            arg #6: "$6
    echo "#          test_name: ${test_name}"
    echo "#        input_file: ${input_file}"
    echo "#    dmrpp_baseline: ${dmrpp_baseline}"
    echo "#  missing_baseline: ${missing_baseline}"
    echo "#       output_file: ${output_file}"
    echo "#       xfail_param: ${xfail_param}"
    echo "# missing_data_file: ${missing_data_file}"
    echo "#       gd_verbose: ${gd_verbose}"
    echo "#          TEST_CMD: ${TEST_CMD}"
])

AS_IF([test -n "$baselines" -a x$baselines = xyes],
[
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Calling get_dmrpp application."])
    AT_CHECK([${TEST_CMD}], [], [stdout], [stderr])
    NORMALIZE_EXEC_NAME([${output_file}])
    REMOVE_PATH_COMPONENTS([${output_file}])
    REMOVE_VERSIONS([${output_file}])
    REMOVE_BUILD_DMRPP_INVOCATION_ATTR_VALUE([${output_file}])
    REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([${output_file}])
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Copying missing data dmrpp result to ${dmrpp_baseline}.tmp"])
    AT_CHECK([mv ${output_file} ${dmrpp_baseline}.tmp])
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Copying missing data result to ${missing_baseline}.missing.tmp"])
    AT_CHECK([ncdump ${abs_builddir}/${missing_data_file} > ${missing_baseline}.missing.tmp])
    rm -f ${abs_builddir}/${missing_data_file}
],
[
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp: Calling get_dmrpp application."])
    AT_CHECK([${TEST_CMD}], [], [stdout], [stderr])
    NORMALIZE_EXEC_NAME([${output_file}])
    REMOVE_PATH_COMPONENTS([${output_file}])
    REMOVE_VERSIONS([${output_file}])
     AS_IF([test -z "$at_verbose"], [
        echo ""
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
        echo "# get_dmrpp: Filtered GROUP BEFORE INV ${output_file} BEGIN"
        echo "#"
        cat ${output_file};
        echo "#"
        echo "# get_dmrpp: Filtered GROUP BEFORE INV ${output_file} END"
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    ])
    
    REMOVE_BUILD_DMRPP_INVOCATION_ATTR_VALUE([${output_file}])
    REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([${output_file}])
    AS_IF([test -z "$at_verbose"], [
        echo ""
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
        echo "# get_dmrpp: Filtered GROUP ${output_file} BEGIN"
        echo "#"
        cat ${output_file};
        echo "#"
        echo "# get_dmrpp: Filtered GROUP ${output_file} END"
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    ])
    AT_CHECK([diff -b -B ${dmrpp_baseline} ${output_file}])

    AT_CHECK([ncdump ${abs_builddir}/${missing_data_file} > tmp])
    AS_IF([test -z "$at_verbose"], [
        echo ""
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
        echo "# get_dmrpp: missing data file BEGIN"
        echo "#"
        cat tmp;
        echo "#"
        echo "# get_dmrpp: missing data file END"
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    ])
    AT_CHECK([diff -b -B ${missing_baseline} tmp])
    rm -f ${abs_builddir}/${missing_data_file}
])

AT_CLEANUP
])


##########################################################################################
# AT_GET_DMRPP_3_20_TESTS()
#
#
m4_define([AT_GET_DMRPP_3_20_TESTS],  [dnl

AT_SETUP([get_dmrpp $1])
AT_KEYWORDS([get_dmrpp data dap4 DAP4])

GET_DMRPP="${abs_top_builddir}/modules/dmrpp_module/data/get_dmrpp"
chmod +x "${GET_DMRPP}"
ls -l "${GET_DMRPP}"

BASELINES_DIR="${abs_srcdir}/get_dmrpp_baselines"
DATA_DIR="modules/dmrpp_module/data/dmrpp"
BUILD_DIR=$(readlink -f "${abs_top_builddir}")
BES_DATA_ROOT=$(readlink -f "${abs_top_srcdir}")

test_name="$1"
input_file="$2"
baseline="${BASELINES_DIR}/$3"
params="$4"
output_file="$5"
AT_XFAIL_IF([test z$6 = zxfail]) # This is always run FIRST

echo "${input_file}" | grep "s3://"
if test $? -eq 0
then
    # We're here because it's an S3 Test
    # Only run the S3 tests if specifically instructed to do so.
    AT_SKIP_IF([test x$s3tests = xno])
else
    # It's a file test so we need to amend the input_file
    # name to reference the correct thing.
    input_file="${DATA_DIR}/${input_file}"
fi

if test -n "${output_file}"
then
    output_file="${BUILD_DIR}/modules/dmrpp_module/tests_build_dmrpp/${output_file}"
    params="${params} -o ${output_file}"
else
    output_file=stdout
fi

# Amend the PATH to pick up besstandalone
export PATH=${abs_top_builddir}/standalone:$PATH

gd_verbose=""
AS_IF([test -z "$at_verbose"], [ gd_verbose="-v -X" ])

TEST_CMD="${GET_DMRPP} -V -A -b ${BES_DATA_ROOT} ${params} ${gd_verbose} ${input_file}"

# at_verbose=""

AS_IF([test -z "$at_verbose"], [
    echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    echo "#   abs_top_srcdir: ${abs_top_srcdir}"
    echo "#       abs_srcdir: ${abs_srcdir}"
    echo "#       top_srcdir: ${top_srcdir}"
    echo "#         builddir: ${builddir}"
    echo "#     abs_builddir: ${abs_builddir}"
    echo "#     top_builddir: ${top_builddir}"
    echo "# top_build_prefix: ${top_build_prefix}"
    echo "# abs_top_builddir: ${abs_top_builddir}"
    echo "#        GET_DMRPP: ${GET_DMRPP}"
    echo "#    BES_DATA_ROOT: ${BES_DATA_ROOT}"
    echo "#    BASELINES_DIR: ${BASELINES_DIR}"
    echo "#"
    echo "# AT_GET_DMRPP_3_20() arguments: "
    echo "#           arg #1: "$1
    echo "#           arg #2: "$2
    echo "#           arg #3: "$3
    echo "#           arg #4: "$4
    echo "#           arg #5: "$5
    echo "#           arg #6: "$6
    echo "#        test_name: ${test_name}"
    echo "#       input_file: ${input_file}"
    echo "#         baseline: ${baseline}"
    echo "#      xfail_param: ${xfail_param}"
    echo "#           params: ${params}"
    echo "#      output_file: ${output_file}"
    echo "#       gd_verbose: ${gd_verbose}"
    echo "#         TEST_CMD: ${TEST_CMD}"
])

AS_IF([test -n "$baselines" -a x$baselines = xyes],
[
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Calling get_dmrpp application."])
    AT_CHECK([${TEST_CMD}], [], [stdout], [stderr])
    NORMALIZE_EXEC_NAME([${output_file}])
    REMOVE_PATH_COMPONENTS([${output_file}])
    REMOVE_VERSIONS([${output_file}])
    REMOVE_BUILD_DMRPP_INVOCATION_ATTR([${output_file}])
    REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([${output_file}])
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp_baselines: Copying result to ${baseline}.tmp"])
    AT_CHECK([mv ${output_file} ${baseline}.tmp])
],
[
    AS_IF([test -z "$at_verbose"], [echo "# get_dmrpp: Calling get_dmrpp application."])
    AT_CHECK([${TEST_CMD}], [], [stdout], [stderr])
    NORMALIZE_EXEC_NAME([${output_file}])
    REMOVE_PATH_COMPONENTS([${output_file}])
    REMOVE_VERSIONS([${output_file}])
    REMOVE_BUILD_DMRPP_INVOCATION_ATTR([${output_file}])
    REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE([${output_file}])
    AS_IF([test -z "$at_verbose"], [
        echo ""
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
        echo "# get_dmrpp: Filtered ${output_file} BEGIN"
        echo "#"
        cat ${output_file};
        echo "#"
        echo "# get_dmrpp: Filtered ${output_file} END"
        echo "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
    ])
    AT_CHECK([diff -b -B ${baseline} ${output_file}])
])

AT_CLEANUP
])


# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# Remove the build_dmrpp invocation attribute value
# ndp 03/23/22
# Usage: REMOVE_BUILD_DMRPP_INVOCATION_ATTR_VALUE(file_name)
m4_define([REMOVE_BUILD_DMRPP_INVOCATION_ATTR_VALUE], [dnl
    sed -e 's@<Value>build_dmrpp.*<\/Value>@<Value>Removed</Value>@g'  < $1 > $1.sed
    mv $1.sed $1
])

# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# Remove the build_dmrpp creation time attribute value, an ISO-68601 date string
# ndp 07/25/24
# Usage: REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE(file_name)
#             <Value>2024-07-25T18:22:54Z</Value>
m4_define([REMOVE_BUILD_DMRPP_CREATED_ATTR_VALUE], [dnl
    sed -e 's@<Value>[[0-9]]\{4\}-[[0-9]]\{2\}-[[0-9]]\{2\}T[[0-9]]\{2\}\:[[0-9]]\{2\}\:[[0-9]]\{2\}Z</Value>@<Value>removed date string</Value>@g'  < $1 > $1.sed
    mv $1.sed $1
])

# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# Replace the DAP Attribute named "invocation" an invariant message
# ndp 03/23/22
# Usage: REMOVE_BUILD_DMRPP_INVOCATION_ATTR(file_name)
m4_define([REMOVE_BUILD_DMRPP_INVOCATION_ATTR], [dnl
    # This sed magic: '1h;2,$H;$!d;g' slurps up the entire file into a single line.
    # Courtesy of: https://unix.stackexchange.com/users/21763/antak
    #   Reference: https://unix.stackexchange.com/questions/26284/how-can-i-use-sed-to-replace-a-multi-line-string
    sed \
        -e '1h;2,$H;$!d;g' \
        -e 's@<Attribute name="invocation" type="String">.*</Value>@<Attribute name="Removed(invocation)">@' \
         < $1 > $1.sed
    mv $1.sed $1
])
