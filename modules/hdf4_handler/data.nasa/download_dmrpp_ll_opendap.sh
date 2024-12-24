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
#Cloud
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/AIRS.2024.01.01.L3.RetStd_IR001.v7.0.7.0.G24002230956.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/3B42.20180802.03.7.HDF.eos2ll.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MOD021KM.A2024024.0000.061.2024024014421.NRT.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MOD03.A2000166.0255.061.2017173092154.hdf.eos2ll.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/AMSR_E_L2_Land_V09_200206190615_A.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/AMSR_E_L3_SeaIce25km_V15_20020601.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MOD10A1F.A2024025.h27v04.061.2024027145105.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MOD29.A2000166.0255.005.2008189120917.hdf.eos2ll.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MOD11A1.A2024025.h10v06.061.2024028004317.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MCD12Q1.A2022001.h10v06.061.2023243073808.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MOD13Q1.A2023353.h17v06.061.2024005131728.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MCD19A1.A2024025.h10v06.061.2024027100206.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MCD43A4.A2012009.h25v05.061.2021202161101.hdf.eos2ll.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MCD43A4.A2012009.h25v05.061.2021202161101_ll_vars.nc
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/MCD43A4.A2012009.h25v05.061.2021202161101.hdf.eos2ll_nmd.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/nmd/AMSR_E_L3_SeaIce25km_V15_20020601.hdf.nmd.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/nmd/AMSR_E_L3_SeaIce25km_V15_20020601.hdf_mvs.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/nmd/MOD10A1F.A2024025.h27v04.061.2024027145105.hdf.nmd.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/DMRPP_LL_TESTS/nmd/MOD10A1F.A2024025.h27v04.061.2024027145105.hdf_mvs.h5
