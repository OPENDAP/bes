#include "hdf5.h"
#define H5FILE_NAME "t_enum2_mt_scalar.h5"
#define H5FILE_NAME2 "t_enum2_mt_array.h5"
#define SIZE 4

int main(void)  {

    hid_t file, type, type2, space, dset;

    uint64_t val = 0;
    uint64_t data[4];

    int32_t ival = 0;
    int32_t idata[4];

    hsize_t size[1];
    size[0] = SIZE;
    type = H5Tenum_create(H5T_STD_U64LE);
    val = 0;
    H5Tenum_insert(type, "RED",   (const void*) &val);
    val = 1;
    H5Tenum_insert(type, "GREEN",   (const void*) &val);
    val = 2;
    H5Tenum_insert(type, "BLUE",   (const void*) &val);

    type2 = H5Tenum_create(H5T_STD_I32LE);
    ival = 1000;
    H5Tenum_insert(type2, "TRUE",   (const void*) &ival);
    ival = 2000;
    H5Tenum_insert(type2, "FALSE",   (const void*) &ival);


#if 0
    enum Color {RED,GREEN,BLUE};
    enum Color data[] = { BLUE,GREEN,RED,GREEN};
#endif
 
    data[0] = 2;
    data[1] = 1;
    data[2] = 0;
    data[3] = 1;

    idata[0] = 1000;
    idata[1] = 2000;
    idata[2] = 2000;
    idata[3] = 1000;
    
    file = H5Fcreate(H5FILE_NAME,H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    space = H5Screate(H5S_SCALAR);

    dset = H5Dcreate2(file, "enum_scalar_u64", type, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Dwrite(dset,type,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val);

    H5Dclose(dset);

    dset = H5Dcreate2(file, "enum_scalar_i32", type2, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Dwrite(dset,type2,H5S_ALL,H5S_ALL,H5P_DEFAULT,&ival);

    H5Dclose(dset);

    H5Sclose(space);

    H5Fclose(file);
 
    space = H5Screate_simple(1,size,NULL);
    file = H5Fcreate(H5FILE_NAME2,H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    dset = H5Dcreate2(file, "enum_array", type, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dwrite(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    H5Dclose(dset);

    dset = H5Dcreate2(file, "enum_array2", type2, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dwrite(dset, type2, H5S_ALL, H5S_ALL, H5P_DEFAULT, idata);
    H5Dclose(dset);

    H5Tclose(type);
    H5Tclose(type2);
    H5Sclose(space);
    H5Fclose(file);
    return 0;

}
