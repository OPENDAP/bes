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
## OMI
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/OMI_L3.nc4.h5

## GOZCARD (MEaSUREs)
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/GOZ-Source-MLP_HCl_ev1-00_2005.nc4.h5

## AIRS(M)_CPR_MAT - Multi-Sensor Water Vapor with Cloud Climatology (MEaSUREs)
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/matched-airs.aqua_cloudsat-v3.1-2006.06.15.239_airs.nc4.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/matched-airs.aqua_cloudsat-v3.1-2011.03.11.001_amsu.nc4.h5

# GSFC
## mabel (ICESat-2)
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GSFC/mabel_l2a_20110322T165030_005_1.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/LaRC/AirMSPI_ER2_GRP_ELLIPSOID_20161006_181726Z_CA-NewberrySprings_SWPF_F01_V006.he5

# LaRC
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/LaRC/MOP03T-20221002-L3V5.9.1.he5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/LaRC/MOP03TM-202209-L3V95.9.1.he5

# NSIDC
## GLAS
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/GLAH13_633_2103_001_1317_0_01_0001.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/GLAH06_634_2113_002_0152_2_01_0001.h5

# PO.DAAC
## Aquarius
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/Q2011149002900.L2_SCI_V1.0.bz2.0.bz2.0.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/Q20111722011263.L3b_SNSU_EVSCI_V1.2.main.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/Q20103372010343.L3m_7D_SCIB1_V1.0_SSS_1deg.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/S-MODE_PFC_OC2108A_adcp_os75nb.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/OMG_Bathy_SBES_L2_20150804000000.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/OMG_Bathy_SBES_L2_20150804000000.h5.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/20160409062000-OSPO-L2P_GHRSST-SSTsubskin-VIIRS_NPP-ACSPO_V2.61-v02.0-fv01.0.nc.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/SWOT_L2_HR_Raster_250m_UTM50V_N_x_x_x_406_023_131F_20230121T040652_20230121T040653_PIA0_01.nc.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/20220930120000-REMSS-L4_GHRSST-SSTfnd-MW_OI-GLOB-v02.0-fv05.0.nc.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/20220930120000-REMSS-L4_GHRSST-SSTfnd-MW_OI-GLOB-v02.0-fv05.0.nc.h5.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/20220930120000-REMSS-L4_GHRSST-SSTfnd-MW_OI-GLOB-v02.0-fv05.0.nc.h5.dio.dmrpp


#GESDISC GPM
# GPM files. 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/1A.GPM.GMI.COUNT2014v3.20140305-S061920-E075148.000087.V03A.h5 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/2A.GPM.GMI.GPROF2014v1-4.20140921-S002001-E015234.003195.V03C.h5 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/3B-HHR.MS.MRG.3IMERG.20200101-S000000-E002959.0000.V06B.HDF5 
 
# HFVHANDLER-129 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/LPRM-AMSR2_L2_D_SOILM2_V001_20120702231838.nc4.h5 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/LPRM-AMSR2_L3_A_SOILM3_V001_20121216010911.nc4.h5 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/OBPG/A20030602003090.L3m_MO_AT108_CHL_chlor_a_4km.h5 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/OBPG/S20030602003090.L3m_MO_ST92_CHL_chlor_a_9km.h5 

#2-D lat/lon netCDF-4 like file
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/TOMS-N7_L2-TOMSN7AERUV_1991m0630t0915-o64032_v02-00-2015m0918t123456.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/S5PNRTIL2NO220180422T00470920180422T005209027060100110820180422T022729.nc.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/OMI-Aura_L2-OMIAuraAER_2006m0815t130241-o11086_v01-00-2018m0529t115547.h5

#SWDB files for testing the memory cache
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/DeepBlue-SeaWiFS-1.0_L3_20100613_v004-20130604T133539Z.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/DeepBlue-SeaWiFS_L2_20100101T003505Z_v004-20130524T141300Z.h5

#OMPS-NPP level 3 daily(long variable names)
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5

#Arctas-Car
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/Arctas-car_p3b_20080407_2002_Level1C_20171121.nc.h5

#SMAP level 3
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/SMAP_L3_SM_P_20150406_R14010_001.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/NASAHDF/SMAP_L1C_S0_HIRES_02298_A_20150707T160502_R13080_001.h5

#LPDAAC GEDI level 2: contains compound datatype data
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/LPDAAC/GEDI04_A_2019107224731_O01958_01_T02638_02_002_02_V002.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/LPDAAC/GEDI04.dmrpp
$
#ICESAT-2 ATL 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/ATL03_20181014084920_02400109_003_01.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/ATL08_20181014084920_02400109_003_01.h5
$GET https://gamma.hdfgroup.org/ftp/pub/outgoing/NASAHDF/ATL13_20190330212241_00250301_002_01.h5
#daymet, large variable
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/ORNL/daymet_v4_daily_na_prcp_2010.nc.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/daymet_v4_daily_na_prcp_2010.nc.h5.dmrpp

#CLARREO 
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/LaRC/CLARREO_CERES_NPP_L4.SIM_b001.20170101.091658.PT04M54S.20230307154549.nc.h5

#NISAR ENUM
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/ASF/OPERA_L2_RTC-S1_T102-217155-IW1_20240703T162341Z_20240703T220516Z_S1A_30_v1.0.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/ASF/NISAR_L0_PR_RRSD_001_005_A_128S_20081012T060910_20081012T060926_P01101_F_J_001.h5

#unlimited dimensions and structure
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/LAADS/XAERDT_L2_MODIS_Aqua.A2022365.2355.001.2023250134344.nc
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/LAADS/XAERDT_L2_MODIS_Aqua.A2022365.2355.001.2023250134344.nc.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/S5P_OFFL_L1B_IR_SIR_20210630T172226_20210630T190356_19243_01_010000_20210630T205101.nc
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/S5P_OFFL_L1B_IR_SIR_20210630T172226_20210630T190356_19243_01_010000_20210630T205101.nc.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/AMSR_E_L2_Land_V11_201110031920_D.he5.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/AMSR_E_L2_Land_V11_201110031920_D.he5


#HDF5/DMRPP/fileout netCDF  big array size
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_big.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_big.h5.dmr
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_big.h5.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_big.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_4d.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_4d_2.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_4d_3.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_4d_4.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/test_ba_grp_dim_whole.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/test_ba_grp_dim_whole.h5.cf.dmr
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/test_ba_grp_dim_whole.h5.cf.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/test_ba_grp_dim_whole.h5.default.dmr
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/test_ba_grp_dim_whole.h5.default.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/test_ba_grp_dim_whole.h5.deflev.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/t_a_b.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/t_a_b.h5.dmr
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/t_a_b.h5.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_big_1d_cont.h5.gz
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_big_1d_cont.h5.dmr
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_big_1d_cont.h5.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/big_1d_shuf.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/big_1d_shuf.h5.dmr
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/big_1d_shuf.h5.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/big_1d_shuf_be.h5
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/big_1d_shuf_be.h5.dmr
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/big_1d_shuf_be.h5.dmrpp

#direct IO dmrpp files
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/big_1d_shuf.h5_dio.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/d_dset_4d.h5_dio.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/others/daymet_v4_daily_na_prcp_2010.nc.h5_dio.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/SMAP_L3_SM_P_20150406_R14010_001.h5_dio.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/NSIDC/ATL13_20190330212241_00250301_002_01.h5_dio.dmrpp

$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/GESDISC/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5_dio.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/OMG_Bathy_SBES_L2_20150804000000.h5_dio.dmrpp
$GET http://test.opendap.org/opendap/nasa-hdf/opendap/HDF5/NASA1/PODAAC/SWOT_L2_HR_Raster_250m_UTM50V_N_x_x_x_406_023_131F_20230121T040652_20230121T040653_PIA0_01.nc.h5_dio.dmrpp

if [ ! -e d_dset_big_1d_cont.h5 ]; then
    gunzip d_dset_big_1d_cont.h5.gz
fi
