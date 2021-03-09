#!/bin/bash
#
# Given that the BES has just pushed a new set of packages, built with the libdap
# RPMs, grab those and use them to make a new set of Docker containers. The
# hyrax-docker git repo runs its own build to do this (and can be triggered
# separately).

set -e

echo "-- -- -- -- -- -- -- -- -- after_deploy BEGIN -- -- -- -- -- -- -- -- --"

echo "New CentOS-7 snapshot of BES pushed. Triggering a OLFS build"

git clone --depth 1 https://github.com/opendap/olfs
git config --global user.name "The-Robot-Travis"
git config --global user.email "npotter@opendap.org"

cd olfs
git checkout master

snap_time="BES-<version.build> "`date "+%FT%T%z"`

echo "bes-snapshot record: ${snap_time}"
echo "${snap_time}" > bes-snapshot

cat bes-snapshot;

git commit -am "${snap_time} Triggering OLFS build for snapshots.";
git push https://$GIT_UID:$GIT_PSWD@github.com/opendap/olfs --all;

echo "-- -- -- -- -- -- -- -- -- after_deploy END -- -- -- -- -- -- -- -- --"
