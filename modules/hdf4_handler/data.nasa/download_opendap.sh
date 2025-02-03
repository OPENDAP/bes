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
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/AIRS.2008.10.27.L3.RetStd001.v5.2.2.0.G08303124144.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/AIRS.2013.12.08.001.L2.RetStd.v6.0.7.0.G13345141819.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/AIRS.2013.01.01.L3.RetStd001.v6.0.9.0.G13092214104.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LAADS/MOD021KM.A2010277.1710.005.2010278082807.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MYD09Q1.A2007001.h00v09.005.2007085230839.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MOD16A2.A2017145.h10v04.006.2017160232658.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MYD21A1N.A2018304.h06v03.006.2018327044516.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/MOD29.A2013196.1250.005.2013196195940.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/MOD29E1D.A2009340.005.2009341094922.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LaRC/MISR_AEROSOL_P017_O036105_F10_0020_GOM_b64-72.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/AMSR_E_L2A_BrightnessTemperatures_V10_200501180027_D.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/AMSR_E_L3_RainGrid_V06_200206.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/AMSR_E_L3_5DaySnow_V09_20050126.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/AMSR_E_L3_SeaIce6km_V11_20050118.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LaRC/MOP02-20120801-L2V8.2.2.prov.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LaRC/MOP03-200112220L3V2.0.1.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/NISE_SSMISF17_20110424.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/VIP01P4.A2010001.002.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/2A23.20120925.84650.7.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/3B43.20130901.7.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LaRC/CER_ISCCP-D2like-GEO_Composite_Beta1_023031.200510.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LaRC/CER_ES8_TRMM-PFM_Edition2_025021.20000229.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/MERRA300.prod.assim.tavg3_3d_chm_Nv.20120630.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/OBPG/T20000322000060.L3m_MO_NSST_4.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/OBPG/T2010001000000.L2_LAC_SST.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/PODAAC/2006001-2006005.s0454pfrt-bsst.hdf

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GHRC/LISOTD_HRAC_V2.2.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LAADS/MYD09.A2019003.2040.006.2019005020913.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LAADS/MYD09.A2019003.2040.006.2019005020913.hdf.dmrpp

#Cloud
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/AIRS.2024.01.01.L3.RetStd_IR001.v7.0.7.0.G24002230956.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/AIRS.2024.01.01.L3.RetStd_IR001.v7.0.7.0.G24002230956.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/3B42.20180802.03.7.HDF
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/GESDISC/3B42.20180802.03.7.HDF.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LAADS/MOD021KM.A2024024.0000.061.2024024014421.NRT.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LAADS/MOD021KM.A2024024.0000.061.2024024014421.NRT.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LAADS/MOD03.A2000166.0255.061.2017173092154.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LAADS/MOD03.A2000166.0255.061.2017173092154.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/AMSR_E_L2_Land_V09_200206190615_A.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/AMSR_E_L2_Land_V09_200206190615_A.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/AMSR_E_L3_SeaIce25km_V15_20020601.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/AMSR_E_L3_SeaIce25km_V15_20020601.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/MOD10A1F.A2024025.h27v04.061.2024027145105.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/MOD10A1F.A2024025.h27v04.061.2024027145105.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/MOD29.A2000166.0255.005.2008189120917.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/NSIDC/MOD29.A2000166.0255.005.2008189120917.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MOD11A1.A2024025.h10v06.061.2024028004317.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MOD11A1.A2024025.h10v06.061.2024028004317.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MCD12Q1.A2022001.h10v06.061.2023243073808.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MCD12Q1.A2022001.h10v06.061.2023243073808.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MOD13Q1.A2023353.h17v06.061.2024005131728.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MOD13Q1.A2023353.h17v06.061.2024005131728.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MCD19A1.A2024025.h10v06.061.2024027100206.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MCD19A1.A2024025.h10v06.061.2024027100206.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MCD43A4.A2012009.h25v05.061.2021202161101.hdf
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MCD43A4.A2012009.h25v05.061.2021202161101.hdf.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF4/NASA1/LPDAAC/MCD43GF_bsa_Band1_365_2021_V061.hdf 
