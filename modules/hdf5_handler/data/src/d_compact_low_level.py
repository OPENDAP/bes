import h5py
import numpy as np

data = np.arange(10)

with h5py.File("compact_lowlevel.h5", "w") as f:
    space = h5py.h5s.create_simple(data.shape)
    dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)
    dcpl.set_layout(h5py.h5d.COMPACT)

    dset_id = h5py.h5d.create(
        f.id,
        b"my_dataset",
        h5py.h5t.NATIVE_INT64,
        space,
        dcpl=dcpl
    )

    dset_id.write(h5py.h5s.ALL, h5py.h5s.ALL, data)

print("Low-level compact dataset created.")
