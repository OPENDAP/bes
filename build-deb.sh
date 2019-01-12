#!/bin/sh
#
# This is part of the Travis build CD process. It is intended to be
# run from within the Travis build process and depends on the settings
# in the BES's .travis.yml file. However, it is run _inside a Docker
# container_ using values passed in by the .travis.yml file
#
# The env vars $HOME, $OS, $DIST, AND $LIBDAP_RPM_VERSION must be set.
#
# run the script for the debian build.
# Make sure to download the debian version of libdap

# e: exit immediately on non-zero exit value from a command
# u: treat unset env vars in substitutions as an error
set -eux

echo "env:"
printenv

# verify we are in $HOME

echo "pwd = `pwd`"

cd $HOME

#Get the pre-built dependencies (all static libraries) for ubuntu.
(cd /tmp && curl -s -O https://s3.amazonaws.com/opendap.travis.build/hyrax-dependencies-$OS-static.tar.gz)

# This dumps the dependencies in $HOME/install/deps/{lib,bin,...}
tar -xzvf /tmp/hyrax-dependencies-$OS-static.tar.gz

# Then get the libdap debian package
# libdap-3.20.2-1_amd64.deb
(cd /tmp && curl -s -O https://s3.amazonaws.com/opendap.travis.build/libdap_${LIBDAP_RPM_VERSION}_amd64.deb)

dpkg -i /tmp/libdap_${LIBDAP_RPM_VERSION}_amd64.deb

# Get a fresh copy of the bes sources and any submodules
git clone https://github.com/opendap/bes

cd $HOME/bes

git submodule update --init --recursive

# build (autoreconf; configure, make)
autoreconf -fiv

#$prefix is set in the Dockerfile
./configure --disable-dependency-tracking --prefix=$prefix --with-dependencies=$prefix/deps

make deb -j7
