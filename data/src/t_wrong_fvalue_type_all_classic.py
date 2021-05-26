import h5py
import numpy

# Generate an HDF5 file that only includes
# the following data type: f32, double, short, int8, int , uint8, unsigned short
# These datatype can be handled by DAP2 to fileout netCDF classic.
file = h5py.File ("t_wrong_fvalue_type_all_classic.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((2), 'i1')
temp_array2 = numpy.array([-1,-1])

#
dses_i8 = file.create_dataset ('i8', data=temp_array2,
                                 dtype=numpy.int8)
dses_i8.attrs.create ('_FillValue', data=128, dtype =numpy.uint8)

dses_ui8 = file.create_dataset ('ui8', data=temp_array,
                                 dtype=numpy.uint8)
dses_ui8.attrs.create ('_FillValue', data=255, dtype =numpy.uint16)

temp2_dset = file.create_dataset ('i16', data=temp_array2, dtype = numpy.int16)
temp2_dset.attrs.create ('_FillValue', data=-100, dtype =numpy.float64)

#
temp2_dset = file.create_dataset ('ui16', data=temp_array, dtype = numpy.uint16)
temp2_dset.attrs.create ('_FillValue', data=65535, dtype =numpy.int32)

#
temp2_dset = file.create_dataset ('i32', data=temp_array2,
                                 dtype=numpy.int32)
temp2_dset.attrs.create ('_FillValue', data=-65535, dtype =numpy.float64)

#
temp2_dset = file.create_dataset ('f32', data=temp_array,
                                 dtype=numpy.float32)
temp2_dset.attrs.create ('_FillValue', data=65535, dtype =numpy.int32)

#
temp2_dset = file.create_dataset ('f64', data=temp_array2,
                                 dtype=numpy.float64)
temp2_dset.attrs.create ('_FillValue', data=-1.3, dtype =numpy.float32)


dset1 = file.create_dataset("s_int8",(), dtype='i1')
dset1[...] = -127
dset1.attrs.create ('_FillValue', data=128, dtype =numpy.float32)

dset2 = file.create_dataset("s_uint8",(), dtype=numpy.uint8)
dset2[...] = 255
dset2.attrs.create ('_FillValue', data=0, dtype =numpy.float32)

dset3 = file.create_dataset("s_int16",(), dtype='i2')
dset3[...] = -1
dset3.attrs.create ('_FillValue', data=32768, dtype =numpy.float32)

dset4 = file.create_dataset("s_uint16",(), dtype=numpy.uint16)
dset4[...] = 1
dset4.attrs.create ('_FillValue', data=0, dtype =numpy.float32)

dset5 = file.create_dataset("s_int32",(), dtype='i4')
dset5[...] = -1
dset5.attrs.create ('_FillValue', data=2147483647, dtype =numpy.float32)

dset6 = file.create_dataset("s_f32",(), dtype=numpy.float32)
dset6[...] = -1
dset6.attrs.create ('_FillValue', data=-999, dtype =numpy.int32)

dset6 = file.create_dataset("s_f64",(), dtype=numpy.float64)
dset6[...] = -1
dset6.attrs.create ('_FillValue', data=9999, dtype =numpy.float32)




file.close

