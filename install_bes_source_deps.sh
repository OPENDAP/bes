#!/bin/sh

set -e

# we could test for versions...

if test ! -x "$HOME/deps/bin/bison
then
  wget http://ftp.gnu.org/gnu/bison/bison-3.0.4.tar.gz
  tar -xzf bison-3.0.4.tar.gz
  (cd bison-3.0.4 && ./configure --prefix=$HOME/deps/ && make -j7 && make install)
else
    echo "Using cached bison."
fi

if test ! -x "$HOME/deps/bin/dap-config
then
  wget http://www.opendap.org/pub/tmp/libdap-3.15.0.tar.gz
  tar -xzf libdap-3.15.0.tar.gz
  (cd libdap-3.15.0 && ./configure --prefix=$HOME/deps/ && make -j7 && make install)
else
    echo "Using cached libdap."
fi

if test ! -x "$HOME/deps/bin/h5ls
then
  wget http://www.opendap.org/pub/tmp/hdf5-1.8.15-patch1.tar.gz
  tar -xzf hdf5-1.8.15-patch1.tar.gz
  (cd hdf5-1.8.15-patch1 && ./configure --prefix=$HOME/deps/ CFLAGS="-fPIC -O2 -w" && make -j7 && make install)
else
  echo "Using cached libhdf5."
fi
