import h5py
import numpy as np

# Variable-length UTF-8 string dtype
str_dtype = h5py.string_dtype(encoding='utf-8')

with h5py.File("vl_str_dup_cross_array.h5", "w") as f:
    # Create a 2x4 variable-length string dataset
    dset = f.create_dataset(
        "str_dup",
        shape=(2, 4),
        dtype=str_dtype
    )

    # Fill the dataset
    dset[...] = np.array([
        ["apple", "banana", "cherry", "cherry"],
        ["cherry", "giraffe", "giraffe", "hippopotamus"]
    ], dtype=object)


