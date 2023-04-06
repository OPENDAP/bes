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
 
file = h5py.File ("d_size64_be.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((2, 3), 'i8')

#  values between a[60][*] and a[120][*] is around 300.0
for x in range (0, 2):
    for y in range (0, 3):
        temp_array[x][y] = 280+x+2*(y+1) 

temp_dset = file.create_dataset ('i64_be', data=temp_array,
	                         dtype=h5py.h5t.STD_I64BE)

file.close



