#!/bin/sh
#
# This is part of the Travis build CD process. It is intended to be
# run from within the Travis build process and depends on the settings
# in the BES's .travis.yml file. However, it is run _inside a Docker
# container_ using values passed in by the .travis.yml file
#
# The env vars $HOME, $os and $dist must be set.
#
# run the script like (with the obvious changes for CentOS7):
# docker run -e os=centos6 -e dist=el6 
# -v $prefix/centos6/rpmbuild:/root/rpmbuild -v `pwd`:/root/travis 
# opendap/centos6_hyrax_builder:1.1 /root/travis/build-rpm.sh 

# e: exit immediately on non-zero exit value from a command
# u: treat unset env vars in substitutions as an error
set -eux

# This script will start with /root as the CWD since that's how the 
# centos6/7 hyrax build containers are configured. The PATH will be 
# set to include $prefix/bin and $prefix/deps/bin; $prefix will be
# $HOME/install. $HOME is /root for the build container.

echo "env:"
printenv

# verify we are in $HOME

echo "pwd = `pwd`"

if test $HOME != $PWD
then
    exit 1
fi

# Get the pre-built dependencies (all static libraries). $os is 'centos6' or 'centos7'
(cd /tmp && curl -s -O https://s3.amazonaws.com/opendap.travis.build/hyrax-dependencies-$os-static.tar.gz)

# This dumps the dependencies in $HOME/install/deps/{lib,bin,...}
tar -xzvf /tmp/hyrax-dependencies-$os-static.tar.gz

# Then get the libdap RPMs packages
# libdap-3.20.0-1.el6.x86_64.rpm libdap-devel3.20.0-1.el6.x86_64.rpm
# $dist is 'el6' or 'el7'
(cd /tmp && curl -s -O https://s3.amazonaws.com/opendap.travis.build/libdap-3.20.0-1.$dist.x86_64.rpm)
(cd /tmp && curl -s -O https://s3.amazonaws.com/opendap.travis.build/libdap-devel-3.20.0-1.$dist.x86_64.rpm)

yum install -y /tmp/*.rpm

# Get a fresh copy of the sources and any submodules
#  git clone https://github.com/opendap/bes
# Since we have a local copy of the repo, clone that, avoiding network traffic
git clone /root/travis bes

cd bes

git submodule update --init --recursive

# build (autoreconf; configure, make)
autoreconf -fiv

./configure --disable-dependency-tracking --prefix=$prefix --with-dependencies=$prefix/deps

# set up the rpm tree in $HOME. We didn't need to do this for libdap because 
# rpmbuild took care of it, but for the BES, the Makefile copies the source 
# tarball and thus, we have to make the rpm dir tree. Reading the Makefile.am
# will reveal more...
mkdir -pv $HOME/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# This will leave the package in $HOME/rpmbuild/RPMS/x86_64/*.rpm
if test $dist = el6
then
    make -j4 c6-all-static-rpm
else
    make -j4 all-static-rpm
fi
    