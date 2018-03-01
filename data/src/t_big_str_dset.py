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
big_str="".join(choice(lowercase) for i in range(n))
#print glo_attr_str
file = h5py.File('t_big_str_dset.h5','w')
data=(big_str,big_str)
dt=h5py.special_dtype(vlen=str)

dset_a=file.create_dataset("arr_big_dset",(2,),dtype=dt)
dset_a[...]=data
#dset_s=file.create_dataset("sca_dset",(),dtype=dt)
#dset_s[...]=big_str

file.close()
