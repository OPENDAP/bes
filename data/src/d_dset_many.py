# Copyright (C) 2014 The HDF Group
#
# This examaple creates an HDF5 file d_dset_many.h5 and puts 2000 datasets.
#
import h5py
import numpy as np
#
# Create a new file using defaut properties.
#
file = h5py.File('d_dset_many.h5','w')
#
# Create a dataset under the Root group.
#
data = np.zeros((2, 2))
for i in range(2000):
    dataset = file.create_dataset("dset%d" % i,(2, 2), h5py.h5t.STD_I32BE)
    dataset[...] = data

#
# Close the file before exiting
#
file.close()

