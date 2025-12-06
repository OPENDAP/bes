#!/bin/bash
#
# This script is used to debug the RPM build process using a docker container.
# It mirrors the way the RPM build is done using Travis, as closely as possible
# on a local machine. jhrg 6/7/24

# This is how Travis builds the RHEL 8 RPMs:
#
#        - export BES_BUILD=centos-stream8
#        - mkdir -p $prefix/rpmbuild
#        - echo "branch name ${TRAVIS_PULL_REQUEST_BRANCH:-$TRAVIS_BRANCH}"
#        - docker run --env prefix=/root/install --volume $prefix/rpmbuild:/root/rpmbuild
#            --volume $TRAVIS_BUILD_DIR:/root/travis
#            --env OS=centos-stream8
#            --env DIST=el8
#            --env LIBDAP_RPM_VERSION=$LIBDAP_RPM_VERSION
#            --env BES_BUILD_NUMBER=$BES_BUILD_NUMBER
#            --env AWS_ACCESS_KEY_ID=$AWS_ACCESS_KEY_ID
#            --env AWS_SECRET_ACCESS_KEY=$AWS_SECRET_ACCESS_KEY
#            opendap/centos-stream8_hyrax_builder:1.1 /root/travis/travis/build-rh8-rpm.sh

# I used the keys that are bound to the travis-bes user, but any keys that can
# read from the opendap.travis.build S3 bucket should work.

if printenv prefix >/dev/null 2>&1; then echo "Prefix: $prefix"; else echo "Env Var prefix not defined"; exit; fi
if printenv AWS_ACCESS_KEY_ID >/dev/null 2>&1; then echo "AWS_ACCESS_KEY_ID: X"; else echo "AWS_ACCESS_KEY_ID not defined"; exit; fi
if printenv AWS_SECRET_ACCESS_KEY >/dev/null 2>&1; then echo "AWS_SECRET_ACCESS_KEY: X"; else echo "AWS_SECRET_ACCESS_KEY not defined"; exit; fi

# Get these from the path to the repo and $BES_SRC_DIR/libdap4-snapshot
export BES_SRC_DIR=/Users/jgallag4/src/hyrax/bes
export LIBDAP_RPM_VERSION=3.21.0-70

export BES_BUILD_NUMBER=0

mkdir -p $prefix/rpmbuild

docker run -it --volume $prefix/rpmbuild:/root/rpmbuild \
  --volume $BES_SRC_DIR:/root/travis \
  --env prefix=/root/install \
  --env OS=centos-stream8 \
  --env DIST=el8 \
  --env LIBDAP_RPM_VERSION=$LIBDAP_RPM_VERSION \
  --env BES_BUILD_NUMBER=$BES_BUILD_NUMBER \
  --env AWS_ACCESS_KEY_ID=$AWS_ACCESS_KEY_ID \
  --env AWS_SECRET_ACCESS_KEY=$AWS_SECRET_ACCESS_KEY \
  opendap/rocky8_hyrax_builder:latest /root/travis/travis/build-rh8-rpm.sh

