#
# This examaple creates an HDF5 file dset.h5 and an empty datasets /dset in it.
#
import numpy
import h5py
#
# Create a new file using defaut properties.
#
file = h5py.File('t_scalar_dset.h5','w')
#
dset1 = file.create_dataset("t_int8",(), dtype='i1')
dset1[...] = -127

dset2 = file.create_dataset("t_uint8",(), dtype=numpy.uint8)
dset2[...] = 255

dset3 = file.create_dataset("t_int16",(), dtype='i2')
dset3[...] = -32767

dset4 = file.create_dataset("t_uint16",(), dtype=numpy.uint16)
dset4[...] = 65535

dset5 = file.create_dataset("t_int32",(), dtype='i4')
dset5[...] = -2147483647

dset6 = file.create_dataset("t_uint32",(), dtype=numpy.uint32)
dset6[...] = 4294967295

dset7 = file.create_dataset("t_float32",(), h5py.h5t.NATIVE_FLOAT)
dset7[...] = 1.1

dset8 = file.create_dataset("t_float64",(), h5py.h5t.NATIVE_DOUBLE)
dset8[...] = -2.2


#
# Close the file before exiting
#
file.close()
