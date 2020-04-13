valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.3.bescmd >test.nc

#dnl The same data (bescmd/same .dods response but with different constraints
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.5.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.6.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.7.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT.0.bescmd 
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT01.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT01.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT01.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT01.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/cedar.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/cedar.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/cedar.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/cedar.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/fits.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/fits.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/fits.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/fits.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/namesT.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/namesT.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/namesT.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/namesT.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT00.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT00.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT00.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT00.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT01.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT01.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT01.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT01.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT02.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT02.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT02.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT02.3.bescmd >test.nc

#AT_BESCMD_RESPONSE_TEST(bescmd/.0.bescmd
#AT_BESCMD_BINARYDATA_RESPONSE_TEST(bescmd/.1.bescmd
#AT_BESCMD_NETCDF_RESPONSE_TEST(bescmd/.2.bescmd
#AT_BESCMD_NETCDF_RESPONSE_TEST(bescmd/.3.bescmd

#dnl This test reads attributes from a .das in addition to data from 
#dnl a .dods file. Its baselines are interchangable with the fnoc1.nc
#dnl data file (bescmd/but that requires the netcdf handler.

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/fnoc.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/fnoc.1.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/fnoc.2.bescmd >test.nc

#dnl t_string is derived from an hdf5 file

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/t_string.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/t_string.1.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/t_string.2.bescmd >test.nc

#dnl Tests that require handlers other than dapreader

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4_constraint.0.bescmd
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4_constraint.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4_constraint.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4_constraint.3.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/function_result_unwrap.bescmd >test.nc

#dnl Tests added for the fix for Hyrax-282. jhrg 11/3/16
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.5.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.6.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.7.bescmd >test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.8.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.9.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.10.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.11.bescmd >test.nc
#dnl Test added for the fix for Hyrax-764. sk 08/17/18
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridNwm.bescmd >test.nc

#dnl Test added for the fix for HK-23.
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/testFillValue.bescmd >test.nc

#dnl NC4 test
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.8.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.9.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/simpleT00.10.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/arrayT01.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/cedar.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/fits.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.12.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/namesT.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT00.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT01.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/structT02.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/t_string.3.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/hdf4_constraint.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/testFillValue_nc4.bescmd >test.nc

#dnl Add HDF5 CF response 
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/h5_numeric_types.0.bescmd 
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/h5_numeric_types.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/h5_numeric_types.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/grid_1_2d.h5.0.bescmd 
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/grid_1_2d.h5.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/grid_1_2d.h5.2.bescmd >test.nc

rm -rf test.nc
