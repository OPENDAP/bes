import h5py
import numpy as np
#
#
file = h5py.File('compound_simple_bigger_chunk.h5','w')
#
# Create  dataset under the Root group.
#
comp_type = np.dtype([('Orbit', 'i'), ('Temperature', 'f4')])
dataset = file.create_dataset("DSC",(2,2), maxshape=(2,None),chunks=(2,4),dtype=comp_type)
#data = np.array([(1153, 53.23), (1184, 55.12), (1201,45.11),(1212,50.21)], dtype = comp_type)
data=np.zeros((2,2), dtype=comp_type)
data['Orbit'] = [[1153,1184],[1201,1212]]
data['Temperature'] = [[53.23,55.12],[45.11,50.21]]
dataset[...] = data

#
# Close the file before exiting
#
file.close()

