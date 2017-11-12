import h5py
import numpy as np
#
#
file = h5py.File('FakeDim_remove.h5','w')
#
# Create  dataset under the Root group.
#
comp_type = np.dtype([('Orbit', 'i'), ('Temperature', 'f8')])
dataset = file.create_dataset("DSC",(2,), maxshape=(None,),chunks=(2,),dtype=comp_type)
data = np.array([(1153, 53.23), (1184, 55.12)], dtype = comp_type)
dataset[...] = data
comp_type1 = np.dtype([('Pressure', 'f8'), ('index', 'i')])
dataset = file.create_dataset("dummy",(3,), maxshape=(None,),chunks=(3,),dtype=comp_type1)
data = np.array([(999.0, 1), (997.0, 2),(996.0,3)], dtype = comp_type1)
dataset[...] = data
atomic_array = np.arange(4)
dataset = file.create_dataset("atomic",(4,),maxshape=(None,),chunks=(4,),data=atomic_array,dtype='i2')
dataset_dim = file.create_dataset("dima",(4,),maxshape=(None,),chunks=(4,),data=atomic_array,dtype='i2')
file["dima"].dims.create_scale(file["dima"])
file["atomic"].dims[0].attach_scale(file["dima"])

#
# Close the file before exiting
#
file.close()

