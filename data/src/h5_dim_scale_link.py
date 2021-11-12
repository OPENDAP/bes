# Create an HDF5 file that includes hardlinks for dimension scales.
# This tests  the ticket HYRAX-588


import numpy as np
import h5py
#
# Create an HDF5 file.
#
f = h5py.File('dim_scale_link.h5','w')

# Create dimension and dimension scales(dim0,dim1)
# Create hardlinks to dimensions

# Create a group
grp = f.create_group("g0")

dim0s=np.arange(4,dtype=np.int32)
dim0 = grp.create_dataset("dim0",data=dim0s)
dim0.make_scale('dim0')

f["dim0_h"] = dim0

grp1 = f.create_group("g1")

dim1s=np.arange(2,dtype=np.int32)
dim1 = grp1.create_dataset("dim1",data=dim1s)
dim1.make_scale('dim1')

grp2 = grp1.create_group("g2")
data1 = np.arange(8,dtype=np.int32)
dset = grp2.create_dataset("dset",(4,2),data=data1)
dset.dims[0].attach_scale(dim0)
dset.dims[1].attach_scale(dim1)
 
grp3 = grp1.create_group("g3")
grp3["dim1_h"] = dim1

 
# Close the file before exiting
f.close()
