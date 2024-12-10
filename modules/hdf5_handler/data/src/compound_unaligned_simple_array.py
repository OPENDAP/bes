import h5py
import numpy as np
#
#
file = h5py.File('compound_high_comp_unaligned_simple_array.h5','w')
#
# Create  dataset under the Root group.
#
comp_type = np.dtype([('Orbit', 'f8'), ('Misc','i2'),('Temperature', 'f4',(4))])
dataset = file.create_dataset("DSC_memb_array",(4,), chunks=(2,),compression="gzip", compression_opts=1, shuffle=True,dtype=comp_type)
data = np.array([(1153, 0, (1,1,1,1)), (1184,1, (2,2,2,2)),(1205, 2, (3,3,3,3)),(1216, 3, (4,4,4,4))], dtype = comp_type)

dataset[...] = data

#
# Close the file before exiting
#
file.close()

