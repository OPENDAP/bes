#!/bin/bash
#
# Given that the BES has just pushed a new set of packages, built with the libdap
# RPMs, grab those and use them to make a new set of Docker containers. The
# hyrax-docker git repo runs its own build to do this (and can be triggered
# separately).

set -e

git config --global user.name "The-Robot-Travis"
git config --global user.email "npotter@opendap.org"

echo "New CentOS-7 snapshot of BES pushed. Triggering a OLFS build" >&2

LIBDAP4_SNAPSHOT=`cat libdap4-snapshot`
echo "libdap4-snapshot record: ${LIBDAP4_SNAPSHOT}" >&2

export libdap_version=$(echo LIBDAP4_SNAPSHOT | grep libdap | awk '{print $1;}' | sed "s/libdap4-//g" )
echo "libdap_version: ${libdap_version}" >&2

export bes_version=$(cat ./bes_VERSION)
echo "bes_version: ${bes_version}" >&2

export time_now=$(date "+%FT%T%z")
echo "time_now: ${time_now}" >&2

# Build the BES snapshot record.
BES_SNAPSHOT="bes-${bes_version} ${time_now}"
echo "bes-snapshot record: ${BES_SNAPSHOT}" >&2

echo "Tagging bes with version: ${bes_version}"
git tag -m "bes-${bes_version}" -a "${bes_version}"
git push "https://${GIT_UID}:${GIT_PSWD}@github.com/OPENDAP/bes.git" "${bes_version}"

# Now do the work to trigger the OLFS TravisCI build.
git clone --depth 1 https://github.com/OPENDAP/olfs
cd olfs
git checkout master

# Add the libdap4 snapshot line to the bes-snapshot file.
echo "${LIBDAP4_SNAPSHOT}" > bes-snapshot

# Append the BES snapshot record to the bes-snapshot file.
echo "${BES_SNAPSHOT}" >> bes-snapshot

cat bes-snapshot >&2

# Bounding the commit message with the " character allows use to include
# new line stuff for easy commit message readability later.
git commit -am \
"bes: Triggering OLFS build for snapshot production.
Build Version Matrix:
${BES_SNAPSHOT}
";

git push "https://${GIT_UID}:${GIT_PSWD}@github.com/OPENDAP/olfs.git" --all;
