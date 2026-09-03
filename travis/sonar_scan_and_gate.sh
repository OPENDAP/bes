#!/bin/bash
#
# sonar_scan_and_gate.sh <sonar-project-key> <sonar-properties-file>
#
# Run build-wrapper + sonar-scanner for one of the BES sonar projects.
# sonar-scanner itself fails the build on a quality gate failure (see
# sonar.qualitygate.wait=true in the sonar-bes-*.properties files).
# Building this as a single shared script avoids keeping three
# near-identical copies of this logic in .travis.yml (one per
# sonar-bes-*.properties file).
#
# Usage (from .travis.yml), e.g. for the "bes" scan:
#   ./travis/sonar_scan_and_gate.sh opendap-bes sonar-bes-framework.properties
#
# Correctly scanning a pull request requires more than pointing curl at the
# right badge URL: sonar-scanner itself must be told this is a PR analysis
# (sonar.pullrequest.key/branch/base), otherwise SonarCloud records the scan
# against the project's main branch, which both reports the wrong quality
# gate result AND overwrites master's own analysis history with the PR's
# code. jhrg 9/1/26
#
HR="=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="

function loggy() {
    echo "$@" | awk '{ print "# sonar_scan_and_gate.sh() - "$0;}' >&2
}

# Build the PR-specific sonar-scanner options into the global array
# sonar_pr_opts, based on the Travis TRAVIS_PULL_REQUEST/TRAVIS_BRANCH/
# TRAVIS_PULL_REQUEST_BRANCH environment variables. When this is a pull
# request build, TRAVIS_BRANCH is the PR's *target* branch (usually
# master), not the PR's own branch, so we must pass the PR context
# explicitly or the analysis is recorded as if it were a master-branch
# analysis. Pulled out as its own function so sonar_scan_and_gate_test.sh
# can source this file and exercise this decision without running the
# rest of the script (autoreconf/configure/sonar-scanner). jhrg 9/1/26
function build_sonar_pr_opts() {
    sonar_pr_opts=()
    if [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
        loggy "This is a Pull Request build for PR #$TRAVIS_PULL_REQUEST ($TRAVIS_PULL_REQUEST_BRANCH -> $TRAVIS_BRANCH)"
        sonar_pr_opts=(
            "-Dsonar.pullrequest.key=$TRAVIS_PULL_REQUEST"
            "-Dsonar.pullrequest.branch=$TRAVIS_PULL_REQUEST_BRANCH"
            "-Dsonar.pullrequest.base=$TRAVIS_BRANCH"
        )
    else
        loggy "This is a branch build for $TRAVIS_BRANCH"
    fi
}

# The part of the script that actually builds and scans. Kept in a function,
# and only invoked below when this file is executed (not sourced), so that
# sonar_scan_and_gate_test.sh can source the file for just build_sonar_pr_opts()
# without triggering a real build. jhrg 9/1/26
function main() {
    loggy "BEGIN $HR"

    sonar_project_key="$1"
    sonar_properties_file="$2"

    if [ -z "$sonar_project_key" ] || [ -z "$sonar_properties_file" ]; then
        loggy "ERROR - usage: sonar_scan_and_gate.sh <sonar-project-key> <sonar-properties-file>"
        return 1
    fi

    set -e

    autoreconf --force --install --verbose
    ./configure $CONFIGURE_OPTIONS --disable-dependency-tracking --prefix="$prefix" --with-dependencies="$prefix/deps" --enable-developer --enable-coverage
    build-wrapper-linux-x86-64 --out-dir bw-output make -j16

    build_sonar_pr_opts

    sonar-scanner -Dproject.settings="$sonar_properties_file" -Dsonar.token="$SONAR_TOKEN" "${sonar_pr_opts[@]}"

    ls -l "$TRAVIS_BUILD_DIR" >&2

    # In the .travis.yml file, we used curl to fetch the project_badges/quality_gate
    # SVG badge and grep it for "QUALITY GATE PASS" to fail the build. That's a
    # display endpoint for README badges, not a documented API for CI gating: the
    # pass/fail text is rendered into SVG markup with no stable contract, and badge
    # responses can be served from a cache, so a check run right after analysis
    # isn't guaranteed to see the just-computed result. It's also redundant: every
    # sonar-bes-*.properties file sets sonar.qualitygate.wait=true, so sonar-scanner
    # (above) already blocks on the quality gate and exits non-zero when it fails,
    # which - combined with `set -e` - already fails this script. jhrg 9/1/26

    loggy "END $HR"
}

# Only run main when executed directly (./sonar_scan_and_gate.sh ...), not
# when sourced by a test script. jhrg 9/1/26
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
    exit $?
fi
