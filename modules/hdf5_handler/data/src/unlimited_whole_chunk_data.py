import h5py
import numpy as np
#
#
file = h5py.File('whole_chunk_data_unlim.h5','w')
#
# Create  dataset under the Root group.
#
atomic_array = np.arange(5) +2 
dataset = file.create_dataset("p_cd",(5,),maxshape=(None,),chunks=(5,),data=atomic_array,dtype='i2')

#
# Close the file before exiting
#
file.close()

