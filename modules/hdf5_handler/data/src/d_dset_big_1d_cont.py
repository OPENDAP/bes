import h5py
import numpy as np
#
# Create a new file using defaut properties.
#
file = h5py.File('d_dset_big_1d_cont.h5','w')
#
# Create a dataset under the Root group.
#
num_elems=5*1024*1024*1024
#
# Initialize data object with 0.
#
datav = np.empty(num_elems,dtype=np.uint8)
#
# Assign new values
#
for i in range(num_elems):
    datav[i] = i%5

dataset = file.create_dataset("dset",data=datav)
#
# Close the file before exiting
#
file.close()

