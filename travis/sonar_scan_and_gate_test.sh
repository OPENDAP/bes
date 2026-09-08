#!/bin/bash
#
# sonar_scan_and_gate_test.sh
#
# Unit tests for the PR-vs-branch decision logic in sonar_scan_and_gate.sh.
# This sources that file (which, per its own guard, does not run `main` when
# sourced) so it can call build_sonar_pr_opts() directly with different
# TRAVIS_* environment variable combinations and check the resulting
# sonar-scanner options, without needing autoreconf/configure/sonar-scanner
# or network/SonarCloud access.
#
# Run with:
#   ./travis/sonar_scan_and_gate_test.sh
#
HR="=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$script_dir/sonar_scan_and_gate.sh"

# loggy() comes from the sourced script (deliberately reused here instead
# of defining a second copy that sourcing would just shadow anyway).
loggy "BEGIN $HR"

failures=0

# assert_sonar_pr_opts_eq <test-name> <expected-value>
#
# Compares against the global sonar_pr_opts array left behind by
# build_sonar_pr_opts(). Not written as a generic "pass an array name"
# helper because bash 3.2 (the default /bin/bash on macOS, with no
# Homebrew bash installed on this machine) has no namerefs (`local -n`,
# bash 4.3+) to pass an array by reference. jhrg 9/1/26
function assert_sonar_pr_opts_eq() {
    local test_name="$1"
    local expected="$2"
    local actual
    actual="$(printf '%s\n' "${sonar_pr_opts[@]}")"

    if [ "$actual" == "$expected" ]; then
        loggy "PASS - $test_name"
    else
        loggy "FAIL - $test_name"
        loggy "  expected: $expected"
        loggy "  actual:   $actual"
        failures=$((failures + 1))
    fi
}

# assert_eq <test-name> <expected> <actual>
function assert_eq() {
    local test_name="$1"
    local expected="$2"
    local actual="$3"

    if [ "$actual" == "$expected" ]; then
        loggy "PASS - $test_name"
    else
        loggy "FAIL - $test_name (expected '$expected', got '$actual')"
        failures=$((failures + 1))
    fi
}

# --- Test 1: a plain branch build (e.g. a push to master) gets no
# --- sonar.pullrequest.* options. ---
TRAVIS_PULL_REQUEST="false"
TRAVIS_PULL_REQUEST_BRANCH=""
TRAVIS_BRANCH="master"
build_sonar_pr_opts
assert_eq "branch build: sonar_pr_opts is empty" "0" "${#sonar_pr_opts[@]}"

# --- Test 2: a pull request build gets the three sonar.pullrequest.*
# --- options, built from PR number/source branch/target branch, in order.
# --- This is the exact behavior that was missing before this script
# --- existed, which caused PR scans to be recorded against master. ---
TRAVIS_PULL_REQUEST="42"
TRAVIS_PULL_REQUEST_BRANCH="feature-x"
TRAVIS_BRANCH="master"
build_sonar_pr_opts
expected=$'-Dsonar.pullrequest.key=42\n-Dsonar.pullrequest.branch=feature-x\n-Dsonar.pullrequest.base=master'
assert_sonar_pr_opts_eq "PR build: sonar_pr_opts has key/branch/base in order" "$expected"

# --- Test 3: a pull request build targeting a non-master branch still uses
# --- that branch as sonar.pullrequest.base (not hardcoded to "master"). ---
TRAVIS_PULL_REQUEST="7"
TRAVIS_PULL_REQUEST_BRANCH="hotfix-y"
TRAVIS_BRANCH="release-3.x"
build_sonar_pr_opts
expected=$'-Dsonar.pullrequest.key=7\n-Dsonar.pullrequest.branch=hotfix-y\n-Dsonar.pullrequest.base=release-3.x'
assert_sonar_pr_opts_eq "PR build against non-master target: base tracks TRAVIS_BRANCH" "$expected"

# --- Test 4: main() rejects missing arguments before doing any real work
# --- (no autoreconf/configure/sonar-scanner should run). ---
( main )
assert_eq "main with no args: returns 1" "1" "$?"

( main only-one-arg )
assert_eq "main with one arg: returns 1" "1" "$?"

loggy "END $HR"

if [ "$failures" -ne 0 ]; then
    loggy "$failures test(s) FAILED"
    exit 1
fi

loggy "All tests passed"
exit 0
