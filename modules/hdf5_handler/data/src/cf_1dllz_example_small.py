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
 
file = h5py.File ("t_cf_1dllzs.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((2,3, 6), 'f')

#  values between a[0][*] and a[1][*] is around 300.0
for z in range (0,2):
    for x in range (0, 1):
    	for y in range (0, 6):
        	temp_array[z][x][y] = 280.0 - 10*z 

# values between a[0][*] and a[59][*], a[121][*] and a[179][*]
#   is around 280.0
    for x in range (1, 2):
        for y in range (0, 6):
	        temp_array[z][x][y] = 300.0 -10*z

    for x in range (2, 3):
	    for y in range (0, 6):
	        temp_array[z][x][y] = 280.0 -10*z

temp_dset = file.create_dataset ('temp', data=temp_array,
	chunks=(2,3,6), compression='gzip')

temp_dset.attrs["long_name"] = "temperature"
temp_dset.attrs["units"] = "kelvin"

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
lat_array = numpy.ones (3, 'f') 
for x in range (0, 3):
    lat_array[x] = -90.0 + x*60

lat_dset = file.create_dataset ('lat', data = lat_array) 
lat_dset.attrs["long_name"] = "latitude"
lat_dset.attrs["units"] = "degrees_north"
lat_dset.attrs["standard_name"] = "latitude"


# ************  LONGITUDE  ***********
lon_array = numpy.ones (6, 'f') 
for x in range (0, 6):
    lon_array[x] = -180.0 + x*60

lon_dset = file.create_dataset ('lon', data = lon_array)
lon_dset.attrs["long_name"] = "longitude"
lon_dset.attrs["units"] = "degrees_east" 
lon_dset.attrs["standard_name"]= "longitude"


file.close



