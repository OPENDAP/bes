#
# This examaple creates an HDF5 file dset.h5 and an empty datasets /dset in it.
#
import numpy
import h5py
#
# Create a new file using defaut properties.
#
file = h5py.File('t_scalar_dset_float_be.h5','w')
#

dset7 = file.create_dataset("t_float32",(), h5py.h5t.IEEE_F32BE)
dset7[...] = 1.1

dset8 = file.create_dataset("t_float64",(), h5py.h5t.IEEE_F64BE)
dset8[...] = -2.2


#
# Close the file before exiting
#
file.close()
