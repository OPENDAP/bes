import h5py
import numpy as np

data = np.array([
    [b'abc', b'def', b'ghi', b'jkl', b'mno'],
    [b'pqr', b'stu', b'vwx', b'yz1', b'234'],
    [b'567', b'890', b'ABC', b'DEF', b'GHI'],
    [b'JKL', b'MNO', b'PQR', b'STU', b'VWX'],
    [b'YZ!', b'@#$', b'123', b'456', b'789']
], dtype='S3')

with h5py.File('chunked_fstr.h5', 'w') as f:
    dset = f.create_dataset(
        'strings',
        shape=(5, 5),
        dtype='S3',
        chunks=(2, 2)
    )

    dset[:] = data

with h5py.File('chunked_one_fstr.h5', 'w') as f:
    dset = f.create_dataset(
        'strings',
        shape=(5, 5),
        dtype='S3',
        chunks=(5, 5)
    )

    dset[:] = data

with h5py.File('chunked_one_comp_fstr.h5', 'w') as f:
    dset = f.create_dataset(
        'strings',
        shape=(5, 5),
        dtype='S3',
        chunks=(5, 5),
        compression="gzip"
    )

    dset[:] = data


with h5py.File('chunked_comp_fstr.h5', 'w') as f:
    dset = f.create_dataset(
        'strings',
        shape=(5, 5),
        dtype='S3',
        chunks=(2, 2),
        compression="gzip"
    )

    dset[:] = data

with h5py.File('cont_fstr.h5', 'w') as f:
    dset = f.create_dataset(
        'strings',
        shape=(5, 5),
        dtype='S3'
    )

    dset[:] = data

