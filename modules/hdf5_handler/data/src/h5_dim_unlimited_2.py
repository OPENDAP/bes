# An example that contains a variable that has 3-D unlimited dimensions.
#

import numpy as np
import h5py
#
# Create an HDF5 file.
#
f = h5py.File('multi_unlimited_full.h5','w')

# Create dimension and dimension scales(time,dim2,dim3,dim4)
dim1s = np.arange(1,dtype=np.int32)
dim1  = f.create_dataset("d1",(1,),maxshape=(None,),data=dim1s)
dim1.dims.create_scale(dim1)

# Create group "g1"
grp = f.create_group("g1")
dim2s=np.arange(2,dtype=np.int32)
dim2 = grp.create_dataset("d2",(2,),maxshape=(None,),data=dim2s)
dim2.dims.create_scale(dim2)

#Create subgroups under /Product
grp2 = grp.create_group("g2")
dim3s=np.arange(3,dtype=np.int32)
dim3 = grp2.create_dataset("d3",(3,),maxshape=(None,),data=dim3s)
dim3.dims.create_scale(dim3)

#Create 3-D temp and attach scales
data2=np.arange(6,dtype=np.int32)
dset2 = grp2.create_dataset("var_no_dimnames",(1,2,3),maxshape=(None,None,None),chunks=(2,2,3),compression="gzip", compression_opts=1,data=data2)
data1 = np.arange(6,dtype=np.int32)
dset1 = grp2.create_dataset("var",(1,2,3),maxshape=(None,None,None),chunks=(2,2,3), compression="gzip", compression_opts=1, data=data1)
dset1.dims[0].attach_scale(dim1)
dset1.dims[1].attach_scale(dim2)
dset1.dims[2].attach_scale(dim3)
f.close()
