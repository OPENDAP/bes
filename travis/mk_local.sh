#!/bin/bash

export  HR="#######################################################################"
export HR1="--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---"
function loggy(){
    echo  "$@" | awk '{ print "# mk_local() - "$0;}'  >&2
}
loggy "$HR"
loggy "BEGIN"

make_target="${1:-"check"}"
loggy "make_target: $make_target"

# We stop failing on error (set +e) for running
# make check because we want to collect the test
# logs before the job fails.

set +e
make "$make_target" -j16
test_status=$?

# We use fail on error (set -e) for
# uploading the test logs, and
# acting on the error state.

set -e
./travis/collect_autotest_logs.sh
./travis/upload-test-results.sh
if [ $test_status -ne 0 ]; then
    echo "# ERROR - 'make check' failed, exiting. test_status: $test_status" >&2
fi

loggy "END"
loggy "$HR"
exit $test_status;
