import h5py
import numpy as np
#
#
file = h5py.File('compound_simple_array_fix_str.h5','w')
#
# Create  dataset under the Root group.
#
comp_type = np.dtype([('Orbit', 'i'), ('Temperature', 'f4'), ('year', '<S4')])
dataset = file.create_dataset("DSC",(2,),dtype = comp_type)
data = np.array([(1153,53.23,'2023'),(1184,55.12,'2024')], dtype = comp_type)
dataset[...] = data

#
# Close the file before exiting
#
file.close()

