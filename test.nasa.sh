#!/bin/tcsh
#
# Copyright (C) 2013 The HDF Group
# All rights reserved
#
# 
set path=(/hdfdap/hyrax-1.8.0/bin $path)
setenv LD_LIBRARY_PATH /hdfdap/hyrax-1.8.0/lib
setenv PKG_CONFIG_PATH /hdfdap/hyrax-1.8.0/lib/pkgconfig:/usr/lib/pkgconfig/

# Set up the local test environment.
cp data/grid_1_2d.h5 data.nasa/
mv data data.orig
mv data.nasa data
mv bes-testsuite/hdf5_handlerTest.at bes-testsuite/hdf5_handlerTest.at.orig
cp bes-testsuite/hdf5_handlerTest.nasa.at bes-testsuite/hdf5_handlerTest.at

# Test CF option for NASA products.
./configure  --with-hdf5=/hdfdap/hyrax-1.8.0
make
make check >& test.nasa.txt
mv bes-testsuite/hdf5_handlerTest.log test.nasa.log.txt

make distclean

# Restore the original test environment.
mv bes-testsuite/hdf5_handlerTest.at.orig bes-testsuite/hdf5_handlerTest.at
mv data data.nasa
mv data.orig data
rm data.nasa/grid_1_2d.h5
grep 'All' test.nasa.txt
grep 'failed unexpectedly' test.nasa.txt



