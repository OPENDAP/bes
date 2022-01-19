#
# This script creates an HDF5 file dim_scale.h5 that emulates new OMI level 2 product
# file name: OMI-Aura_L2-OMIAuraAER_2006m0815t130241-o11086_v01-00-2018m0529t115547.h5
# 2-D lat/lon variables under group GeoData,
# 1-D profile time and 1-D vertical coordinate also under group GeoData.
# Another 1-D vertical coordinate under group OtherData.
# physical variables temp and temp2 under group data. 
# float temp(dim1, dim2, dim3) ;
#     string temp:coordinates = "/GeoData/lat /GeoData/lon /GeoData/proftime /GeoData/v2" ;
# float temp2(dim1, dim2, dim4) ;
#     string temp2:coordinates = "/GeoData/lat /GeoData/lon /OtherData/v3" ;
# Besides, four dimension and dimension scales (dim1,dim2,dim3,dim4) are created.


import numpy as np
import h5py
#
# Create an HDF5 file.
#
f = h5py.File('dim_scale_smix.h5','w')

# Create dimension and dimension scales(dim1,dim2,dim3,dim4)
dim1s=np.arange(4,dtype=np.int32)
dim1 = f.create_dataset("mydim1",data=dim1s)
dim1.dims.create_scale(dim1)

data1 = np.arange(4,dtype=np.int32)
data2 = np.arange(8,dtype=np.int32)
dset1 = f.create_dataset("v",data=data1)
dset2 = f.create_dataset("v_mdim",(4,2),data=data2)
dset1.dims[0].attach_scale(dim1)
 
# Create a group
grp = f.create_group("g")
#dim2s=np.arange(2,dtype=np.int32)
#dim2 = grp.create_dataset("dim2",data=dim2s)
#dim2.dims.create_scale(dim2)
 
# Create dataset "temp2" and the corresponding attributes.
data3 = np.arange(2,dtype=np.int32)
data4 = np.arange(6,dtype=np.int32)

dset3 = grp.create_dataset("vg",data=data3)
dset4 = grp.create_dataset("vg_mdim",(3,2),data=data4)
dim2s=np.arange(2,dtype=np.int32)
dim2 = grp.create_dataset("mydim2",data=dim2s)
dim2.dims.create_scale(dim2)
 
dset3.dims[0].attach_scale(dim2)

# Close the file before exiting
f.close()
