 # * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 # Copyright by The HDF Group.                                               *
 # Copyright by the Board of Trustees of the University of Illinois.         *
 # All rights reserved.                                                      *
 #                                                                           *
 # This file is part of HDF5.  The full HDF5 copyright notice, including     *
 # terms governing use, modification, and redistribution, is contained in    *
 # the files COPYING and Copyright.html.  COPYING can be found at the root   *
 # of the source code distribution tree; Copyright.html can be found at      *
 # http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 # access to either file, you may request a copy from help@hdfgroup.org.     *
 # * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

import h5py 
import numpy 
 
file = h5py.File ("cf_1dll_same_dimsize.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((3, 3), 'f')

temp_array2 = numpy.ones ((3, 3, 3), 'f')

for x in range (0, 3):
    for y in range (0, 3):
        temp_array[x][y] = 280.0 

for x in range (0, 3):
    for y in range (0, 3):
        for z in range (0, 3):
            temp_array2[x][y][z] = 300.0 


temp_dset = file.create_dataset ('temp', data=temp_array)

temp_dset.attrs["long_name"] = "temperature"
temp_dset.attrs["units"] = "kelvin"

vlen = h5py.special_dtype (vlen = str)
temp_dset.attrs.create ('coordinates', data = ['lat', 'lon'], 
            dtype=vlen) 

# must explicitly declare numerical data, or else the datatype is assumed
#   to be F63LE (we want F32)
temp_dset.attrs.create ('valid_min', data=0.0, dtype ='f')
temp_dset.attrs.create ('valid_max', data=300.0, dtype ='f')
vrange =[275, 305]
temp_dset.attrs.create ('valid_range', data=vrange, dtype='f') 
temp_dset.attrs.create ('_FillValue', data=-999.0, dtype ='f') 
temp_dset.attrs.create ('scale_factor', data=1.0, dtype='f') 
temp_dset.attrs.create ('add_offset', data = 10.0, dtype = 'f')

temp_dset2 = file.create_dataset ('temp2', data=temp_array2)

temp_dset2.attrs["long_name"] = "temperature"
temp_dset2.attrs["units"] = "kelvin"

vlen = h5py.special_dtype (vlen = str)
temp_dset2.attrs.create ('coordinates', data = ['lat', 'lon'], 
            dtype=vlen) 

# must explicitly declare numerical data, or else the datatype is assumed
#   to be F63LE (we want F32)
temp_dset2.attrs.create ('valid_min', data=0.0, dtype ='f')
temp_dset2.attrs.create ('valid_max', data=300.0, dtype ='f')
vrange =[275, 305]
temp_dset2.attrs.create ('valid_range', data=vrange, dtype='f') 
temp_dset2.attrs.create ('_FillValue', data=-999.0, dtype ='f') 
temp_dset2.attrs.create ('scale_factor', data=1.0, dtype='f') 
temp_dset2.attrs.create ('add_offset', data = 10.0, dtype = 'f')


# ************  LATITUDE  ************
lat_array = numpy.ones (3, 'f') 
for x in range (0, 3):
    lat_array[x] = -90.0 + x

lat_dset = file.create_dataset ('lat', data = lat_array) 
lat_dset.attrs["long_name"] = "latitude"
lat_dset.attrs["units"] = "degrees_north"
lat_dset.attrs["standard_name"] = "latitude"


# ************  LONGITUDE  ***********
lon_array = numpy.ones (3, 'f') 
for x in range (0, 3):
    lon_array[x] = -3.0 + x

lon_dset = file.create_dataset ('lon', data = lon_array)
lon_dset.attrs["long_name"] = "longitude"
lon_dset.attrs["units"] = "degrees_east" 
lon_dset.attrs["standard_name"]= "longitude"


file.close



