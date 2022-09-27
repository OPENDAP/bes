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
import numpy as np
 
# Simple latitude,longitude and their corresponding "bounds" attributes.   
# ************  LATITUDE  ************
lat_array = np.arange (0,2,dtype=np.float32) 
lat_bounds_1 = np.arange(-0.5,1.5,dtype=np.float32) 
lat_bounds_2 = np.arange(0.5,2.5,dtype=np.float32)
#np.concatenate requires the arrays to be concatenated must be supplied as tuples.
lat_bounds = np.concatenate((lat_bounds_1,lat_bounds_2))

print (lat_bounds)
print (lat_array)
# ************  LONGITUDE  ***********
lon_array = np.arange (1,3,dtype=np.float32)
lon_bounds_1 = np.arange(0.5,2.5,dtype=np.float32) 
lon_bounds_2 = np.arange(1.5,3.5,dtype=np.float32)
#np.concatenate requires the arrays to be concatenated must be supplied as tuples.
lon_bounds = np.concatenate((lon_bounds_1,lon_bounds_2))

print (lon_bounds)
print (lon_array)

# ************  PRESSURE  ***********
pre_array = np.arange (999,1001,dtype=np.float32) 
pre_bounds_1 = np.arange(998.5,1000.5,dtype=np.float32) 
pre_bounds_2 = np.arange(999.5,1001.5,dtype=np.float32)
#np.concatenate requires the arrays to be concatenated must be supplied as tuples.
pre_bounds = np.concatenate((pre_bounds_1,pre_bounds_2))


file = h5py.File ("t_cf_latlon_bounds.h5", 'w')
grp = file.create_group("Geo")
lat = grp.create_dataset ('lat', data=lat_array)
lat.attrs["bounds"] = "/Bounds/lat_bounds"
lon = grp.create_dataset ('lon', data=lon_array)
lon.attrs["bounds"] = "lon_bounds"

grp2 = file.create_group("Bounds")
lat_bnd = grp2.create_dataset ('lat_bounds',data = lat_bounds)
#longitude's bound is under the same group of longitude.
lon_bnd = grp.create_dataset ('lon_bounds',data = lon_bounds)

#Height, use relative path.
grp3 = grp.create_group("Vertical")
pre = grp3.create_dataset('pre',data=pre_array)
pre.attrs["bounds"] = "../../pre_bounds"
pre_bnd = file.create_dataset("pre_bounds",data = pre_bounds)

file.close()




