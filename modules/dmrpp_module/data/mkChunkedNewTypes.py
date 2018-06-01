import h5py
import numpy as np

PATH ='/Users/vskorole/opendap/hyrax/bes/modules/dmrpp_module/data/h5_test_data/'
#################################################################################
# Make HDF5 chunked string array
f = h5py.File(PATH + 'chunked_string_array.h5','w')
strr = np.array(['wqwqt','jhgjhgjh','kjhkjhk','ddsfdsg','njiuh' ])
dset = f.create_dataset("string_array", (5,), data = strr, chunks = True)
f.close()
#################################################################################
# Make HDF5 chunked scalar big string ???
f = h5py.File(PATH + 'chunked_scalar_string.h5','w')
LENGTH = 1000
alphabet = list('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789')
bstr = ''.join(np.random.choice(alphabet) for _ in range(LENGTH))
dt = h5py.special_dtype(vlen = unicode)
dset = f.create_dataset("scalar_big_string", dtype = dt, data = bstr)
f.close()
#################################################################################
# Make HDF5 chunked enum type
FILE = PATH + 'chunked_enum.h5'
DATASET = "DS1"
ATTRIBUTE = "A1"
DIM0 = 4
DIM1 = 7
# Create the scalar enum datatype.
mapping = {'SOLID': 0, 'LIQUID': 1, 'GAS': 2, 'PLASMA': 3}
dtype = h5py.special_dtype(enum=(np.int16, mapping))

# Initialize the data.
wdata = np.zeros((DIM0, DIM1), dtype=np.int32)
for i in range(DIM0):
    for j in range(DIM1):
        wdata[i][j] = ((i + 1) * j - j) % (mapping['PLASMA'] + 1)

with h5py.File(FILE, 'w') as f:
    dset = f.create_dataset(DATASET, data=0)
    dset.attrs.create(ATTRIBUTE, wdata, dtype=dtype)

with h5py.File(FILE) as f:
    rdata = f[DATASET].attrs[ATTRIBUTE]
f.close()
#################################################################################
# HDF5 file compound.h5 and an empty datasets /DSC in it.
file = h5py.File(PATH + 'chunked_compound.h5','w')
# Create a dataset under the Root group.
#
comp_type = np.dtype([('Orbit', 'i'),
                      ('Location', np.str_, 6),
                      ('Temperature (F)', 'f8'),
                      ('Pressure (inHg)', 'f8')])
dataset = file.create_dataset("DSC",(4,), comp_type, chunks = True)
data = np.array([(1153, "Sun   ", 53.23, 24.57),
                 (1184, "Moon  ", 55.12, 22.95),
                 (1027, "Venus ", 103.55, 31.23),
                 (1313, "Mars  ", 1252.89, 84.11)], dtype = comp_type)
dataset[...] = data
file.close()
#
# Read data
#
file = h5py.File(PATH + 'chunked_compound.h5', 'r')
dataset = file["DSC"]
print "Reading Orbit and Location fields..."
orbit = dataset['Orbit']
print "Orbit: ", orbit
location = dataset['Location']
print "Location: ", location
data = dataset[...]
print "Reading all records:"
print data
print "Second element of the third record:", dataset[2, 'Location']
file.close()
#################################################################################
# Use the Numpy void datatype to stand in for H5T_OPAQUE
FILE = PATH + "chunked_opaque.h5"
DATASET = "DSO"
ATTRIBUTE = "ATT"
DIM0 = 4
LEN = 7

# Initialize the data. Use the Numpy void datatype to stand in for
# H5T_OPAQUE
dtype=np.dtype('V7')
wdata = np.zeros((DIM0,), dtype=dtype)
for i in range(DIM0):
    wdata[i]= b'OPAQUE' + bytes([i + 48])

with h5py.File(FILE, 'w') as f:
    dset = f.create_dataset(DATASET, data=0)
    dset.attrs.create(ATTRIBUTE, wdata, dtype=dtype)

with h5py.File(FILE) as f:
    rdata = f[DATASET].attrs[ATTRIBUTE]
#
# Read data
#
print("%s:" % DATASET)
for row in rdata:
    print(row.tostring())

f.close()