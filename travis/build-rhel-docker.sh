#!/bin/sh
#
# This is part of the Travis build CD process. It is intended to be
# run from within the Travis build process and depends on env variables
# set in the BES's .travis.yml file.
#
# It can be run locally, as long as the env vars $BES_REPO_DIR,
# $SNAPSHOT_IMAGE_TAG, $BUILD_VERSION_TAG, $DIST, $OS, $BES_BUILD_NUMBER,
# and $LIBDAP_RPM_VERSION are set.
#
# Optional env vars are $GDAL_OPTION and $DOCKER_DEV_FLAGS.
#
# When running locally, AWS credentials for OPeNDAP AWS must also be set, e.g.,
# $AWS_ACCESS_KEY_ID and $AWS_SECRET_ACCESS_KEY (or set via aws configuration)
#
# Run the script like this:
#    BES_REPO_DIR="." \
#    SNAPSHOT_IMAGE_TAG="opendap/bes_rhel8:snapshot" \
#    BUILD_VERSION_TAG="opendap/bes_rhel8:12345" \
#    DIST=el8 \
#    OS=rocky8 \
#    BES_BUILD_NUMBER=12345 \
#    LIBDAP_RPM_VERSION='3.21.1-332' \
#    bash build-rhel-docker.sh
#
# If building locally, add extra docker build flags through the ENV var
# `DOCKER_DEV_FLAGS`, e.g.,
# export DOCKER_DEV_FLAGS="--platform linux/amd64 --no-cache"
#
# e: exit immediately on non-zero exit value from a command
# u: treat unset env vars in substitutions as an error
set -eu

function loggy(){
    echo  "$@" | awk '{ print "# "$0;}'  >&2
}

HYRAX_DEPENDENCIES_TARBALL="hyrax-dependencies-${OS}.tgz"
LIBDAP_RPM_FILENAME="libdap-$LIBDAP_RPM_VERSION.$DIST.x86_64.rpm"
LIBDAP_DEVEL_RPM_FILENAME="libdap-devel-$LIBDAP_RPM_VERSION.$DIST.x86_64.rpm"
DOCKER_DEV_FLAGS=${DOCKER_DEV_FLAGS:-""}
GDAL_OPTION=${GDAL_OPTION:-"--without-gdal"}
AWS_DOWNLOADS_DIR="/tmp/dependency_downloads"

loggy "#########################################################################"
loggy "$0 BEGIN"
loggy "Preparing to build docker image."
loggy ""
loggy "Input variables:"
loggy "       BES_REPO_DIR: '$BES_REPO_DIR'"
loggy " SNAPSHOT_IMAGE_TAG: '$SNAPSHOT_IMAGE_TAG'"
loggy "  BUILD_VERSION_TAG: '$BUILD_VERSION_TAG'"
loggy "               DIST: '$DIST'"
loggy "                 OS: '$OS'"
loggy "   BES_BUILD_NUMBER: '$BES_BUILD_NUMBER'"
loggy " LIBDAP_RPM_VERSION: '$LIBDAP_RPM_VERSION'"
loggy "        GDAL_OPTION: '$GDAL_OPTION'"
loggy "   DOCKER_DEV_FLAGS: '$DOCKER_DEV_FLAGS'"
loggy ""
loggy "Artifact info:"
loggy "       HYRAX_DEPENDENCIES_TARBALL: '$HYRAX_DEPENDENCIES_TARBALL'"
loggy "       LIBDAP_RPM_FILENAME: '$LIBDAP_RPM_FILENAME'"
loggy "       LIBDAP_DEVEL_RPM_FILENAME: '$LIBDAP_DEVEL_RPM_FILENAME'"
loggy "       AWS_DOWNLOADS_DIR: '$AWS_DOWNLOADS_DIR'"
loggy ""

set -eux

# Downloading AWS dependencies...
mkdir -p $AWS_DOWNLOADS_DIR
[[ -e "$AWS_DOWNLOADS_DIR/$HYRAX_DEPENDENCIES_TARBALL" ]] || aws s3 cp "s3://opendap.travis.build/$HYRAX_DEPENDENCIES_TARBALL" $AWS_DOWNLOADS_DIR
[[ -e "$AWS_DOWNLOADS_DIR/$LIBDAP_RPM_FILENAME" ]] || aws s3 cp "s3://opendap.travis.build/$LIBDAP_RPM_FILENAME" "$AWS_DOWNLOADS_DIR"
[[ -e "$AWS_DOWNLOADS_DIR/$LIBDAP_DEVEL_RPM_FILENAME" ]] || aws s3 cp "s3://opendap.travis.build/$LIBDAP_DEVEL_RPM_FILENAME" "$AWS_DOWNLOADS_DIR"

# Building the docker image...
docker image pull opendap/rocky8_hyrax_builder:latest
docker build \
    --build-arg LIBDAP_RPM_FILENAME="$LIBDAP_RPM_FILENAME" \
    --build-arg LIBDAP_DEVEL_RPM_FILENAME="$LIBDAP_DEVEL_RPM_FILENAME" \
    --build-arg HYRAX_DEPENDENCIES_TARBALL="$HYRAX_DEPENDENCIES_TARBALL" \
    --build-arg CPPFLAGS=-I/usr/include/tirpc \
    --build-arg LDFLAGS=-ltirpc \
    --build-arg GDAL_OPTION="$GDAL_OPTION" \
    --build-arg NJOBS_OPTION="-j16" \
    --build-arg BES_BUILD_NUMBER="$BES_BUILD_NUMBER" \
    --build-arg PREFIX=/root/install \
    --tag "${SNAPSHOT_IMAGE_TAG}" \
    --tag "${BUILD_VERSION_TAG}" \
    --build-context aws_downloads="$AWS_DOWNLOADS_DIR/" \
    --progress=plain $DOCKER_DEV_FLAGS \
    -f ${BES_REPO_DIR}/Dockerfile ${BES_REPO_DIR}


echo "Docker build complete!"
docker image ls -a