valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/OMI_L3.nc4.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/1A.GPM.GMI.COUNT2014v3.20140305-S061920-E075148.000087.V03A.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/2A.GPM.GMI.GPROF2014v1-4.20140921-S002001-E015234.003195.V03C.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/A20030602003090.L3m_MO_AT108_CHL_chlor_a_4km.h5.bescmd >nasa_dap4_default_test.nc
#AirMSPI_ER2_GRP_ELLIPSOID_20161006_181726Z_CA-NewberrySprings_SWPF_F01_V006.he5.bescmd
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/Arctas-car_p3b_20080407_2002_Level1C_20171121.nc.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/ATL03_20181014084920_02400109_003_01.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/ATL08_20181014084920_02400109_003_01.h5.bescmd >nasa_dap4_default_test.nc
# Comment out this test since the dataset contains fixed-size chunked string arrays and the dmrpp module generates memory leaks
# when accessing a fixed-size chunked string array. This is documented in the ticket https://bugs.earthdata.nasa.gov/browse/HYRAX-1225
# We will resume this test until the above ticket is fixed.
# The variable name is /ancillary_data/control, a fixed-size string array.
#valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/ATL13_20190330212241_00250301_002_01.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/DeepBlue-SeaWiFS-1.0_L3_20100613_v004-20130604T133539Z.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/DeepBlue-SeaWiFS_L2_20100101T003505Z_v004-20130524T141300Z.h5.bescmd >nasa_dap4_default_test.nc
#    GLAH13_633_2103_001_1317_0_01_0001.h5.bescmd
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/GOZ-Source-MLP_HCl_ev1-00_2005.nc4.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/LPRM-AMSR2_L2_D_SOILM2_V001_20120702231838.nc4.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/LPRM-AMSR2_L3_A_SOILM3_V001_20121216010911.nc4.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/mabel_l2a_20110322T165030_005_1.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/matched-airs.aqua_cloudsat-v3.1-2006.06.15.239_airs.nc4.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/matched-airs.aqua_cloudsat-v3.1-2011.03.11.001_amsu.nc4.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/OMI-Aura_L2-OMIAuraAER_2006m0815t130241-o11086_v01-00-2018m0529t115547.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/Q20103372010343.L3m_7D_SCIB1_V1.0_SSS_1deg.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/Q2011149002900.L2_SCI_V1.0.bz2.0.bz2.0.h5.bescmd >nasa_dap4_default_test.nc
#Q20111722011263.L3b_SNSU_EVSCI_V1.2.main.h5.bescmd
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/S20030602003090.L3m_MO_ST92_CHL_chlor_a_9km.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/S5PNRTIL2NO220180422T00470920180422T005209027060100110820180422T022729.nc.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/SMAP_L1C_S0_HIRES_02298_A_20150707T160502_R13080_001.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/SMAP_L3_SM_P_20150406_R14010_001.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/TOMS-N7_L2-TOMSN7AERUV_1991m0630t0915-o64032_v02-00-2015m0918t123456.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/GLAH06_634_2113_002_0152_2_01_0001.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/S-MODE_PFC_OC2108A_adcp_os75nb.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/20220930120000-REMSS-L4_GHRSST-SSTfnd-MW_OI-GLOB-v02.0-fv05.0.nc.h5.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/20220930120000-REMSS-L4_GHRSST-SSTfnd-MW_OI-GLOB-v02.0-fv05.0.nc.h5.dmrpp.bescmd >nasa_dap4_default_test.nc

valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/OMG_Bathy_SBES_L2_20150804000000.h5.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/SWOT_L2_HR_Raster_250m_UTM50V_N_x_x_x_406_023_131F_20230121T040652_20230121T040653_PIA0_01.nc.h5.bescmd >nasa_dap4_default_test.nc

valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/S5P_OFFL_L1B_RA_BD8_20180430T001950_20180430T020120_02818_01_010000_20180430T035011.nc.h5.dmrpp.bescmd >nasa_dap4_default_test.nc
#direct IO check
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/20220930120000-REMSS-L4_GHRSST-SSTfnd-MW_OI-GLOB-v02.0-fv05.0.nc.h5.dio.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/test_ba_grp_dim_whole.h5.deflev.dmrpp.bescmd >nasa_dap4_default_test.nc
## Comment out this test since the dataset contains fixed-size chunked string arrays and the dmrpp module generates memory leaks
# when accessing a fixed-size chunked string array. This is documented in the ticket https://bugs.earthdata.nasa.gov/browse/HYRAX-1225
# We will resume this test until the above ticket is fixed.
# The variable name is /ancillary_data/control, a fixed-size string array.
#valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/ATL13_20190330212241_00250301_002_01.h5_dio.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/OMPS-NPP_NMTO3-L3-DAILY_v2.1_2018m0102_2018m0104t012837.h5_dio.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/OMG_Bathy_SBES_L2_20150804000000.h5_dio.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/SWOT_L2_HR_Raster_250m_UTM50V_N_x_x_x_406_023_131F_20230121T040652_20230121T040653_PIA0_01.nc.h5_dio.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/daymet_v4_daily_na_prcp_2010.nc.h5_dio.dmrpp.bescmd >nasa_dap4_default_test.nc
# Comment out this test since the dataset contains fixed-size chunked string arrays and the dmrpp module generates memory leaks
# when accessing a fixed-size chunked string array. This is documented in the ticket https://bugs.earthdata.nasa.gov/browse/HYRAX-1225
# We will resume this test until the above ticket is fixed.
# valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/SMAP_L3_SM_P_20150406_R14010_001.h5_dio.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/big_1d_shuf.h5_dio.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/d_dset_4d.h5_dio.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/AIRS.2024.01.01.L3.RetStd_IR001.v7.0.7.0.G24002230956.hdf.dmrpp.bescmd >nasa_dap4_default_test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/nasa-bescmd/GEDI04.dmrpp.bescmd >nasa_dap4_default_test.nc



