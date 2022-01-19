# Copyright (C) 2014 The HDF Group
#
# This examaple creates an HDF5 file d_dset_big.h5 and a 8G dataset /dset in it.
#
# It will take 2 hours to create this file.
#
import h5py
import numpy as np
#
# Create a new file using defaut properties.
#
file = h5py.File('d_dset_big.h5','w')
#
# Create a dataset under the Root group.
#
dataset = file.create_dataset("dset",(2, 1024, 1024, 1024), h5py.h5t.STD_I32BE)
#
# Initialize data object with 0.
#
data = np.zeros((2, 1024, 1024, 1024))
#
# Assign new values
#

for i in range(2):
    for j in range(1024):
       for k in range(1024):
          for l in range(1024):
               data[i][j][k][l]= -1
#
# Write data
#
dataset[...] = data

#
# Close the file before exiting
#
file.close()

