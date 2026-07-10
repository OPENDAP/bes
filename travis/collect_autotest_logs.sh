#!/bin/bash
#
#
#
#
#
#
HR="########################################################################"
HR3="--- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---"
HR2="-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --"
HR1="- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"

function loggy() {
    echo  "$@" | awk '{ print "# collect_autotest_logs() - "$0;}'  >&2
}

loggy "$HR"
loggy "BEGIN"

user="${BES_USER:-"$(id -un)"}"
loggy "              user: $user"

group="${BES_GROUP:-"$(id -gn)"}"
loggy "             group: $group"

TEST_LOGS_DIR="${TEST_LOGS_DIR:-"/tmp"}"
loggy "     TEST_LOGS_DIR: $TEST_LOGS_DIR"

TEST_STATUS_FILE="${TEST_LOGS_DIR}/bes-tests-status"
loggy "  TEST_STATUS_FILE: $TEST_STATUS_FILE"

if [ -n "${TRAVIS_JOB_NUMBER}" ]
then
  TEST_LOGS_FILE="${TEST_LOGS_DIR}/bes-autotest-${TRAVIS_JOB_NUMBER}-logs.tar.gz"
else
  TEST_LOGS_FILE="${TEST_LOGS_DIR}/bes-autotest-logs.tar.gz"
fi
loggy "    TEST_LOGS_FILE: $TEST_LOGS_FILE"

TEST_LOG_INVENTORY="${TEST_LOGS_DIR}/bes-log-file-list.txt"
loggy "TEST_LOG_INVENTORY: $TEST_LOG_INVENTORY"

loggy "$HR2 "

loggy "# Setup $TEST_LOGS_DIR"

mkdir -vp "$TEST_LOGS_DIR"
chown -v $BES_USER:$BES_USER "$TEST_LOGS_DIR"

loggy "$HR2 "
loggy "# Bundling test logs and site_maps:"
find . \( -name "*.log" -o -name "*site_map.txt" \) -print > "$TEST_LOG_INVENTORY"
loggy "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- "
loggy "# Test Log Inventory:"
loggy "$(cat "$TEST_LOG_INVENTORY")"
loggy "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- "
loggy "# Making Test Log tarball..."
loggy "$(tar -cvzf "$TEST_LOGS_FILE" -T "$TEST_LOG_INVENTORY" 2>&1 )"
loggy "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- "
loggy "# TEST_LOGS_FILE: $(ls -l "$TEST_LOGS_FILE")"
loggy "# -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- "

loggy "END"
loggy "$HR"
