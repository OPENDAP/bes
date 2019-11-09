#!/bin/bash
#
# Given that the BES has just pushed a new set of packages, built with the libdap
# RPMs, grab those and use them to make a new set of Docker containers. The
# hyrax-docker git repo runs its own build to do this (and can be triggered
# separately).

set -e

echo "-- -- -- -- -- -- -- -- -- after_deploy BEGIN -- -- -- -- -- -- -- -- --"

echo "New CentOS-7 snapshot of BES pushed. Triggering a Docker build"

git clone https://github.com/opendap/hyrax-docker
git config --global user.name "The-Robot-Travis"
git config --global user.email "npotter@opendap.org"

cd hyrax-docker/hyrax-snapshot
date +%s | tee -a snapshot.time

cat snapshot.time;

git commit -am "The BES has produced new snapshot files. Triggering Hyrax-Docker image builds for snapshots.";
git push https://$GIT_UID:$GIT_PSWD@github.com/opendap/hyrax-docker --all;

echo "-- -- -- -- -- -- -- -- -- after_deploy END -- -- -- -- -- -- -- -- --"
