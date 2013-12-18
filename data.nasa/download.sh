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

# GESDISC 
## OMI
### Swath
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMI-Aura_L2-OMBRO_2012m1204t1200-o44623_v003-2012m1204t185453.he5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMI-Aura_L2-OMNO2_2008m0720t2016-o21357_v003-2008m0721t101450.he5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMI-Aura_L2-OMUVB_2006m0104t0019-o07831_v003-2008m0716t0249.he5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMI-Aura_L2G-OMTO3G_2004m1001_v003-2009m0602t123920.he5
### Grid
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMI-Aura_L3-OMAERUVd_2009m0109_v003-2011m1203t141123.he5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMI-Aura_L3-OMSO2e_2012m0101_v003-2012m0103t014459.he5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMI-Aura_L3-OMAEROe_2012m0213_v003-2012m0215t021315.he5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/OMI_L3.nc4.h5

## MLS
### Single swath
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/MLS-Aura_L2GP-BrO_v02-23-c01_2010d255.he5
### Single swath - Near Real Time product
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/MLS-Aura_L2GP-O3_v03-40-NRT-06-c01_2013d024t0010.he5
### Multiple swaths
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/MLS-Aura_L2GP-Temperature_v03-33-c01_2011d316.he5


## HIRDLS
### Swath
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/HIRDLS-Aura_L2_v06-00-00-c01_2008d001.he5
### ZA
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/HIRDLS-Aura_L3ZAD_v06-00-00-c02_2005d022-2008d077.he5
### Level 3 Stratospheric Column
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/HIRDLS-Aura_L3SCOL_v06-00-00-c02_2005d022-2008d077.he5

## (S)BUV (MEaSUREs Ozone)
### Swath
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/SBUV2-NOAA17_L2-SBUV2N17L2_2011m1231_v01-01-2012m0905t152911.h5
### ZA
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/BUV-Nimbus04_L3zm_v01-00-2012m0203t144121.h5


## SWDB (MEaSUREs)
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/DeepBlue-SeaWiFS-1.0_L3_20100101_v002-20110527T191319Z.h5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/DeepBlue-SeaWiFS-1.0_L3_19970903_v003-20111127T185012Z.h5

## GSSTF (MEaSUREs)
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/GSSTF.2b.2008.01.01.he5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/GSSTFYC.3.Year.1988_2008.he5

## GOZCARD (MEaSUREs)
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/GOZ-Source-MLP_HCl_ev1-00_2005.nc4.h5

## AIRS(M)_CPR_MAT - Multi-Sensor Water Vapor with Cloud Climatology (MEaSUREs)
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/matched-airs.aqua_cloudsat-v3.1-2006.06.15.239_airs.nc4.h5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/matched-airs.aqua_cloudsat-v3.1-2011.03.11.001_amsu.nc4.h5

## GOSAT/acos
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GESDISC/acos_L2s_110419_43_Production_v110110_L2s2800_r01_PolB_110430192739.h5

# GSFC
## mabel (ICESat-2)
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/GSFC/mabel_l2a_20110322T165030_005_1.h5

# LaRC ASDC
## TES
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/LaRC/TES-Aura_L2-O3-Nadir_r0000011015_F05_07.he5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/LaRC/TES-Aura_L3-CH4_r0000010410_F01_07.he5

# NSIDC
## GLAS
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/NSIDC/GLAH13_633_2103_001_1317_0_01_0001.h5

# PO.DAAC
## Aquarius
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/PODAAC/Q2011149002900.L2_SCI_V1.0.bz2.0.bz2.0.h5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/PODAAC/Q20111722011263.L3b_SNSU_EVSCI_V1.2.main.h5
$GET ftp://ftp.hdfgroup.uiuc.edu/pub/outgoing/opendap/data/HDF5/NASA1/PODAAC/Q20103372010343.L3m_7D_SCIB1_V1.0_SSS_1deg.h5


