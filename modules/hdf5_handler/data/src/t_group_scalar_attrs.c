/*
  Test all supported types attributes under root group and group called "/g".


  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_group_scalar_attrs t_group_scalar_attrs.c 

  To generate the test file, run
  %./t_group_scalar_attrs

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_group_scalar_attrs.h5

  Copyright (C) 2012 The HDF Group
 */

#include "hdf5.h"
#include <string.h>

#define H5FILE_NAME "t_group_scalar_attrs.h5"

int
main (void)
{
    hid_t file;			/* file handle */
    hid_t grp;			/* group handles */
    hid_t filetype, memtype;	/* datatype handles */
    hid_t sid, space;		/* dataspace handles */
    hid_t attr;	/* attribute identifiers */
    herr_t status;

    char wdata = 'A';	/* write buffer */
    int8_t wdata2 = -128;
    uint8_t wdata3 = 127;
    int16_t wdata4 = 32676;
    int32_t wdata5 = 2147483647;
    float wdata6 = -1.12;
    double wdata7 = 44.55;
    uint16_t wdata8 = 65535;
    uint32_t wdata9 = 4294967295;
    char *wdata10 = "Parting is such sweet sorrow.";
    char *wdata11[4] = {"Parting", "is such", "sweet", "sorrow"};
    hsize_t dims[2] = {2,2};

    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create groups in the file.
     */
    grp = H5Gcreate2(file, "/g", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);    

    /*
     * Create dataspaces.
     */
    sid = H5Screate(H5S_SCALAR);
    space = H5Screate_simple(2, dims, NULL);

    /*
     * Test 2d string of variable length.
     */
    
    /*
     * Create file and memory datatypes.  For this example we will save
     * the strings as FORTRAN strings.
     */
    filetype = H5Tcopy (H5T_FORTRAN_S1);
    status   = H5Tset_size (filetype, H5T_VARIABLE);
    memtype  = H5Tcopy (H5T_C_S1);
    status   = H5Tset_size (memtype, H5T_VARIABLE);

    /*
     * Create the attribute and write the variable-length string data
     * to it.
     */
    attr  = H5Acreate2 (file, "vl_string", filetype, space,  H5P_DEFAULT, H5P_DEFAULT);
    H5Awrite (attr, memtype, wdata11);

    H5Sclose(space);
    H5Aclose(attr);
    H5Tclose(filetype);
    H5Tclose(memtype);

    /*
     * Test string of fixed length.
     */

    /*
     * Create file and memory datatypes.  For this example we will save
     * the strings as FORTRAN strings, therefore they do not need space
     * for the null terminator in the file.
     */
    filetype = H5Tcopy(H5T_FORTRAN_S1);
    status   = H5Tset_size (filetype, strlen(wdata10));
    memtype  = H5Tcopy(H5T_C_S1);
    status   = H5Tset_size (memtype, strlen(wdata10)+1); 

    /*
     * Create the attribute and write the string data to it.
     */
    attr = H5Acreate2 (file, "string", filetype, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr, memtype, wdata10);
    H5Aclose(attr);
    attr = H5Acreate2 (grp, "string", filetype, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr, memtype, wdata10);
    H5Aclose(attr);

    H5Tclose(filetype);
    H5Tclose(memtype);

    /*
     * Test unsigned int16.
     */
    attr = H5Acreate2(file, "uint16", H5T_NATIVE_USHORT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_USHORT, &wdata8);
    H5Aclose(attr);
    attr = H5Acreate2(grp, "uint16", H5T_NATIVE_USHORT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_USHORT, &wdata8);
    H5Aclose(attr);

    /*
     * Test unsigned int32.
     */
    attr = H5Acreate2(file, "uint32", H5T_NATIVE_UINT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_UINT, &wdata9);
    H5Aclose(attr);
    attr = H5Acreate2(grp, "uint32", H5T_NATIVE_UINT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_UINT, &wdata9);
    H5Aclose(attr);

    /*
     * Test float.
     */
    attr = H5Acreate2(file, "float", H5T_NATIVE_FLOAT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_FLOAT, &wdata6);
    H5Aclose(attr);
    attr = H5Acreate2(grp, "float", H5T_NATIVE_FLOAT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_FLOAT, &wdata6);
    H5Aclose(attr);

    /*
     * Test double.
     */
    attr = H5Acreate2(file, "double", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_DOUBLE, &wdata7);
    H5Aclose(attr);
    attr = H5Acreate2(grp, "double", H5T_NATIVE_DOUBLE, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_DOUBLE, &wdata7);
    H5Aclose(attr);

    /*
     * Test char.
     */
    attr  = H5Acreate2(file, "char", H5T_NATIVE_CHAR, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_CHAR, &wdata);
    H5Aclose(attr);
    attr  = H5Acreate2(grp, "char", H5T_NATIVE_CHAR, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_CHAR, &wdata);
    H5Aclose(attr);

    /*
     * Test signed char.
     */
    attr = H5Acreate2(file, "signed_char", H5T_NATIVE_SCHAR, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_SCHAR, &wdata2);
    H5Aclose(attr);
    attr = H5Acreate2(grp, "signed_char", H5T_NATIVE_SCHAR, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_SCHAR, &wdata2);
    H5Aclose(attr);

    /*
     * Test unsigned char.
     */
    attr = H5Acreate2(file, "unsigned_char", H5T_NATIVE_UCHAR, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_UCHAR, &wdata3);
    H5Aclose(attr);
    attr = H5Acreate2(grp, "unsigned_char", H5T_NATIVE_UCHAR, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_UCHAR, &wdata3);
    H5Aclose(attr);

    /*
     * Test int16.
     */
    attr = H5Acreate2(file, "int16", H5T_NATIVE_SHORT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_SHORT, &wdata4);
    H5Aclose(attr);
    attr = H5Acreate2(grp, "int16", H5T_NATIVE_SHORT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr, H5T_NATIVE_SHORT, &wdata4);
    H5Aclose(attr);    

    /*
     * Test int32.
     */
    attr = H5Acreate2(file, "int32", H5T_NATIVE_INT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_INT, &wdata5);
    H5Aclose(attr);
    attr = H5Acreate2(grp, "int32", H5T_NATIVE_INT, sid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_INT, &wdata5);
    H5Aclose(attr);


    H5Sclose(sid);
    H5Gclose(grp);
    H5Fclose(file);

    return 0;
}

