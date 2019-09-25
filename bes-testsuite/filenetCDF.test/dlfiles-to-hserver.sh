#!/bin/sh

#The path BES is installed must be provided explicitly.
if [ -z "$BESPATH" ]; then
    echo Environment variable BESPATH should be set. It should be the BES path the handler is linked with.
    exit
fi

FNC_NOCLEANUP="yes"

# Use this path to figure out the location of besstandalone, bes.conf and the data.
BESSTANDALONE="$BESPATH/bin/besstandalone"
BESCONF="$BESPATH/etc/bes/bes.conf"
BESH5SHARE="$BESPATH/share/hyrax/data/hdf5"

#1. Remember the current directory
CURDIR=`pwd`
echo $CURDIR

#2. Obtain HDF5 files
cd $BESH5SHARE
SHAREDIR=`pwd`
echo $SHAREDIR
GET=""
command -v  wget > /dev/null && GET="wget -N --retr-symlinks"
if [ -z "$GET" ]; then
  command -v  curl > /dev/null && GET="curl -O"
fi

if [ -z "$GET" ]; then
  echo "Neither wget nor curl found in your system."
  exit
fi

FNC_CP_TEST="fnc_nasa_test"
mkdir $FNC_CP_TEST
cd $FNC_CP_TEST

#NASA files
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/LPDAAC/VNP09A1.A2015257.h29v11.001.2016221164845.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/NASA1/NSIDC/SMAP_L3_SM_P_20150406_R14010_001.h5

#Default files
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/hdf5_handler/grid_1_2d.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/hdf5_handler/dim_scale.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/opendap/data/HDF5/hdf5_handler/t_2d_2dll.nc4.h5

#Remove all the HDF5 files under BES hyrax share directory and the generated NC files
if [ -z "$FNC_NOCLEANUP" ]; then 
        cd $SHAREDIR
        rm -rf $FNC_CP_TEST 
fi

