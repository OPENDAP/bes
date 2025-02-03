#!/bin/bash
#
# This script accomplishes the following:
#   - It tags the repo with the version+build number and pushes the tag to GitHub
#   - It triggers the next component in the build matrix, the OLFS, to build.
#     This is accomplished by:
#         - Checking out the OLFS project on the master branch.
#         - Updating the "build recipe" in the file "bes-snapshot" with the libdap4 and bes versions
#         - Committing the changes and pushing them to github.
#         - This will cause the olfs project to build and a successful completion of the olfs
#           build will trigger the hyrax-docker build.
#

set -e

git config --global user.name "The-Robot-Travis"
git config --global user.email "npotter@opendap.org"

echo "New snapshot of BES pushed. Triggering a OLFS build" >&2

LIBDAP4_SNAPSHOT=$(cat ./libdap4-snapshot)
echo "libdap4-snapshot record: ${LIBDAP4_SNAPSHOT}" >&2

#export libdap_version=$(echo "${LIBDAP4_SNAPSHOT}" | grep libdap | awk '{print $1;}' | sed "s/libdap4-//g" )
#echo "libdap_version: ${libdap_version}" >&2

export bes_version=$(cat ./bes_VERSION)
echo "bes_version: ${bes_version}" >&2

export build_dmrpp_version="build_dmrpp-${bes_version}"
echo "build_dmrpp_version: ${build_dmrpp_version}" >&2

export time_now=$(date "+%FT%T%z")
echo "time_now: ${time_now}" >&2

# Build the BES snapshot record.
BES_SNAPSHOT="bes-${bes_version} ${time_now}"
echo "BES_SNAPSHOT: ${BES_SNAPSHOT}" >&2

# Build the build_dmrpp snapshot record.
BUILD_DMRPP_SNAPSHOT="${build_dmrpp_version} ${time_now}"
echo "BUILD_DMRPP_SNAPSHOT: ${BUILD_DMRPP_SNAPSHOT}" >&2

echo "Tagging bes with version: ${bes_version}"
git tag -m "bes-${bes_version}" -a "${bes_version}"
git push "https://${GIT_UID}:${GIT_PSWD}@github.com/OPENDAP/bes.git" "${bes_version}"

echo "Tagging bes with build_dmrpp version: ${build_dmrpp_version}"
git tag -m "${build_dmrpp_version}" -a "${build_dmrpp_version}"
git push "https://${GIT_UID}:${GIT_PSWD}@github.com/OPENDAP/bes.git" "${build_dmrpp_version}"

# Now do the work to trigger the OLFS TravisCI build.
git clone --depth 1 https://github.com/OPENDAP/olfs
cd olfs
git checkout master

# Add the libdap4 snapshot line to the bes-snapshot file.
echo "${LIBDAP4_SNAPSHOT}" > bes-snapshot

# Append the BES snapshot record to the bes-snapshot file.
echo "${BES_SNAPSHOT}" >> bes-snapshot

# Append the build_dmrpp snapshot record to the bes-snapshot file.
echo "${BUILD_DMRPP_SNAPSHOT}" >> bes-snapshot

cat bes-snapshot >&2

# Bounding the commit message with the " character allows use to include
# new line stuff for easy commit message readability later.
git commit -am \
"bes: Triggering OLFS build for snapshot production.\nBuild Version Matrix:\n${LIBDAP4_SNAPSHOT}\n${BES_SNAPSHOT}";

git push "https://${GIT_UID}:${GIT_PSWD}@github.com/OPENDAP/olfs.git" --all;
