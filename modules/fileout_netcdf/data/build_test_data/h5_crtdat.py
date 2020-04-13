#
# This examaple creates an HDF5 file dset.h5 and an empty datasets /dset in it.
#
import numpy
import h5py
#
# Create a new file using defaut properties.
#
file = h5py.File('tdset_types.h5','w')
#
dset1 = file.create_dataset("s_int8",(), dtype='i1')
dset1[...] = -127
dset1.attrs.create('attr_s_int8',data=-127,dtype='i1')

dset2 = file.create_dataset("s_uint8",(), dtype=numpy.uint8)
dset2[...] = 255
dset2.attrs.create('attr_s_uint8',data=255,dtype=numpy.uint8)

dset3 = file.create_dataset("s_int16",(), dtype='i2')
dset3[...] = -32767
dset3.attrs.create('attr_s_int16',data=-32767,dtype=numpy.int16)

dset4 = file.create_dataset("s_uint16",(), dtype=numpy.uint16)
dset4[...] = 65535
dset4.attrs.create('attr_s_uint16',data=65535,dtype=numpy.uint16)

dset5 = file.create_dataset("s_int32",(), dtype='i4')
dset5[...] = -2147483647
dset5.attrs.create('attr_s_int32',data=-2147483647,dtype=numpy.int32)

dset6 = file.create_dataset("s_uint32",(), dtype=numpy.uint32)
dset6[...] = 4294967295
dset6.attrs.create('attr_s_uint32',data=4294967295,dtype=numpy.uint32)

dset7 = file.create_dataset("s_float32",(), h5py.h5t.NATIVE_FLOAT)
dset7[...] = 1.1
dset7.attrs.create('attr_s_float32',data=1.1,dtype=numpy.float32)

dset8 = file.create_dataset("s_float64",(), h5py.h5t.NATIVE_DOUBLE)
dset8[...] = -2.2
dset8.attrs.create('attr_s_float64',data=-2.2,dtype=numpy.double)

adset1_array = numpy.empty(2)
adset1_array.fill(-127)
adset1 = file.create_dataset("a_int8",data=adset1_array, dtype='i1')
adset1.attrs.create('attr_a_int8',data=adset1_array,dtype='i1')

adset2_array = numpy.empty(2)
adset2_array.fill(255)
adset2 = file.create_dataset("a_uint8",data=adset2_array, dtype=numpy.uint8)
adset2.attrs.create('attr_a_uint8',data=adset2_array,dtype=numpy.uint8)

adset3_array = numpy.empty(2)
adset3_array.fill(-32767)
adset3 = file.create_dataset("a_int16",data=adset3_array, dtype='i2')
adset3.attrs.create('attr_a_int16',data=adset3_array,dtype=numpy.int16)

adset4_array = numpy.empty(2)
adset4_array.fill(65535)
adset4 = file.create_dataset("a_uint16",data=adset4_array, dtype=numpy.uint16)
adset4.attrs.create('attr_a_uint16',data=adset4_array,dtype=numpy.uint16)

adset5_array = numpy.empty(2)
adset5_array.fill(-2147483647)
adset5 = file.create_dataset("a_int32",data=adset5_array, dtype='i4')
adset5.attrs.create('attr_a_int32',data=adset5_array,dtype=numpy.int32)

adset6_array = numpy.empty(2)
adset6_array.fill(4294967295)
adset6 = file.create_dataset("a_uint32",data=adset6_array, dtype=numpy.uint32)
adset6.attrs.create('attr_a_uint32',data=adset6_array,dtype=numpy.uint32)

adset7_array = numpy.empty(2)
adset7_array.fill(1.1)
adset7 = file.create_dataset("a_float32",data=adset7_array, dtype=h5py.h5t.NATIVE_FLOAT)
adset7.attrs.create('attr_a_float32',data=adset7_array,dtype=numpy.float32)

adset8_array = numpy.empty(2)
adset8_array.fill(-2.2)
adset8 = file.create_dataset("a_float64",data=adset8_array, dtype=h5py.h5t.NATIVE_DOUBLE)
adset8.attrs.create('attr_a_float64',data=adset8_array,dtype=numpy.double)


#
# Close the file before exiting
#
file.close()
