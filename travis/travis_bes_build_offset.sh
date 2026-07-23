#!/bin/bash
#
# When the release numbers are edited in configure.ac, update this
# to the current Travis number so that the 'build number' in
# x.y.z-<build number> is zero. jhrg 3/22/21
#
HR="=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="
###########################################################################
# loggy()
function loggy() {
    echo "$@" | awk '{ print "# travis_bes_build_offset.sh() - "$0;}' >&2
}

loggy "BEGIN $HR"

# This is the TravisCI build number when the
# last formal release was built.
export BES_TRAVIS_BUILD_OFFSET=7717

if [ "$TRAVIS_PULL_REQUEST" != "false" ]
then
  loggy "This is a Pull Request build for PR #$TRAVIS_PULL_REQUEST"
  loggy "Setting BES_TRAVIS_BUILD_OFFSET to 0"
  BES_TRAVIS_BUILD_OFFSET=0
fi

loggy "Using BES_TRAVIS_BUILD_OFFSET: $BES_TRAVIS_BUILD_OFFSET"
loggy "END $HR"
