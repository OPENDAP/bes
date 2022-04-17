#besstandalone -d"cerr,all" -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/grid_1_2d.h5.dmr.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/testsds1.hdf.dds.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/MYD09.data.bescmd 
#valgrind besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/MYD09.data.bescmd | getdap -M -
#besstandalone -d"cerr,all" -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/MYD09.data.bescmd | getdap -M -
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/MYD09.dds.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/MYD09.das.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/swath_3_3d_dimmap.das.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/swath_3_3d_dimmap.dds.bescmd
#valgrind besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/SwathFile.hdf.dds.bescmd
#valgrind besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/SwathFile.hdf.das.bescmd
#valgrind --leak-check=full besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/swath_3_3d_dimmap.dds.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/swath_3_3d_dimmap.dds.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.nasa.with_hdfeos2/CER_ES8_TRMM-PFM_Edition2_025021.20000229.hdf.das.bescmd

#besstandalone -d"cerr,all" -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/MYD09.data.bescmd | getdap -M -
#valgrind --leak-check=full besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/grid_1_2d.h5.dds.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/SwathFile.hdf.dds.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/SwathFile.hdf.das.bescmd
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/SwathFile.hdf.s.data.bescmd 
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.with_hdfeos2/SwathFile.hdf.s.data.bescmd | getdap -M -
#valgrind besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/swath_3_3d_dimmap.data.bescmd | getdap -M -
#besstandalone -c bes-testsuite/bes.with_hdfeos2.conf -i bes-testsuite/h4.local/swath_3_3d_dimmap.data.bescmd 
#
