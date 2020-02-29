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
 

# initialize temperature array
temp_array = numpy.ones ((9, 18), 'f')

for x in range (0, 3):
    for y in range (0, 18):
        temp_array[x][y] = 280.0 

for x in range (3, 6):
    for y in range (0, 18):
	    temp_array[x][y] = 300.0 

for x in range (6, 9):
	for y in range (0, 18):
	    temp_array[x][y] = 280.0

# ************  LATITUDE  ************
lat_array = numpy.ones ((9,18), 'f') 
for x in range (0, 9):
    for y in range (0,18):
        lat_array[x][y] = -90.0 + x *20;

# ************  LONGITUDE  ***********
lon_array = numpy.ones ((9,18), 'f') 
for x in range (0, 9):
    for y in range (0,18):
         lon_array[x][y] = -180.0 + y*20;


file = h5py.File ("t_cf_geo_grp_2dlatlon.h5", 'w')
grp = file.create_group("Data")
temp_dset = grp.create_dataset ('temp', data=temp_array)
grp2 = file.create_group("Geolocation")
temp_dset.attrs["units"] = "kelvin"
lat_dset = grp2.create_dataset ('lat', data = lat_array)
lat_dset.attrs["units"] = "degrees_north"
lon_dset = grp2.create_dataset ('lon', data = lon_array)
lon_dset.attrs["units"] = "degrees_east" 

file.close()


file = h5py.File ("t_cf_geo2_grp_2dlatlon.h5", 'w')
grp = file.create_group("Data")
temp_dset = grp.create_dataset ('temp', data=temp_array)
grp2 = file.create_group("GeolocationData")
temp_dset.attrs["units"] = "kelvin"
lat_dset = grp2.create_dataset ('lat', data = lat_array)
lat_dset.attrs["units"] = "degrees"
lon_dset = grp2.create_dataset ('lon', data = lon_array)
lon_dset.attrs["units"] = "degrees" 

file.close()

file = h5py.File ("t_cf_non_geo_grp_2dlatlon.h5", 'w')
grp = file.create_group("Data")
temp_dset = grp.create_dataset ('temp', data=temp_array)
grp2 = file.create_group("FakeGeo")
temp_dset.attrs["units"] = "kelvin"
lat_dset = grp2.create_dataset ('lat', data = lat_array)
lat_dset.attrs["units"] = "degrees"
lon_dset = grp2.create_dataset ('lon', data = lon_array)
lon_dset.attrs["units"] = "degrees" 

file.close()



