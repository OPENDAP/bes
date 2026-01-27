import numpy as np
import h5py

FILE = "h5_one_bigger_chunk_size.h5"
DATASET = "DS1"

DIM0 = 2
DIM1 = 2
CHUNK0 = 2
CHUNK1 = 4

def run():

    # Initialize the data.
    wdata = np.zeros((DIM0, DIM1), dtype=np.uint8)
    for i in range(DIM0):
        for j in range(DIM1):
            wdata[i][j] = i+2*j+1 

    with h5py.File(FILE, 'w') as f:
        dset = f.create_dataset(DATASET, (DIM0, DIM1), maxshape=(CHUNK0,None), chunks=(CHUNK0, CHUNK1),
                                 dtype=np.uint8)
        dset[...] = wdata

if __name__ == "__main__":
    run()
