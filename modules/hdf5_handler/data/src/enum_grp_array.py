import h5py
import numpy as np
#
#
DIM1 =3
DIM0 =4

file = h5py.File('enum_grp_array.h5','w')
mapping = {'FALSE':0,'TRUE':1}
datatype = h5py.special_dtype(enum=(np.int8,mapping))

wdata = np.zeros((DIM0),dtype=np.int8)
for i in range(DIM0):
    wdata[i] = i%(mapping['TRUE']+1)
#
# Create  dataset under group g1.
#
grp = file.create_group("g1")
dataset = grp.create_dataset("g1_enum",(DIM0,),dtype = datatype)
dataset[...] = wdata

wdata = np.zeros((DIM1),dtype=np.int8)
for i in range(DIM1):
    wdata[i] = i%(mapping['TRUE']+1)

#Create another dataset under group g2
grp2 = grp.create_group("g2")
dataset = grp2.create_dataset("g2_enum",(DIM1,),dtype = datatype)

dataset[...] = wdata


#
# Close the file before exiting
#
file.close()

