#besstandalone -d"cerr,all" -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/grid_1_2d.h5.dmr.bescmd
besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/testsds1.hdf.dds.bescmd
#valgrind --leak-check=full besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/grid_1_2d.h5.dds.bescmd
#besstandalone -c /etc/bes/bes.conf -i bes-testsuite/h4.nasa.with_hdfeos2/AMSR_E_L3_RainGrid_V06_200206.hdf.dds.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.nasa.with_hdfeos2/MERRA300.prod.assim.tavg3_3d_chm_Nv.20120630.hdf.data.bescmd | getdap -M -

