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
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/t_wrong_fvalue_type_all_classic.h5.bescmd >test.nc


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
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/simpleT00.8.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/simpleT00.9.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/simpleT00.10.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/arrayT.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/arrayT01.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/cedar.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/fits.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/gridT.12.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/namesT.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/structT00.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/structT01.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/structT02.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/t_string.3.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/hdf4.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/hdf4_constraint.4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/testFillValue_nc4.bescmd >test.nc

#dnl Add HDF5 CF response 
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/h5_numeric_types.0.bescmd 
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/h5_numeric_types.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/h5_numeric_types.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/grid_1_2d.h5.0.bescmd 
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/grid_1_2d.h5.1.bescmd | getdap -M -
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/grid_1_2d.h5.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/t_int64_dap4_d4.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/t_int64_dap4_d4_constraint.bescmd >test.nc

#dnl Add response that supports group(HDF5 default response et al.)
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/nc4_group_atomic.h5.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/t_cf_grp.h5.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/simple_nc4.nc.h5.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/t_group_scalar.h5.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/t_grp_string.h5.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/simple_nc4.nc_constraint.h5.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/simple_nc4.nc_constraint_2.h5.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/dim_scale_smix.h5_local_constraint.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/dim_scale_dim_name_clash.h5_local_constraint.2.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/nc4_group_var_dim_name_same.h5.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/t_wrong_fvalue_type_dap4_all_nocf.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk.dmrpp.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_s_c.dmrpp.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_s_c_c.dmrpp.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/eos5_grid.dmrpp.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/eos5_grid.h5.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/eos5_2_grids.h5.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/eos5_ps_grp.h5.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/eos5_sin_grp.h5.bescmd >test.nc


valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/grid_1_2d_dap2_ce_empty.h5.2.bescmd>gr.nc4
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/grid_1_2d_dap2_to_4_ce_empty.h5.2.bescmd>gr_d4.nc4
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/d_group_dap4_ce_empty.h5.2.bescmd>d_group.nc4
valgrind besstandalone -c tests/bes.nc4.conf -i tests/bescmd/t_wrong_fvalue_type_dap4_all.bescmd>d_group.nc4
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/t_wrong_fvalue_type_dap4_all_nocf.bescmd>d_group.nc4
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/nc4_group_mlls_cf.h5.bescmd>d_group.nc4
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/gridT.3.compression.bescmd>d_group.nc4

valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/SDS_fle_shuf_2def.h5.bescmd >test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/SDS_fle_shuf_2def.h5.dmrpp.bescmd >test.nc

valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/dim_scale_null_space.h5.bescmd>test.nc

valgrind besstandalone -c tests/bes.conf -i tests/bescmd/test_ba_start.dap.bescmd>test.nc
valgrind besstandalone -c tests/bes.conf -i tests/bescmd/test_ba_stride_stop.dap.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/test_ba_start_dim.dap.bescmd>test.nc

#valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/chunked_string_array_c.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/d_size64_chunk_be.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/d_size8.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compact_example.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/t_int_scalar.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/t_scalar_dset_float_be.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/FValue_c_b.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/netcdf_3_two_chunks_deflate.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_s_c_full_constraint.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_s_c_one_var.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_s_c_partial_constraint.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_2deflates.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_shuf_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_shuf_2deflates.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_fletcher_shuf_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/One_chunk_s_c_fvalue.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Two_chunks.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Two_chunks_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Two_chunks_2deflates.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Two_chunks_shuf_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Two_chunks_shuf_2deflates.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Two_chunks_fletcher_shuf_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Three_chunks_partial_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Three_chunks_partial_2deflates.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Three_chunks_partial_shuf_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Three_chunks_partial_shuf_2deflates.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/Three_chunks_partial_fletcher_shuf_deflate.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/all_storages_filters.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/all_storages_filters.h5.deflev.dmrpp.bescmd>test.nc

valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/all_storages_filters_constraint.h5.deflev.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/d_size8_large_chunk.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/nc4_group_atomic_comp_no_dio.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/swath_wrong_dim_rp.nc.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/nc4_group_atomic_comp.h5.dmrpp.bescmd>test.nc

#Structure support
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compound_simple_scalar.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compound_simple_scalar_memb_array.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compound_simple_array.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compound_simple2_array.h5.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compound_group_simple.h5.dmrpp.bescmd>test.nc

#HDF4 support
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/vg_hl_test.hdf.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/SDS_simple_comp.hdf.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/SDS_simple_float.hdf.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/SDS_simple_double.hdf.dmrpp.bescmd>test.nc
valgrind besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/SDS_chunk_extra_area_comp.hdf.dmrpp.bescmd>test.nc
rm -rf test.nc
rm -rf gr.nc4
rm -rf gr_d4.nc4
rm -rf d_group.nc4
