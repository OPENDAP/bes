import h5py
import numpy

file = h5py.File ("t_wrong_fvalue_type.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((2), 'i4')

#The variable datatype is unsigned int
temp_dset = file.create_dataset ('temp', data=temp_array,
                                 dtype=numpy.uint32)

# The fillvalue datatype is unsigned short 
temp_dset.attrs.create ('_FillValue', data=100, dtype =numpy.uint16)

# The variable datatype is float
temp2_dset = file.create_dataset ('temp2', data=temp_array,
                                 dtype=numpy.float32)

# The fillvalue datatype is double
temp2_dset.attrs.create ('_FillValue', data=100, dtype =numpy.float64)


file.close

