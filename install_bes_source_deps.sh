#!/bin/sh

set -e

# Add in a better test that looks at the version numbers of the stuff
# in hyrax-dependencies/downloads or src. Just get the basic build working
# for now.

if test ! -d "$HOME/deps/lib"
then
  wget http://www.opendap.org/pub/tmp/travis/hyrax-dependencies-1.11.2.tar
  tar -xzf hyrax-dependencies-1.11.2.tar
  (cd hyrax-dependencies && make -j7 prefix=$HOME/deps)
else
    echo "Using cached hyrax-dependencies."
fi

# if test ! -x "$HOME/deps/bin/bison"
# then
#   wget http://ftp.gnu.org/gnu/bison/bison-3.0.4.tar.gz
#   tar -xzf bison-3.0.4.tar.gz
#   (cd bison-3.0.4 && ./configure --prefix=$HOME/deps/ && make -j7 && make install)
# else
#     echo "Using cached bison."
# fi

if test ! -x "$HOME/deps/bin/dap-config"
then
  wget http://www.opendap.org/pub/tmp/travis/libdap-3.15.0.tar.gz
  tar -xzf libdap-3.15.0.tar.gz
  (cd libdap-3.15.0 && ./configure --prefix=$HOME/deps/ && make -j7 && make install)
else
    echo "Using cached libdap."
fi

# if test ! -x "$HOME/deps/bin/h5ls"
# then
#   wget http://www.opendap.org/pub/tmp/hdf5-1.8.15-patch1.tar.gz
#   tar -xzf hdf5-1.8.15-patch1.tar.gz
#   (cd hdf5-1.8.15-patch1 && ./configure --prefix=$HOME/deps/ CFLAGS="-fPIC -O2 -w" && make -j7 && make install)
# else
#   echo "Using cached libhdf5."
# fi
