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
f = h5py.File('dim_scale.h5','w')

# Create dimension and dimension scales(dim1,dim2,dim3,dim4)
dim1s=np.arange(4,dtype=np.int32)
dim2s=np.arange(2,dtype=np.int32)
dim3s=np.arange(2,dtype=np.int32)
dim4s=np.arange(2,dtype=np.int32)
dim1 = f.create_dataset("dim1",data=dim1s)
dim2 = f.create_dataset("dim2",data=dim2s)
dim3 = f.create_dataset("dim3",data=dim3s)
dim4 = f.create_dataset("dim4",data=dim4s)
dim1.dims.create_scale(dim1)
dim2.dims.create_scale(dim2)
dim3.dims.create_scale(dim3)
dim4.dims.create_scale(dim4)

# Create three groups
grp = f.create_group("data")
grp2 = f.create_group("GeoData")
grp3 = f.create_group("OtherData")
 
# Create dataset "temp" and the corresponding attributes.
# Note that the coordinates attribute includes a redundant variable /GeoData/proftime
data1 = np.arange(16,dtype=np.float32)+300
data2 = np.arange(16,dtype=np.float32)+280
dset1 = grp.create_dataset("temp",(4,2,2),data=data1)
dset1.dims[0].attach_scale(dim1)
dset1.dims[1].attach_scale(dim2)
dset1.dims[2].attach_scale(dim3)
dset1.attrs["coordinates"]="/GeoData/lat /GeoData/lon /GeoData/proftime /GeoData/v2"
dset1.attrs["units"]="K"

 
# Create dataset "temp2" and the corresponding attributes.
# Note that the coordinates attribute includes a coordinate variable in another group 
# /OtherData/v3.
dset2 = grp.create_dataset("temp2",(4,2,2),data=data2)
dset2.dims[0].attach_scale(dim1)
dset2.dims[1].attach_scale(dim2)
dset2.dims[2].attach_scale(dim4)
dset2.attrs["coordinates"]="/GeoData/lat /GeoData/lon /OtherData/v3"
dset2.attrs["units"]="K"

# Create 2-D lat/lon
latdata = np.arange(8,dtype=np.float32)+40
lat1=grp2.create_dataset("lat",(4,2),data=latdata)
lat1.dims[0].attach_scale(dim1)
lat1.dims[1].attach_scale(dim2)
lat1.attrs["units"]="degrees_north"
londata = np.arange(8,dtype=np.float32)-100
lon1=grp2.create_dataset("lon",(4,2),data=londata)
lon1.dims[0].attach_scale(dim1)
lon1.dims[1].attach_scale(dim2)
lon1.attrs["units"]="degrees_east"

# Create profile time
proftimedata1 = np.arange(10,14,dtype=np.float32)
proftimedset = grp2.create_dataset("proftime",(4,),data=proftimedata1)
proftimedset.dims[0].attach_scale(dim1)
proftimedset.attrs["units"]="seconds since 2010-01-01"

# Create 1-D v3 CV
v3data = np.arange(30,32,dtype=np.float32)
v3dset = grp3.create_dataset("v3",(2,),data=v3data)
v3dset.dims[0].attach_scale(dim4)
v3dset.attrs["units"]="km"

# Create 1-D v2 CV
v2data = np.arange(20,22,dtype=np.float32)
v2dset = grp2.create_dataset("v2",(2,),data=v2data)
v2dset.dims[0].attach_scale(dim3)
v2dset.attrs["units"]="hpa"
#
# Close the file before exiting
#
f.close()
