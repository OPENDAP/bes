#!/bin/sh

set -e

# Add in a better test that looks at the version numbers of the stuff
# in hyrax-dependencies/downloads or src. Just get the basic build working
# for now.

# hyrax-dependencies appends '/deps' to 'prefix'
export prefix=$HOME
export PATH=$HOME/deps/bin:$PATH

# Force the build FIXME
rm -rf $HOME/deps

if test ! -d "$HOME/deps"
then
  wget http://www.opendap.org/pub/tmp/travis/hyrax-dependencies-1.11.2.tar
  tar -xf hyrax-dependencies-1.11.2.tar
  (cd hyrax-dependencies && make for-travis -j7 > /dev/null)
  echo "Completed dependency build - routed all messages to /dev/null"
else
    echo "Using cached hyrax-dependencies."
fi

# unlike hyrax-dependencies, the libdap tar needs --prefix to be the
# complete dir name. The hyrax-deps... project is a bit of a hack...

if test ! -x "$HOME/deps/bin/dap-config"
then
  wget http://www.opendap.org/pub/tmp/travis/libdap-3.15.0.tar.gz
  tar -xzf libdap-3.15.0.tar.gz
  (cd libdap-3.15.0 && ./configure --prefix=$prefix/deps/ && make -j7 && make install)
else
    echo "Using cached libdap."
fi
