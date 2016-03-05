import h5py
import numpy

file = h5py.File ("t_invalid_fvalue.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((1, 2), 'i1')

for x in range (0, 0):
    for y in range (0, 1):
        temp_array[x][y] = 1;

temp_dset = file.create_dataset ('temp', data=temp_array,
                                 dtype=numpy.uint8)

# must explicitly declare numerical data, or else the datatype is assumed
#   to be F64LE (we want F32)
#temp_dset.attrs.create ('_FillValue', data=100, dtype ='i8')
temp_dset.attrs.create ('_FillValue', data=-127, dtype=numpy.int8)

# initialize temperature array
temp2_array = numpy.ones ((1, 2), 'i2')

for x in range (0, 0):
    for y in range (0, 1):
        temp2_array[x][y] = 1;

temp2_dset = file.create_dataset ('temp2', data=temp_array,
                                 dtype=numpy.uint16)

# must explicitly declare numerical data, or else the datatype is assumed
#   to be F64LE (we want F32)
#temp_dset.attrs.create ('_FillValue', data=100, dtype ='i8')
temp2_dset.attrs.create ('_FillValue', data=-32768, dtype=numpy.int16)


file.close

