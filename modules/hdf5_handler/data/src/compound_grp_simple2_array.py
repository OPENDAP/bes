import h5py
import numpy as np
#
#
file = h5py.File('compound_group_simple.h5','w')
#
# Create  dataset under the Root group.
#
comp_type = np.dtype([('Orbit', 'i'), ('Temperature', 'f4',(3))])
dataset = file.create_dataset("DSC_memb_array",(2,), chunks=(2,),dtype=comp_type)
data = np.array([(1153, (53.23,53.87,54.12)), (1184, (55.12,55.95,56.25))], dtype = comp_type)
dataset[...] = data

# Create a group
grp = file.create_group("g")

comp2_type = np.dtype([('Orbit', 'i'), ('Temperature', 'f4')])
dataset2 = grp.create_dataset("DSC",(2,), chunks=(2,),dtype=comp2_type)
data2 = np.array([(1213, 33.56), (1234, 34.78)], dtype = comp2_type)
dataset2[...] = data2

#
# Close the file before exiting
#
file.close()

