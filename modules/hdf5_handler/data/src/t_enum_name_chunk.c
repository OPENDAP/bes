#include "hdf5.h"
#define H5FILE_NAME "t_enum_name_chunk_array.h5"
#define SIZE 4

int main(void)  {

    hid_t file, type, space, dset;

    uint8_t val = 0;
    uint8_t data[4];

    hsize_t chunk_size[1], size[1];
    size[0] = SIZE;
    chunk_size[0] = SIZE/2;

    data[0] = 2;
    data[1] = 1;
    data[2] = 0;
    data[3] = 1;
    
    space = H5Screate_simple(1,size,NULL);
    file = H5Fcreate(H5FILE_NAME,H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    type = H5Tenum_create(H5T_STD_U8LE);
    val = 0;
    H5Tenum_insert(type, "RED",   (const void*) &val);
    val = 1;
    H5Tenum_insert(type, "GREEN",   (const void*) &val);
    val = 2;
    H5Tenum_insert(type, "BLUE",   (const void*) &val);
    H5Tcommit2(file,"Color",type,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

    hid_t plist_id = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(plist_id,1,chunk_size);
    dset = H5Dcreate2(file, "enum_chunk_array", type, space, H5P_DEFAULT, plist_id, H5P_DEFAULT);
    H5Dwrite(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
    H5Pclose(plist_id);

    H5Tclose(type);
    H5Dclose(dset);
    H5Sclose(space);
    H5Fclose(file);
    return 0;

}
