import h5py
import numpy as np
#
#
file = h5py.File('enum_grp.h5','w')
mapping = {'FALSE':0,'TRUE':1}
datatype = h5py.special_dtype(enum=(np.int8,mapping))
#
# Create  dataset under the Root group.
#
grp = file.create_group("g1")
dataset = grp.create_dataset("g1_enum",(),dtype = datatype)
dataset[...] = (0)
grp2 = grp.create_group("g2")
dataset = grp2.create_dataset("g2_enum",(),dtype = datatype)
dataset[...] = (1)

#
# Close the file before exiting
#
file.close()

