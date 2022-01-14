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

 # Programmer:  Isayah Reed <ireed@hdfgroup.org>
 #              Wednesday, May  4, 2011
 #
 # Purpose:
 #  Demonstrates how to use python to add CF attributes to an h5 file. Creates 
 #  a file with 3 datasets: lat, lon, temp. Lat contains the CF attributes: 
 #  units, long_name, and standard_name. Lon has the same CF attributes as the 
 #  latitude dataset. Temp contains the CF attributes: units, long_name, 
 #  _FillValue, coordinates, valid_min, valid_max, valid_range, scale_factor, 
 #  add_offset. Outputs data to cf_example.h5


import h5py 
import numpy 
 
file = h5py.File ("cf_2dlatlon_example.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((180, 360), 'f')

#  values between a[60][*] and a[120][*] is around 300.0
for x in range (0, 60):
    for y in range (0, 360):
        temp_array[x][y] = 280.0 

# values between a[0][*] and a[59][*], a[121][*] and a[179][*]
#   is around 280.0
    for x in range (60, 121):
        for y in range (0, 360):
	        temp_array[x][y] = 300.0 

    for x in range (121, 180):
	    for y in range (0, 360):
	        temp_array[x][y] = 280.0

temp_dset = file.create_dataset ('temp', data=temp_array,
	chunks=(180,360), compression='gzip')

temp_dset.attrs["long_name"] = "temperature"
temp_dset.attrs["units"] = "kelvin"

vlen = h5py.special_dtype (vlen = str)
temp_dset.attrs.create ('coordinates', data = ['lat lon'], 
            dtype=vlen) 

# must explicitly declare numerical data, or else the datatype is assumed
#   to be F64LE (we want F32)
temp_dset.attrs.create ('valid_min', data=0.0, dtype ='f')
temp_dset.attrs.create ('valid_max', data=400.0, dtype ='f')
vrange =[275, 305]
temp_dset.attrs.create ('valid_range', data=vrange, dtype='f') 
temp_dset.attrs.create ('_FillValue', data=-999.0, dtype ='f') 
temp_dset.attrs.create ('scale_factor', data=1.0, dtype='f') 
temp_dset.attrs.create ('add_offset', data = 10.0, dtype = 'f')


# ************  LATITUDE  ************
lat_array = numpy.ones ((180,360), 'f') 
for x in range (0, 180):
    for y in range (0,360):
        lat_array[x][y] = -90.0 + x *y/360;

lat_dset = file.create_dataset ('lat', data = lat_array,
           chunks=(180,360),compression='gzip') 
lat_dset.attrs["long_name"] = "latitude"
lat_dset.attrs["units"] = "degrees_north"
lat_dset.attrs["standard_name"] = "latitude"


# ************  LONGITUDE  ***********
lon_array = numpy.ones ((180,360), 'f') 
for x in range (0, 180):
    for y in range (0,360):
         lon_array[x][y] = -180.0 + y*x/180;

lon_dset = file.create_dataset ('lon', data = lon_array,
           chunks=(180,360),compression='gzip')
lon_dset.attrs["long_name"] = "longitude"
lon_dset.attrs["units"] = "degrees_east" 
lon_dset.attrs["standard_name"]= "longitude"


file.close



