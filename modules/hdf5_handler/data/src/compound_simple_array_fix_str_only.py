import h5py
import numpy as np
#
#
file = h5py.File('compound_simple_array_fix_str_only.h5','w')
#
# Create  dataset under the Root group.
#
comp_type = np.dtype([('year', '<S4')])
dataset = file.create_dataset("DSC",(2,),dtype = comp_type)
data = np.array([('2023'),('2024')], dtype = comp_type)
dataset[...] = data

#
# Close the file before exiting
#
file.close()

