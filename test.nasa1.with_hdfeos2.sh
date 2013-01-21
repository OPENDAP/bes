#!/bin/tcsh
#
# Test CF option for important and supported NASA test files.
# 
# Set up the local test environment.
set path=(/hdfdap/hyrax-1.8.0/bin $path)
setenv LD_LIBRARY_PATH /hdfdap/hyrax-1.8.0/lib
setenv PKG_CONFIG_PATH /hdfdap/hyrax-1.8.0/lib/pkgconfig:/usr/lib/pkgconfig/

# This is required for sample_data_DATA dependency in Makefile.am.
cp data/*.gz data.nasa1/

# Save the original test data directory.
mv data data.orig
mv data.nasa1 data

# Save the original .at file.
mv bes-testsuite/hdf4_handlerTest.with_hdfeos2.at bes-testsuite/hdf4_handlerTest.at.orig

# Copy the .at file for NASA priroity 1 files only.
cp bes-testsuite/hdf4_handlerTest.nasa1.with_hdfeos2.at bes-testsuite/hdf4_handlerTest.with_hdfeos2.at

# Configure with HDF-EOS2 library.
./configure --prefix=/hdfdap/hyrax-1.8.0  --with-hdf4=/hdfdap/local --with-hdfeos2=/hdfdap/local
make
make check >& check_test.nasa1.with_hdfeos2.txt
mv bes-testsuite/hdf4_handlerTest.log check_test.nasa1.with_hdfeos2.log.txt
make distclean

# Restore the original test environment.
mv bes-testsuite/hdf4_handlerTest.at.orig bes-testsuite/hdf4_handlerTest.with_hdfeos2.at
mv data data.nasa1
mv data.orig data
rm data.nasa1/*.gz



