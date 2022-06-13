#!/bin/sh
#
# This script downloads NASA test data from The HDF Group's FTP server.
#
# Check whether "wget" or "curl" is available first.
GET=""
command -v  wget > /dev/null && GET="wget -N --retr-symlinks" 
if [ -z "$GET" ]; then
  command -v  curl > /dev/null && GET="curl -O -C -"
fi

if [ -z "$GET" ]; then
  echo "Neither wget nor curl found in your system."
  exit
fi

# GESDISC 
### Grid
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/GLDAS_NOAH025_3H.A20210101.0000.021.nc4.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF4/NASA1/GESDISC/AIRS.2020.01.01.L3.RetStd_IR031.v7.0.4.0.G20336214947.hdf

