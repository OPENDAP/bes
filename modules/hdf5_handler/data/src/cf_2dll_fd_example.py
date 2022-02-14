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
 
file = h5py.File ("cf_2dll_same_dimsize.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((4, 4), 'f')
temp_array2 = numpy.ones ((4, 4, 5), 'f')
temp_array3 = numpy.ones ((4, 4,4), 'f')
temp_array4 = numpy.ones ((5, 5,4,5), 'f')

temp_dset = file.create_dataset ('temp', data=temp_array)

temp_dset.attrs["long_name"] = "temperature"
temp_dset.attrs["units"] = "degC"

vlen = h5py.special_dtype (vlen = str)
temp_dset.attrs.create ('coordinates', data = ['lat lon'], 
            dtype=vlen) 

temp_dset2 = file.create_dataset ('temp2', data=temp_array2)

temp_dset2.attrs["long_name"] = "temperature"
temp_dset2.attrs["units"] = "degC"

vlen = h5py.special_dtype (vlen = str)
temp_dset2.attrs.create ('coordinates', data = ['lat lon'], 
            dtype=vlen) 

temp_dset3 = file.create_dataset ('temp3', data=temp_array3)

temp_dset3.attrs["long_name"] = "temperature"
temp_dset3.attrs["units"] = "degC"

vlen = h5py.special_dtype (vlen = str)
temp_dset3.attrs.create ('coordinates', data = ['lat lon'], 
            dtype=vlen) 

temp_dset4 = file.create_dataset ('temp4', data=temp_array4)

temp_dset4.attrs["long_name"] = "temperature"
temp_dset4.attrs["units"] = "degC"

vlen = h5py.special_dtype (vlen = str)
temp_dset4.attrs.create ('coordinates', data = ['lat lon'], 
            dtype=vlen) 



# ************  LATITUDE  ************
lat_array = numpy.ones ((4,4), 'f') 
for x in range (0, 4):
    for y in range (0,4):
        lat_array[x][y] = -90.0 + x *y/4;

lat_dset = file.create_dataset ('lat', data = lat_array)
lat_dset.attrs["long_name"] = "latitude"
lat_dset.attrs["units"] = "degrees_north"


# ************  LONGITUDE  ***********
lon_array = numpy.ones ((4,4), 'f') 
for x in range (0, 4):
    for y in range (0,4):
         lon_array[x][y] = -4.0 + y*x/4;

lon_dset = file.create_dataset ('lon', data = lon_array)
lon_dset.attrs["long_name"] = "longitude"
lon_dset.attrs["units"] = "degrees_east" 

file.close



