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
 
file = h5py.File ("d_size8.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((2, 3), 'i1')

#  values between a[60][*] and a[120][*] is around 300.0
for x in range (0, 1):
    for y in range (0, 2):
        temp_array[x][y] = -1; 

temp_dset = file.create_dataset ('temp', data=temp_array,
	                         dtype='i1')

# must explicitly declare numerical data, or else the datatype is assumed
#   to be F64LE (we want F32)
#temp_dset.attrs.create ('_FillValue', data=100, dtype ='i8')
temp_dset.attrs.create ('_FillValue', data=-127, dtype=numpy.int8)

file.close



