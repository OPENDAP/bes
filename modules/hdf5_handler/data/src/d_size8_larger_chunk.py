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
 
file = h5py.File ("d_size8_large_chunk.h5", 'w')

# initialize temperature array
temp_array = numpy.ones ((2, 3), 'i1')

#  values between a[60][*] and a[120][*] is around 300.0
for x in range (0, 1):
    for y in range (0, 2):
        temp_array[x][y] = -1; 

# The maximum dimension is set for the first dimension and the chunk can be bigger than the
# dimension size.
temp_dset = file.create_dataset ('temp', data=temp_array,
	                         dtype='i1', maxshape=(None,3), chunks=(4,3), compression='gzip')


file.close



