#!/bin/bash
#
# Test script for a Docker build of the BES RHEL 8 or 9 image 
# that we use for binary distributions. jhrg 6/26/26

# This is copied and edited slightly from the .travis.yml where the 
# build-rhel-docker.sh script is called.

export BES_BUILD="docker-el8"
export DOCKER_NAME="bes_core"
export BUILDER_BASE_IMAGE="opendap/rocky8_hyrax_builder:latest"
export FINAL_BASE_IMAGE="rockylinux:8"
export DIST="el8"
export OS="rocky8"
export SNAPSHOT_IMAGE_TAG="opendap/${DOCKER_NAME}:snapshot-${DIST}"
export BES_REPO_DIR="$(pwd)"

export DOCKER_DEV_FLAGS="--platform=linux/amd64"

export LIBDAP_RPM_VERSION="3.21.1-332" 
export CONFIGURE_OPTIONS="--disable-ncml" 
export BES_BUILD_NUMBER="0"

./travis/build-rhel-docker.sh




