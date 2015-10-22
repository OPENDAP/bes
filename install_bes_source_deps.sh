#!/bin/sh
#
# Build the dependencies for the Travis-CI build of the BES and all of
# its modules that are distributed with Hyrax.
#
# Two things about this script: 1. It builds both the dependencies for
# a complete set of Hyrax modules and that is quite taxing for the Travis
# system since Travis allows logs of 4MB or less. When the log hits the
# 4MB size, Travis stops the build. To get around that limitation, I send
# stdout output of the hyrax deps build to /dev/null. The output to stderr
# still shows up in the log, however, and that turns out to be important
# since output has to appear once every X minutes (5, 10?) or the build 
# will be stopped.
# 2. We have tired building using Ubuntu packages, but the Ubuntu 12 pkgs
# are just not current enough for Hyrax. We could drop a handful of the
# deps built here and get them from packages, but it's not enough to make
# a big difference. Also, building this way mimics what we will do when it's
# time to make the release RPMs.

set -e

# hyrax-dependencies appends '/deps' to 'prefix'
export prefix=$HOME
export PATH=$HOME/deps/bin:$PATH

# Force the build by un-commenting the following line
# rm -rf $HOME/deps

if test ! -d "$HOME/deps"
then
    wget http://www.opendap.org/pub/tmp/travis/hyrax-dependencies-1.11.2.tar
    tar -xf hyrax-dependencies-1.11.2.tar
    (cd hyrax-dependencies && make for-travis -j7 > /dev/null)
    echo "Completed dependency build - stdout to /dev/null to save space"
else
    echo "Using cached hyrax-dependencies."
fi

# unlike hyrax-dependencies, the libdap tar needs --prefix to be the
# complete dir name. The hyrax-deps... project is a bit of a hack...

if test ! -x "$HOME/deps/bin/dap-config" -o ! "`dap-config --version`" = "libdap 3.16.0"
then
    wget http://www.opendap.org/pub/tmp/travis/libdap-3.16.0.tar.gz
    tar -xzf libdap-3.16.0.tar.gz
    (cd libdap-3.16.0 && ./configure --prefix=$prefix/deps/ && make -j7 && make install)
else
    echo "Using cached libdap."
fi
