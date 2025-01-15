#!/bin/sh
#
# When the release numbers are edited in configure.ac, update this
# to the current Travis number so that the 'build number' in
# x.y.z-<build number> is zero. jhrg 3/22/21

export BES_TRAVIS_BUILD_OFFSET=5824
