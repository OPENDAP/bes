import h5py
import numpy as np

# Variable-length UTF-8 string dtype
str_dtype = h5py.string_dtype(encoding='utf-8')

with h5py.File("vl_str_array.h5", "w") as f:
    # Create a 2x4 variable-length string dataset
    dset = f.create_dataset(
        "str",
        shape=(2, 4),
        dtype=str_dtype
    )

    # Fill the dataset
    dset[...] = np.array([
        ["apple", "banana", "cherry", "date"],
        ["elephant", "fox", "giraffe", "hippopotamus"]
    ], dtype=object)

    dset2 = f.create_dataset(
        "str_cmp",
        shape=(2, 4),
        chunks=(2,2),
        compression="gzip",
        dtype=str_dtype
    )

    # Fill the dataset
    dset2[...] = np.array([
        ["apple", "banana", "cherry", "date"],
        ["elephant", "fox", "giraffe", "hippopotamus"]
    ], dtype=object)

