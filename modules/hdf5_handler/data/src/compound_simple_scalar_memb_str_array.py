import h5py
import numpy as np
#
#
file = h5py.File('compound_simple_scalar_memb_str_array.h5','w')
#
# Create  dataset under the Root group.
#

dt_vlen = h5py.string_dtype(encoding='utf-8')
comp_type = np.dtype([('Orbit', 'i'), ('desc',dt_vlen,(3)),('Temperature', 'f4',(2))])
dataset = file.create_dataset("DSC_scalar_base_array",(),dtype = comp_type)
dataset[...] = (1153,('orbit;','and;','temperature;'),(53.23,55.12))

#
# Close the file before exiting
#
file.close()

