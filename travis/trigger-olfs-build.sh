#!/bin/bash
#
# Given that the BES has just pushed a new set of packages, built with the libdap
# RPMs, grab those and use them to make a new set of Docker containers. The
# hyrax-docker git repo runs its own build to do this (and can be triggered
# separately).

set -e

echo "New CentOS-7 snapshot of BES pushed. Triggering a OLFS build"

LIBDAP4_SNAPSHOT=`cat libdap4-snapshot`;
echo "libdap4-snapshot record: ${LIBDAP4_SNAPSHOT}"

# Compute the BES snapshot record.
BES_SNAPSHOT="bes-`cat bes_VERSION` `date \"+%FT%T%z\"`"
echo "bes-snapshot record: ${BES_SNAPSHOT}" >&2

git clone --depth 1 https://github.com/opendap/olfs
git config --global user.name "The-Robot-Travis"
git config --global user.email "npotter@opendap.org"

cd olfs
git checkout master

# Add the libdap4 snapshot line to the bes-snapshot file.
echo "${LIBDAP4_SNAPSHOT}" > bes-snapshot

# Append the BES snapshot record to the bes-snapshot file.
echo "${BES_SNAPSHOT}" >> bes-snapshot

cat bes-snapshot >&2

git commit -am "${BES_SNAPSHOT} - Triggering OLFS build for snapshots.";
git push https://$GIT_UID:$GIT_PSWD@github.com/opendap/olfs --all;
