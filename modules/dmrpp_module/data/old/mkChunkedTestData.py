#
# Build Test Data Files
#
#
# Authors: H. Joe Lee & Nathan Potter
#
# Date:    12/28/2016
#
#

import h5py
import numpy as np


#########################################################################
# Make HDF5 test files where the dataset/variable is only chunked 

# creating file: chunked_oneD.h5
f = h5py.File('chunked_oneD.h5', 'w')
# Dataset: /d_4_chunks
dt = np.dtype('<f4')
arr = np.arange(40000, dtype=dt)
dset = f.create_dataset('d_4_chunks', (40000,), maxshape=(40000,), chunks=(10000,), dtype=dt, data=arr)
# initialize dataset values here
f.close()


# creating file: chunked_oneD_uneven.h5
f = h5py.File('chunked_oneD_uneven.h5', 'w')
# Dataset: /d_5_odd_chunks
dt = np.dtype('<f4')
arr = np.arange(40000, dtype=dt)
dset = f.create_dataset('d_5_odd_chunks', (40000,), maxshape=(40000,), chunks=(9501,), dtype=dt, data=arr)
# initialize dataset values here
f.close()


# creating file: chunked_twoD.h5
f = h5py.File('chunked_twoD.h5', 'w')
# Dataset: /d_4_chunks
dt = np.dtype('<f4')
arr100x100 = np.arange(10000, dtype=dt).reshape(100,100)
dset = f.create_dataset('d_4_chunks', (100,100), maxshape=(100,100), chunks=(50,50), dtype=dt, data=arr100x100)
# initialize dataset values here
f.close()

# creating file: chunked_twoD_asymmetric.h5
f = h5py.File('chunked_twoD_asymmetric.h5', 'w')
# Dataset: /d_8_chunks
dt = np.dtype('<f4')
arr100x200 = np.arange(20000, dtype=dt).reshape(100,200)
dset = f.create_dataset('d_8_chunks', (100,200), maxshape=(100,200), chunks=(50,50), dtype=dt, data=arr100x200)
# initialize dataset values here
f.close()

# creating file: chunked_twoD_uneven.h5
f = h5py.File('chunked_twoD_uneven.h5', 'w')
# Dataset: /d_10_odd_chunks
dt = np.dtype('<f4')
#arr100x100 = np.arange(10000, dtype=dt).reshape(100,100)
dset = f.create_dataset('d_10_odd_chunks', (100,100), maxshape=(100,100), chunks=(50,41), dtype=dt, data=arr100x100)
# initialize dataset values here
f.close()

# creating file: chunked_threeD.h5
f = h5py.File('chunked_threeD.h5', 'w')
# Dataset: /d_8_chunks
dt = np.dtype('<f4')
arr3 = np.arange(1000000, dtype=dt).reshape(100,100,100)
dset = f.create_dataset('d_8_chunks', (100,100,100), maxshape=(100,100,100), chunks=(50,50,50), dtype=dt, data=arr3)
# initialize dataset values here
f.close()

# creating file: chunked_threeD_asymmetric.h5
f = h5py.File('chunked_threeD_asymmetric.h5', 'w')
# Dataset: /d_8_chunks
dt = np.dtype('<f4')
arr3 = np.arange(1000000, dtype=dt).reshape(100,50,200)
dset = f.create_dataset('d_8_chunks', (100,50,200), maxshape=(100,50,200), chunks=(50,50,50), dtype=dt, data=arr3)
# initialize dataset values here
f.close()

# creating file: chunked_threeD_asymmetric_uneven.h5
f = h5py.File('chunked_threeD_asymmetric_uneven.h5', 'w')
# Dataset: /d_odd_chunks
dt = np.dtype('<f4')
arr3 = np.arange(1000000, dtype=dt).reshape(100,50,200)
dset = f.create_dataset('d_odd_chunks', (100,50,200), maxshape=(100,50,200), chunks=(41,50,53), dtype=dt, data=arr3)
# initialize dataset values here
f.close()


# creating file: chunked_fourD.h5
f = h5py.File('chunked_fourD.h5', 'w')
# Dataset: /d_16_chunks
dt = np.dtype('<f4')
arr4 = np.arange(2560000, dtype=dt).reshape(40,40,40,40)
dset = f.create_dataset('d_16_chunks', (40,40,40,40), maxshape=(40,40,40,40), chunks=(20,20,20,20), dtype=dt, data=arr4)
# initialize dataset values here
f.close()



#########################################################################
# Make HDF5 test files where the dataset/variable is both chunked 
# and compressed.


# creating file: chunked_gziped_oneD.h5
f = h5py.File('chunked_gziped_oneD.h5', 'w')
# Dataset: /d_4_gzipped_chunks
dt = np.dtype('<f4')
arr = np.arange(40000, dtype=dt)
dset = f.create_dataset('d_4_gzipped_chunks', (40000,), maxshape=(40000,), chunks=(10000,), compression='gzip', compression_opts=6, dtype=dt, data=arr)
# initialize dataset values here
f.close()


# creating file: chunked_gziped_twoD.h5
f = h5py.File('chunked_gzipped_twoD.h5', 'w')
# Dataset: /d_4_gzipped_chunks
dt = np.dtype('<f4')
arr2 = np.arange(10000, dtype=dt).reshape(100,100)
dset = f.create_dataset('d_4_gzipped_chunks', (100,100), maxshape=(100,100), chunks=(50,50), compression='gzip', compression_opts=6, dtype=dt, data=arr2)
# initialize dataset values here
f.close()

# creating file: chunked_gziped_threeD.h5
f = h5py.File('chunked_gzipped_threeD.h5', 'w')
# Dataset: /d_8_gzipped_chunks
dt = np.dtype('<f4')
arr3 = np.arange(1000000, dtype=dt).reshape(100,100,100)
dset = f.create_dataset('d_8_gzipped_chunks', (100,100,100), maxshape=(100,100,100), chunks=(50,50,50), compression='gzip', compression_opts=6, dtype=dt, data=arr3)
# initialize dataset values here
f.close()


# creating file: chunked_gziped_fourD.h5
f = h5py.File('chunked_gzipped_fourD.h5', 'w')
# Dataset: /d_16_gzipped_chunks
dt = np.dtype('<f4')
arr4 = np.arange(2560000, dtype=dt).reshape(40,40,40,40)
dset = f.create_dataset('d_16_gzipped_chunks', (40,40,40,40), maxshape=(40,40,40,40), chunks=(20,20,20,20), compression='gzip', compression_opts=6, dtype=dt, data=arr4)
# initialize dataset values here
f.close()

#########################################################################
# Make HDF5 test files where the dataset/variable is both chunked 
# and shuffled.


# creating file: chunked_shuffled_oneD.h5
f = h5py.File('chunked_shuffled_oneD.h5', 'w')
# Dataset: /d_4_shuffled_chunks
dt = np.dtype('<f4')
arr = np.arange(40000, dtype=dt)
dset = f.create_dataset('d_4_shuffled_chunks', (40000,), maxshape=(40000,), chunks=(10000,), shuffle=True, dtype=dt, data=arr)
# initialize dataset values here
f.close()


# creating file: chunked_gziped_twoD.h5
f = h5py.File('chunked_shuffled_twoD.h5', 'w')
# Dataset: /d_4_shuffled_chunks
dt = np.dtype('<f4')
arr2 = np.arange(10000, dtype=dt).reshape(100,100)
dset = f.create_dataset('d_4_shuffled_chunks', (100,100), maxshape=(100,100), chunks=(50,50), shuffle=True, dtype=dt, data=arr2)
# initialize dataset values here
f.close()

# creating file: chunked_shuffled_threeD.h5
f = h5py.File('chunked_shuffled_threeD.h5', 'w')
# Dataset: /d_8_shuffled_chunks
dt = np.dtype('<f4')
arr3 = np.arange(1000000, dtype=dt).reshape(100,100,100)
dset = f.create_dataset('d_8_shuffled_chunks', (100,100,100), maxshape=(100,100,100), shuffle=True, dtype=dt, data=arr3)
# initialize dataset values here
f.close()


# creating file: chunked_shuffled_fourD.h5
f = h5py.File('chunked_shuffled_fourD.h5', 'w')
# Dataset: /d_16_shuffled_chunks
dt = np.dtype('<f4')
arr4 = np.arange(2560000, dtype=dt).reshape(40,40,40,40)
dset = f.create_dataset('d_16_shuffled_chunks', (40,40,40,40), maxshape=(40,40,40,40), shuffle=True, dtype=dt, data=arr4)
# initialize dataset values here
f.close()



#########################################################################
# Make HDF5 test files where the dataset/variable is chunked, shuffled,
# and compressed.


# creating file: chunked_shufzip_oneD.h5
f = h5py.File('chunked_shufzip_oneD.h5', 'w')
# Dataset: /d_4_shufzip_chunks
dt = np.dtype('<f4')
arr = np.arange(40000, dtype=dt)
dset = f.create_dataset('d_4_shufzip_chunks', (40000,), maxshape=(40000,), chunks=(10000,), compression='gzip', compression_opts=6, shuffle=True, dtype=dt, data=arr)
# initialize dataset values here
f.close()


# creating file: chunked_gziped_twoD.h5
f = h5py.File('chunked_shufzip_twoD.h5', 'w')
# Dataset: /d_4_shufzip_chunks
dt = np.dtype('<f4')
arr2 = np.arange(10000, dtype=dt).reshape(100,100)
dset = f.create_dataset('d_4_shufzip_chunks', (100,100), maxshape=(100,100), chunks=(50,50), compression='gzip', compression_opts=6, shuffle=True, dtype=dt, data=arr2)
# initialize dataset values here
f.close()

# creating file: chunked_shufzip_threeD.h5
f = h5py.File('chunked_shufzip_threeD.h5', 'w')
# Dataset: /d_8_shufzip_chunks
dt = np.dtype('<f4')
arr3 = np.arange(1000000, dtype=dt).reshape(100,100,100)
dset = f.create_dataset('d_8_shufzip_chunks', (100,100,100), maxshape=(100,100,100), compression='gzip', compression_opts=6, shuffle=True, dtype=dt, data=arr3)
# initialize dataset values here
f.close()


# creating file: chunked_shufzip_fourD.h5
f = h5py.File('chunked_shufzip_fourD.h5', 'w')
# Dataset: /d_16_shufzip_chunks
dt = np.dtype('<f4')
arr4 = np.arange(2560000, dtype=dt).reshape(40,40,40,40)
dset = f.create_dataset('d_16_shufzip_chunks', (40,40,40,40), maxshape=(40,40,40,40), compression='gzip', compression_opts=6, shuffle=True, dtype=dt, data=arr4)
# initialize dataset values here
f.close()


