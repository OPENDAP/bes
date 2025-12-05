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
# opendap/rocky8_hyrax_builder:1.1 /root/travis/travis/build-rh8-rpm.sh

# e: exit immediately on non-zero exit value from a command
# u: treat unset env vars in substitutions as an error
set -eux

# Formatted output shenanigans...
HR="#########################################################################"
###########################################################################
# loggy()
function loggy(){
    echo  "$@" | awk '{ print "# "$0;}'  >&2
}

yum update -y

# This script will start with /root as the CWD since that's how the
# centos6/7 hyrax build containers are configured. The PATH will be 
# set to include $prefix/bin and $prefix/deps/bin; $prefix will be
# $HOME/install. $HOME is /root for the build container.
loggy "$HR"
loggy "$0 BEGIN"
loggy "Inside the docker container"
loggy "prefix: $prefix"
loggy "HOME: $HOME"
loggy "PATH: $PATH"
loggy "$OS: $OS"
loggy "GDAL_OPTION: $GDAL_OPTION"
loggy "BES_BUILD_NUMBER: $BES_BUILD_NUMBER"

DEPENDENCIES_BUNDLE="hyrax-dependencies-$OS-static.tar.gz"

aws s3 cp "s3://opendap.travis.build/$DEPENDENCIES_BUNDLE" /tmp/

loggy "Dependencies: "
# This dumps the dependencies in $HOME/install/deps/{lib,bin,...}
# The Centos7 dependencies are tarred so they include /root for a reason
# that escapes me. For CentOS Stream8, we have to CD to /root before expanding
# the tar ball to get the dependencies in /root/install. jhrg 2/11/22
if test -n $OS -a $OS = rocky8 -o $OS = centos-stream8
then
  tar -C /$HOME -xzvf "$DEPENDENCIES_BUNDLE"
else
  tar -xzvf /tmp/"$DEPENDENCIES_BUNDLE"
fi

loggy "Dependencies Inventory:"
loggy "$(ls -lR $HOME/install/deps)"

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
if test -n $OS -a $OS = rocky8 -o $OS = centos-stream8
then
  export CPPFLAGS=-I/usr/include/tirpc
  export LDFLAGS=-ltirpc
fi

autoreconf -fiv

./configure --disable-dependency-tracking --prefix=$prefix --with-dependencies=$prefix/deps $GDAL_OPTION --with-build=$BES_BUILD_NUMBER

# set up the rpm tree in $HOME. We didn't need to do this for libdap because 
# rpmbuild took care of it, but for the BES, the Makefile copies the source 
# tarball and thus, we have to make the rpm dir tree. Reading the Makefile.am
# will reveal more...
mkdir -pv $HOME/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# This will leave the package in $HOME/rpmbuild/RPMS/x86_64/*.rpm
make -j16 all-static-rpm

# Just a little reassurance... jhrg 3/23/21
loggy "RPM Inventory:"
loggy "$(ls -l $HOME/rpmbuild/RPMS/x86_64/)"
