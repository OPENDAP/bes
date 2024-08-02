import h5py
import numpy as np
#
#
file = h5py.File('compound_array_fix_vlen_str.h5','w')
#
# Create  dataset under the Root group.
#
dt_vlen = h5py.string_dtype(encoding='utf-8')
comp_type = np.dtype([('Orbit', 'i'), ('year','<S4',(3)),('Temperature', 'f4',(2)), ('month', dt_vlen),('day',dt_vlen,(3))])
dataset = file.create_dataset("DSC_array",(2,),dtype = comp_type)
data = np.array([(1153,('1979','1989','1999'),(53.23,54.11),'12',('1','23','31')),(1184,('2001','2011','2021'),(55.12,56.07),'1',('25','9','14'))], dtype = comp_type)
dataset[...] = data

#
# Close the file before exiting
#
file.close()

