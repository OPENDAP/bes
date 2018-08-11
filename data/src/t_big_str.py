# Copyright (C) 2018 The HDF Group
#
# This examaple creates several HDF5 long string(>netCDF java limit 32767) datasets and attributes.
#
#
import h5py
import numpy as np
from random import choice
from string import lowercase
#
# Create a new file using defaut properties.
#
n = 32768
sca_big_str="".join(choice(lowercase) for i in range(n))

#print glo_attr_str
#This attribute is greater than netCDF java limit.
file = h5py.File('t_big_str.h5','w')
file.attrs["glo_sca_attr"]=sca_big_str

n = 16384
#Note although the string size is bigger, but it doesn't exceed 
# netCDF java limitation.
glo_attr_str_ar1 = "".join(choice(lowercase) for i in range(n))
glo_attr_str_ar2 = glo_attr_str_ar1
data=(glo_attr_str_ar1,glo_attr_str_ar2)
dt=h5py.special_dtype(vlen=str)

#Attribute array
file.attrs.create("glo_arr_attr",data,(2,),dtype=dt)
dataset=file.create_dataset("dset",(2,),h5py.h5t.STD_I16LE)

#The atrribute size exceeds the netCDF Java limitation
dataset.attrs["local_sca_attr"]=sca_big_str
dset_a=file.create_dataset("arr_dset",(2,),dtype=dt)
dset_a[...]=data
#
# Leave the following for debugging
'''
for i in range(2):
    for j in range(1024):
       for k in range(1024):
          for l in range(1024):
               data[i][j][k][l]= -1
#
# Write data
#
print "Writing data..."
dataset[...] = data

print "Dataset dataspace is", dataset.shape
print "Dataset Numpy datatype is", dataset.dtype
print "Dataset name is", dataset.name
print "Dataset is a member of the group", dataset.parent
print "Dataset was created in the file", dataset.file
#
# Close the file before exiting
#
'''
file.close()
