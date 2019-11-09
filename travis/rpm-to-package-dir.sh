#!/bin/bash
#
# Part of the deployment process managed by Travis CI. This code copies RPM
# packages to a 'package' directory. It also duplicates them, making a set
# of RPMs labeled 'snapshot.'

set -e

# DIST should be one of el6 or el7
DIST=${1}

# 'prefix' and 'TRAVIS_BUILD_DIR' are in the environment

# Copied the RPM files with version numbers.
cp ${prefix}/rpmbuild/RPMS/x86_64/* ${TRAVIS_BUILD_DIR}/package/

# Now make a second set of copies with 'snapshot' in place of the version
ver=`basename ${prefix}/rpmbuild/RPMS/x86_64/bes-[-0-9.]*.rpm | sed -e "s|bes-||g" -e "s|.static.${DIST}.x86_64.rpm||g"`
for file in ${prefix}/rpmbuild/RPMS/x86_64/*
do
    echo "Updating BES ${DIST} snapshot with ${file}"
    snap=`basename ${file} | sed "s|${ver}|snapshot|g"`
    cp ${file} ${TRAVIS_BUILD_DIR}/package/${snap}
done

exit 0
