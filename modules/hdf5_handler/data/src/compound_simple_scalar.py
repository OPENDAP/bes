import h5py
import numpy as np
#
#
file = h5py.File('compound_simple_scalar.h5','w')
#
# Create  dataset under the Root group.
#
comp_type = np.dtype([('Orbit', 'i'), ('Temperature', 'f4')])
dataset = file.create_dataset("DSC_scalar",(),dtype = comp_type)
dataset[...] = (1153,53.23)

#
# Close the file before exiting
#
file.close()

