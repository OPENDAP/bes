#include "hdf5.h"
#define H5FILE_NAME "t_enum_name_in_grp.h5"
#define SIZE 4

int main(void)  {

    hid_t file, type, space, dset;

    uint8_t val = 0;
    uint8_t data[4];

    hsize_t size[1];
    size[0] = SIZE;
    hid_t grp;

    file = H5Fcreate(H5FILE_NAME,H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    type = H5Tenum_create(H5T_STD_U8LE);
    val = 0;
    H5Tenum_insert(type, "RED",   (const void*) &val);
    val = 1;
    H5Tenum_insert(type, "GREEN",   (const void*) &val);
    val = 2;
    H5Tenum_insert(type, "BLUE",   (const void*) &val);
    H5Tcommit2(file,"Color",type,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

 
    data[0] = 2;
    data[1] = 1;
    data[2] = 0;
    data[3] = 1;
    
    space = H5Screate(H5S_SCALAR);

    dset = H5Dcreate2(file, "enum_scalar", type, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Dwrite(dset,type,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val);

    H5Tclose(type);
    H5Sclose(space);
    H5Dclose(dset);
 
    grp = H5Gcreate2(file, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
    space = H5Screate_simple(1,size,NULL);
    type = H5Tenum_create(H5T_STD_U8LE);
    val = 0;
    H5Tenum_insert(type, "RED",   (const void*) &val);
    val = 1;
    H5Tenum_insert(type, "GREEN",   (const void*) &val);
    val = 2;
    H5Tenum_insert(type, "BLUE",   (const void*) &val);
    H5Tcommit2(grp,"Color",type,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

    dset = H5Dcreate2(grp, "enum_array", type, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Dwrite(dset, type, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    H5Tclose(type);
    H5Dclose(dset);
    H5Sclose(space);
    H5Gclose(grp);
    H5Fclose(file);
    return 0;

}
