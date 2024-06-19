#besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compound_simple_scalar_vlen_str.h5.dmrpp.bescmd >tt_scalar.nc
#besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compound_array_fix_vlen_str.h5.dmrpp.bescmd >tt_array.nc
besstandalone -c tests/bes.nc4.grp.conf -i tests/bescmd/compound_simple_scalar_memb_str_array.h5.dmrpp.bescmd >tt_struct_array.nc
