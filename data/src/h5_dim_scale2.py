#
# This examaple creates an HDF5 file dset.h5 and an empty datasets /dset in it.
#
import numpy as np
import h5py
#
# Create a new file using defaut properties.
#
f = h5py.File('dim_scale2.h5','w')

grp = f.create_group("Product")
# create dimension
timedata1 = np.arange(1,dtype=np.int32)
dim1  = grp.create_dataset("time",(1,),data=timedata1)
dim1.dims.create_scale(dim1)
dim1.attrs["units"]="seconds since 2010-01-01 00:00:00"
dim2s=np.arange(4,dtype=np.int32)
dim3s=np.arange(2,dtype=np.int32)
dim4s=np.arange(10,12,dtype=np.int32)
dim2 = grp.create_dataset("dim2",data=dim2s)
dim3 = grp.create_dataset("dim3",data=dim3s)
dim4 = grp.create_dataset("dim4",data=dim4s)
dim2.dims.create_scale(dim2)
dim3.dims.create_scale(dim3)
dim4.dims.create_scale(dim4)


grp2 = grp.create_group("Support_data")
grp3 = grp.create_group("GeoData")
data1 = np.arange(8,dtype=np.float32)+300
data2 = np.arange(16,dtype=np.float32)+280
dset1 = grp.create_dataset("temp",(1,4,2),data=data1)
dset1.dims[0].attach_scale(dim1)
dset1.dims[1].attach_scale(dim2)
dset1.dims[2].attach_scale(dim3)
dset1.attrs["coordinates"]="/Product/lat /Product/lon"
dset1.attrs["units"]="K"
dset2 = grp2.create_dataset("temp2",(1,4,2,2),data=data2)
dset2.dims[0].attach_scale(dim1)
dset2.dims[1].attach_scale(dim2)
dset2.dims[2].attach_scale(dim3)
dset2.dims[3].attach_scale(dim4)
dset2.attrs["coordinates"]="/Product/lat /Product/lon"
dset2.attrs["units"]="K"
latdata = np.arange(8,dtype=np.float32)+40
lat1=grp.create_dataset("lat",(1,4,2),data=latdata)
lat1.dims[0].attach_scale(dim1)
lat1.dims[1].attach_scale(dim2)
lat1.dims[2].attach_scale(dim3)
lat1.attrs["units"]="degrees_north"
lat2data = np.arange(4,dtype=np.float32)-40
lat2=grp3.create_dataset("sat_lat",(1,4),data=lat2data)
lat2.dims[0].attach_scale(dim1)
lat2.dims[1].attach_scale(dim2)
lat2.attrs["units"]="degrees_north"
londata = np.arange(8,dtype=np.float32)-100
lon1=grp.create_dataset("lon",(1,4,2),data=londata)
lon1.dims[0].attach_scale(dim1)
lon1.dims[1].attach_scale(dim2)
lon1.dims[2].attach_scale(dim3)
lon1.attrs["units"]="degrees_east"
lon2data = np.arange(4,dtype=np.float32)+100
lon2=grp3.create_dataset("sat_lon",(1,4),data=lon2data)
lon2.dims[0].attach_scale(dim1)
lon2.dims[1].attach_scale(dim2)
lon2.attrs["units"]="degrees_east"
#
# Close the file before exiting
#
f.close()
