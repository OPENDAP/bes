#!/bin/bash
#
# sonar_scan_and_gate.sh <sonar-project-key> <sonar-properties-file>
#
# Run build-wrapper + sonar-scanner for one of the BES sonar projects and
# then poll SonarCloud's quality gate badge for the result. Building this
# as a single shared script avoids keeping three near-identical copies of
# this logic in .travis.yml (one per sonar-bes-*.properties file).
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

loggy "BEGIN $HR"

sonar_project_key="$1"
sonar_properties_file="$2"

if [ -z "$sonar_project_key" ] || [ -z "$sonar_properties_file" ]; then
    loggy "ERROR - usage: sonar_scan_and_gate.sh <sonar-project-key> <sonar-properties-file>"
    exit 1
fi

set -e

autoreconf --force --install --verbose
./configure $CONFIGURE_OPTIONS --disable-dependency-tracking --prefix="$prefix" --with-dependencies="$prefix/deps" --enable-developer --enable-coverage
build-wrapper-linux-x86-64 --out-dir bw-output make -j16

# Build the PR-specific sonar-scanner options. When this is a pull request
# build, TRAVIS_BRANCH is the PR's *target* branch (usually master), not the
# PR's own branch, so we must pass the PR context explicitly or the analysis
# is recorded as if it were a master-branch analysis. jhrg 9/1/26
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

sonar-scanner -Dproject.settings="$sonar_properties_file" -Dsonar.token="$SONAR_TOKEN" "${sonar_pr_opts[@]}"

ls -l "$TRAVIS_BUILD_DIR" >&2

# Read back the quality gate for the same context we just analyzed (PR or
# branch), not the project's default badge, which always reflects main.
badge_url="https://sonarcloud.io/api/project_badges/quality_gate?project=${sonar_project_key}"
if [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
    badge_url="${badge_url}&pullRequest=${TRAVIS_PULL_REQUEST}"
else
    badge_url="${badge_url}&branch=${TRAVIS_BRANCH}"
fi

loggy "Checking quality gate: $badge_url"
curl -s "$badge_url"
curl -s "$badge_url" | grep "QUALITY GATE PASS"
gate_status=$?

loggy "END $HR"
exit $gate_status
