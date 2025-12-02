import numpy as np
import h5py

FILE = "h5_odd_chunk_size_comp.h5"
DATASET = "DS1"

DIM0 = 11
DIM1 = 13
CHUNK0 = 3
CHUNK1 = 5

def run():

    # Initialize the data.
    wdata = np.zeros((DIM0, DIM1), dtype=np.int32)
    for i in range(DIM0):
        for j in range(DIM1):
            wdata[i][j] = i * j - j

    with h5py.File(FILE, 'w') as f:
        dset = f.create_dataset(DATASET, (DIM0, DIM1), chunks=(CHUNK0, CHUNK1),
                                 compression='gzip', dtype='<i4')
        dset[...] = wdata

if __name__ == "__main__":
    run()
