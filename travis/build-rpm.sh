#!/bin/sh
#
# This is part of the Travis build CD process. It is intended to be
# run from within the Travis build process and depends on the settings
# in the BES's .travis.yml file. However, it is run _inside a Docker
# container_ using values passed in by the .travis.yml file
#
# The env vars $HOME, $OS, $DIST AND $LIBDAP_RPM_VERSION must be set.
#
# run the script like (with the obvious changes for CentOS7):
# docker run -e OS=centos6 -e DIST=el6 -e LIBDAP_RPM_VERSION='3.20.0-1'
# -v $prefix/centos6/rpmbuild:/root/rpmbuild -v `pwd`:/root/travis 
# opendap/centos6_hyrax_builder:1.1 /root/travis/build-rpm.sh 

# e: exit immediately on non-zero exit value from a command
# u: treat unset env vars in substitutions as an error
set -eux

yum update -y

# This script will start with /root as the CWD since that's how the
# centos6/7 hyrax build containers are configured. The PATH will be 
# set to include $prefix/bin and $prefix/deps/bin; $prefix will be
# $HOME/install. $HOME is /root for the build container.

echo "Inside the docker container, prefix HOME PATH:"
printenv prefix HOME PATH

# CentOS7 may not need libpng with the new hyrax-dependencies, but I'm not sure
# if the current dependency binaries are built with the latest source and build
# scripts. jhrg 1/19/22
#
# Hopefully the CentOS Stream8 docker image we use to build the RPMs has all we need.
# jhrg 2/11/22
if test -n $OS -a $OS = centos7
then
  yum install -y libpng-devel sqlite-devel
fi

# Get the pre-built dependencies (all static libraries). $OS is 'centos6' or 'centos7'
# aws s3 cp s3://opendap.travis.build/
aws s3 cp s3://opendap.travis.build/hyrax-dependencies-$OS-static.tar.gz /tmp/


# This dumps the dependencies in $HOME/install/deps/{lib,bin,...}
# The Centos7 dependencies are tarred so they include /root for a reason
# that escapes me. For CentOS Stream8, we have to CD to /root before expanding
# the tar ball to get the dependencies in /root/install. jhrg 2/11/22
if test -n $OS -a $OS = centos-stream8
then
  tar -C /$HOME -xzvf /tmp/hyrax-dependencies-$OS-static.tar.gz
else
  tar -xzvf /tmp/hyrax-dependencies-$OS-static.tar.gz
fi

ls -lR $HOME/install/deps

# Then get the libdap RPMs packages
# libdap-3.20.0-1.el6.x86_64.rpm libdap-devel-3.20.0-1.el6.x86_64.rpm
# $DIST is 'el6', 'el7', or 'el8'; $LIBDAP_RPM_VERSION is 3.20.0-1 (set by Travis)
aws s3 cp s3://opendap.travis.build/libdap-$LIBDAP_RPM_VERSION.$DIST.x86_64.rpm /tmp/
aws s3 cp s3://opendap.travis.build/libdap-devel-$LIBDAP_RPM_VERSION.$DIST.x86_64.rpm /tmp/

yum install -y /tmp/*.rpm

# cd to the $TRAVIS_BUILD_DIR directory. Note that we make $HOME/travis
# using the docker run --volume option and set it to $TRAVIS_BUILD_DIR.
cd $HOME/travis

# The build needs these environment variables because CentOS 8 lacks the stock
# XDR/RPC libraries. Those were added to the docker image via the tirpc package.
# jhrg 2/11/22
if test -n $OS -a $OS = centos-stream8
then
  export CPPFLAGS=-I/usr/include/tirpc
  export LDFLAGS=-ltirpc
fi

autoreconf -fiv

echo "BES_BUILD_NUMBER: $BES_BUILD_NUMBER"
./configure --disable-dependency-tracking --prefix=$prefix --with-dependencies=$prefix/deps --with-build=$BES_BUILD_NUMBER

# set up the rpm tree in $HOME. We didn't need to do this for libdap because 
# rpmbuild took care of it, but for the BES, the Makefile copies the source 
# tarball and thus, we have to make the rpm dir tree. Reading the Makefile.am
# will reveal more...
mkdir -pv $HOME/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# This will leave the package in $HOME/rpmbuild/RPMS/x86_64/*.rpm
make -j16 all-static-rpm

# Just a little reassurance... jhrg 3/23/21
ls -l $HOME/rpmbuild/RPMS/x86_64/
