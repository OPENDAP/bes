/*
Test scalar dataset of a compound data type for HDF5 handler with default option.

Compilation instruction:

%/path/to/hdf5/bin/h5cc -o h5_comp_scalar h5_comp_scalar.c 

To generate the test file, run
%./h5_comp_scalar

To view the test file, run
%/path/to/hdf5/bin/h5dump comp_scalar.h5

Copyright (C) 2012-2015 The HDF Group
*/


#include "hdf5.h"

#define H5FILE_NAME   "comp_scalar.h5"
#define DATASETNAME   "comp_scalar"
#define LENGTH        10
#define RANK          1


int
main(void)
{

    /* First structure  and dataset*/
    typedef struct s1_t {
        char   a;
        unsigned char b;
        short  c;
        unsigned short d;
	int    e;
        unsigned int f;
        const char * vs;
        char   s[6];
	float  g;
	double h;
    } s1_t;
    s1_t       s1;
    hid_t      s1_tid;     /* File datatype identifier */
    hid_t      fstr_tid,vstr_tid;


    int        i;
    hid_t      file, dataset, space; /* Handles */
    herr_t     status;
    hsize_t    dim[] = {LENGTH};   /* Dataspace dimensions */

    const char *quote ={"dummy variable length string"};
    char chararray[5] = {'T','E','S','T','0'};

    /*
 *      * Initialize the data
 *           */
        s1.a = INT8_MAX;
        s1.b = UINT8_MAX;
        s1.c = INT16_MAX;
        s1.d = UINT16_MAX;
        s1.e = INT32_MAX;
        s1.f = UINT32_MAX;
        s1.vs = quote;
        for(i = 0;i<5;i++)
            s1.s[i] = chararray[i];
        s1.g = 100.1;
        s1.h = -10000.2;

    /* create the string datatype */
    fstr_tid = H5Tcopy(H5T_C_S1);
    H5Tset_size(fstr_tid,6);
    H5Tset_strpad(fstr_tid,H5T_STR_NULLTERM);

    vstr_tid = H5Tcopy(H5T_C_S1);
    H5Tset_size(vstr_tid,H5T_VARIABLE);
    
    /*
 *      * Create the data space.
 *           */
    space = H5Screate(H5S_SCALAR);

    /*
 *      * Create the file.
 *           */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
 *      * Create the memory data type.
 *           */
    s1_tid = H5Tcreate (H5T_COMPOUND, sizeof(s1_t));
    H5Tinsert(s1_tid, "a_name", HOFFSET(s1_t, a), H5T_NATIVE_CHAR);
    H5Tinsert(s1_tid, "b_name", HOFFSET(s1_t, b), H5T_NATIVE_UCHAR);
    H5Tinsert(s1_tid, "c_name", HOFFSET(s1_t, c), H5T_NATIVE_SHORT);
    H5Tinsert(s1_tid, "d_name", HOFFSET(s1_t, d), H5T_NATIVE_USHORT);
    H5Tinsert(s1_tid, "e_name", HOFFSET(s1_t, e), H5T_NATIVE_INT);
    H5Tinsert(s1_tid, "f_name", HOFFSET(s1_t, f), H5T_NATIVE_UINT);
    H5Tinsert(s1_tid, "vs_name", HOFFSET(s1_t, vs), vstr_tid);
    H5Tinsert(s1_tid, "s_name", HOFFSET(s1_t, s), fstr_tid);
    H5Tinsert(s1_tid, "g_name", HOFFSET(s1_t, g), H5T_NATIVE_FLOAT);
    H5Tinsert(s1_tid, "h_name", HOFFSET(s1_t, h), H5T_NATIVE_DOUBLE);
    
    /*
 *      * Create the dataset.
 *           */
    dataset = H5Dcreate2(file, DATASETNAME, s1_tid, space, H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

    /*
 *      * Wtite data to the dataset;
 *           */
    status = H5Dwrite(dataset, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &s1);

    /*
 *      * Release resources
 *           */
    
    H5Tclose(vstr_tid);
    H5Tclose(fstr_tid);
    H5Tclose(s1_tid);
    H5Sclose(space);
    H5Dclose(dataset);
    H5Fclose(file);

    return 0;
}
