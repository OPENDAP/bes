# An example that contains a variable that has 3-D unlimited dimensions.
#

import numpy as np
import h5py
#
# Create an HDF5 file.
#
f = h5py.File('multi_unlimited_no_dim_names.h5','w')

# Create group "g1"
grp = f.create_group("g1")
#Create subgroups under /Product
grp2 = grp.create_group("g2")
#Create 3-D temp and attach scales
data1=np.arange(6,dtype=np.int32)
dset = grp2.create_dataset("var_no_dimnames",(1,2,3),maxshape=(None,None,None),chunks=(2,2,3), compression="gzip", compression_opts=1, data=data1)
f.close()
