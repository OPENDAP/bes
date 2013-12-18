#!/bin/sh
#
# This script downloads NASA test data from The HDF Group's FTP server.
#
# Check whether "wget" or "curl" is available first.
GET=""
command -v  wget > /dev/null && GET="wget -N --retr-symlinks" 
if [ -z "$GET" ]; then
  command -v  curl > /dev/null && GET="curl -O"
fi

if [ -z "$GET" ]; then
  echo "Neither wget nor curl found in your system."
  exit
fi
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF4/NASA1/GESDISC/AIRS.2009.11.30.240.L2.CO2_Std.v5.4.11.0.CO2.T09346091404.hdf
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF4/NASA1/GESDISC/AIRS.2002.08.01.L3.RetStd_H031.v4.0.21.0.G06104133732.hdf
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF4/NASA1/LAADS/MOD021KM.A2010277.1710.005.2010278082807.hdf
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF4/NASA1/LaRC/MISR_AEROSOL_P017_O036105_F10_0020_GOM_b64-72.hdf
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF4/NASA1/GESDISC/3B43.070901.6A.hdf
