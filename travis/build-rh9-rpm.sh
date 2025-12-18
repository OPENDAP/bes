#!/bin/sh
#
# This is part of the Hyrax TravisCI/CD build process. It is intended to be
# run from within the TravisCI build process and depends on the settings
# in the BES's .travis.yml file. However, it is run _inside a Docker
# container_ using values passed in by the .travis.yml file
#
# Thesae environment variables must be set:
#   - prefix
#   - HOME
#   - PATH
#   - OS
#   - DIST
#   - GDAL_OPTION
#   - BES_BUILD_NUMBER
#   - LIBDAP_RPM_VERSION
#
# run the script like this:
# docker run --env prefix=/root/install --volume $prefix/rpmbuild:/root/rpmbuild
#   --volume $TRAVIS_BUILD_DIR:/root/bes
#   --env LIBDAP_RPM_VERSION=$LIBDAP_RPM_VERSION
#   --env BES_BUILD_NUMBER=$BES_BUILD_NUMBER
#   --env GDAL_OPTION=$GDAL_OPTION
#   --env AWS_ACCESS_KEY_ID=$AWS_ACCESS_KEY_ID
#   --env AWS_SECRET_ACCESS_KEY=$AWS_SECRET_ACCESS_KEY
#   opendap/rocky9_hyrax_builder:latest /root/bes/travis/build-rh9-rpm.sh

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
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Defaults

OS="${OS:-"rocky9"}"

DIST="${DIST:-"el9"}"

GDAL_OPTION="${GDAL_OPTION:-"--with-gdal"}"

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

loggy "$HR"
loggy "$0 BEGIN"
loggy "Running inside the docker container"
loggy "    redhat-release: \"$(cat /etc/redhat-release)\""
loggy "            prefix: $prefix"
loggy "              HOME: $HOME"
loggy "              PATH: $PATH"
loggy "                OS: $OS"
loggy "              DIST: $DIST"
loggy "       GDAL_OPTION: $GDAL_OPTION"
loggy "  BES_BUILD_NUMBER: $BES_BUILD_NUMBER"
loggy "LIBDAP_RPM_VERSION: $LIBDAP_RPM_VERSION"



loggy "Updating..."
yum update -y

# This script will start with /root as the CWD since that's how the
# centos6/7 hyrax build containers are configured. The PATH will be 
# set to include $prefix/bin and $prefix/deps/bin; $prefix will be
# $HOME/install. $HOME is /root for the build container.

loggy "prefix: prefix"
loggy "  HOME: $HOME"
loggy "  PATH: $PATH"

hyrax_deps_tarball="hyrax-dependencies-rocky9-static.tgz"

# Get the pre-built dependencies (all static libraries).
loggy "Retrieving hyrax-dependencies for rocky9"
aws s3 cp "s3://opendap.travis.build/$hyrax_deps_tarball" /tmp/

# This dumps the dependencies in $HOME/install/deps/{lib,bin,...}
# The Centos7 dependencies are tarred so they include /root for a reason
# that escapes me. For CentOS Stream8, we have to CD to /root before expanding
# the tar ball to get the dependencies in /root/install. jhrg 2/11/22
tar -C /$HOME -xzvf "/tmp/$hyrax_deps_tarball"

ls -lR $HOME/install/deps

# Then get the libdap RPMs packages
# libdap-3.20.0-1.el6.x86_64.rpm libdap-devel-3.20.0-1.el6.x86_64.rpm
# $DIST is 'el6', 'el7', or 'el8'; $LIBDAP_RPM_VERSION is 3.20.0-1 (set by Travis)
aws s3 cp s3://opendap.travis.build/libdap-$LIBDAP_RPM_VERSION.$DIST.x86_64.rpm /tmp/
aws s3 cp s3://opendap.travis.build/libdap-devel-$LIBDAP_RPM_VERSION.$DIST.x86_64.rpm /tmp/

yum install -y /tmp/*.rpm

# cd to the $TRAVIS_BUILD_DIR directory. Note that we make $HOME/travis
# using the docker run --volume option and set it to $TRAVIS_BUILD_DIR.
cd $HOME/bes

# The build needs these environment variables because RHEL9.
# Those were added to the docker image via the tirpc package.
export CPPFLAGS=-I/usr/include/tirpc
export LDFLAGS=-ltirpc

autoreconf -fiv

echo "BES_BUILD_NUMBER: $BES_BUILD_NUMBER"
./configure --disable-dependency-tracking --prefix=$prefix --with-dependencies=$prefix/deps $GDAL_OPTION --with-build=$BES_BUILD_NUMBER

# set up the rpm tree in $HOME. We didn't need to do this for libdap because 
# rpmbuild took care of it, but for the BES, the Makefile copies the source 
# tarball and thus, we have to make the rpm dir tree. Reading the Makefile.am
# will reveal more...
mkdir -pv $HOME/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# This will leave the package in $HOME/rpmbuild/RPMS/x86_64/*.rpm
make -j16 all-static-rpm

# Sanity check for human reassurance.
ls -l $HOME/rpmbuild/RPMS/x86_64/
