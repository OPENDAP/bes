#!/bin/tcsh
#
# Copyright (C) 2013 The HDF Group
# All rights reserved
#
# Test CF option for a small set of NASA test files.
# 
# Please read section 7.2 in INSTALL document to know what this script does.
# 
# This script assumes that the latest libdap, bes, hdf4, and hdfeo2 software 
# are installed under /hdfdap/hyrax directory.

# Set up the local test environment.
set path=(/hdfdap/hyrax/bin $path)
setenv LD_LIBRARY_PATH /hdfdap/hyrax/lib
setenv PKG_CONFIG_PATH /hdfdap/hyrax/lib/pkgconfig:/usr/lib/pkgconfig/

# This is required for "sample_data_DATA" dependency in Makefile.am.
cp data/*.gz data.nasa/

# Save the original test data directory.
mv data data.orig
mv data.nasa data

# Save the original .at file.
mv bes-testsuite/hdf4_handlerTest.with_hdfeos2.at bes-testsuite/hdf4_handlerTest.at.orig

# Copy the .at file for small NASA sample files.
cp bes-testsuite/hdf4_handlerTest.nasa.with_hdfeos2.at bes-testsuite/hdf4_handlerTest.with_hdfeos2.at

# Configure with HDF-EOS2 library.
./configure --prefix=/hdfdap/hyrax  --with-hdf4=/hdfdap/hyrax --with-hdfeos2=/hdfdap/hyrax
make
make check >& test.nasa.with_hdfeos2.txt
mv bes-testsuite/hdf4_handlerTest.log test.nasa.with_hdfeos2.log.txt
make distclean

# Restore the original test environment.
mv bes-testsuite/hdf4_handlerTest.at.orig bes-testsuite/hdf4_handlerTest.with_hdfeos2.at
mv data data.nasa
mv data.orig data
rm data.nasa/*.gz

grep 'All' test.nasa.with_hdfeos2.txt
grep 'failed unexpectedly' test.nasa.with_hdfeos2.txt


