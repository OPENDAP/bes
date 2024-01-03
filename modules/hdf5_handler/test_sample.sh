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

#For the netCDF handler test. This should be done under modules/netcdf_handler ,they are from the netcdf handler testsuite.
#besstandalone -c tests/bes.conf -i tests/nc/coads_climatology.nc.dmr.bescmd
#besstandalone -c tests/bes.conf -i tests/nc/coads_climatology.nc.2.bescmd | getdap -M -

#For the fileout coverage json test. This should be done under modules/fileout_covjson,they are from the fileout covjson module. 
#besstandalone -c tests/bes.conf -i tests/covjson/coads_climatology_abstract_object_METADATA.bescmd 
#besstandalone -c tests/bes.conf -i tests/covjson/fnoc1_abstract_object_DATA.bescmd 


#besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.cf/grid_1_2d.h5.das.bescmd
#besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.cf/grid_1_2d.h5.data.bescmd | getdap -M -
besstandalone -c bes-testsuite/bes.cfdmr.conf -i bes-testsuite/h5.cf/d_int64.h5.dmr.bescmd
#besstandalone -d"cerr,all" -c bes-testsuite/bes.conf -i bes-testsuite/h5.cf/grid_1_2d_int64.h5.dmr.bescmd
#besstandalone -c bes-testsuite/bes.default.conf -i bes-testsuite/h5.default/d_int.h5.dap.bescmd | getdap4 -D -M -
#besstandalone -c bes-testsuite/bes.default.conf -i bes-testsuite/h5.default/d_int.h5.dmr.bescmd
#besstandalone -c bes-testsuite/bes.default.conf -i bes-testsuite/h5.default/nc4_coverage_special.h5.dmr.bescmd
#valgrind besstandalone -c bes-testsuite/bes.conf -i bes-testsuite/h5.cf/grid_1_2d_int64.h5.dmr.bescmd
