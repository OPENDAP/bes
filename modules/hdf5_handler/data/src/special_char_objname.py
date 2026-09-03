import h5py
import numpy as np

with h5py.File("t_special_name.h5", "w") as f:
    group = f.create_group("data field")

    my_data = np.array([1, 2], dtype=np.int16)
    dataset = group.create_dataset("data.1", data=my_data)
    dataset.attrs.create('_FillValue',data = -999,dtype = np.int16)
