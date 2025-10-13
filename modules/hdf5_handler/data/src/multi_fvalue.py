
import h5py 
import numpy 
 
file = h5py.File ("multi_fvalue.h5", 'w')

temp_array = numpy.ones ((2,2), 'i')

for z in range (0,2):
    for x in range (0, 2):
        	temp_array[z][x] = 2-z

test_dset = file.create_dataset ('test', data = temp_array) 
mfvalue =[-999, -9999]
test_dset.attrs.create ('_FillValue', data=mfvalue, dtype ='i') 

file.close



