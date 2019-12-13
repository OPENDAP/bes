#besstandalone -d"cerr,all" -c bes-testsuite/bes.conf -i bes-testsuite/h5.he5/grid_1_2d.h5.dmr.bescmd
#valgrind --leak-check=full besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.nasa/3A-MO.GPM.GMI.GRID2014R1.20140601-S000000-E235959.06.V03A.h5.dds.bescmd
#besstandalone -c /etc/bes/bes.conf -i bes-testsuite/h4.nasa1.with_hdfeos2/MOD09GA.A2007268.h10v08.005.2007272184810.hdf.das.bescmd1
#besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h4.nasa1.with_hdfeos2/MOD09GA.A2007268.h10v08.005.2007272184810.hdf.data.bescmd | getdap -M -
#valgrind besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.he5/grid_1_2d.h5.dmr.bescmd
#besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.he5/grid_1_2d.h5.dap.bescmd | getdap4 -D -M -
#valgrind -v besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.he5/grid_1_2d.h5.dap.bescmd
#besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.cf/grid_1_2d.h5.dds.bescmd
#besstandalone -c bes-testsuite/bes.default.conf -i bes-testsuite/h5.default/d_int.h5.das.bescmd
#besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.nasa/OMPS-NPP_LP-L2-O3-DAILY_v2.5_2019m1208_2019m1209t143730.h5.nc.bescmd>OMPS-NPP_LP-L2-O3-DAILY_v2.5_2019m1208_2019m1209t143730.nc
besstandalone -d"cerr,fonc" -c bes-testsuite/bes.conf -i bes-testsuite/h5.nasa/OMPS-NPP_LP-L2-O3-DAILY_v2.5_2019m1208_2019m1209t143730.h5.nc4.bescmd>OMPS-NPP_LP-L2-O3-DAILY_v2.5_2019m1208_2019m1209t143730.nc4

#For the NCML module test. This should be done under modules/ncml_module,the first two are from the ncml_module testsuite.
#besstandalone -c tests/bes.conf -i tests/dmrpp/dmrpp_join_new.bescmd   | getdap -M -
#besstandalone -c tests/bes.conf -i tests/agg_3_tif.bescmd >agg_3_tif.nc

#This one doesn't work. However if you change the aggNew.ncml to aggExisting.ncml, this will work.
#besstandalone -c tests/bes.conf -i tests/agg_nc.bescmd
#gpm tests.
#besstandalone -c tests/bes.conf -i tests/agg_gpm_ncml_dds.bescmd
#valgrind --tool=callgrind besstandalone -c tests/bes.conf -i tests/agg_gpm_ncml.bescmd | getdap -M -

#For the DMRPP module test. This should be done under modules/dmrpp_module,the first two are from the dmrpp_module testsuite.
#besstandalone  -c tests/bes.conf -i tests/chunked/chunked_fourD.h5.dods | getdap -M -
#besstandalone  -d"cerr,all" -c tests/bes.conf -i tests/chunked/chunked_fourD.h5.dods | getdap -M -
#GPM test
#besstandalone  -c tests/bes.conf -i GPMtest.h5.dods | getdap -M -
#besstandalone  -c tests/bes.conf -i GPM-aggr-test.h5.dods | getdap -M -
#besstandalone  -c /opt/kent/opendap/opendapbin/etc/bes/bes.conf -i tests/chunked/chunked_fourD.h5.dmrpp.fnc4 >chunked_fourD.h5.dmrpp.nc4

#For the fileout netCDF test. This should be done under modules/fileout_netcdf,they are from the fileout netcdf testsuite.
#besstandalone -c tests/bes.conf -i tests/bescmd/testFillValue.bescmd >OMPS-NPP.nc
#besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.2.bescmd >test.nc
#besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.3.bescmd >test.nc4





