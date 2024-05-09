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

#define H5FILE_NAME   "comp_scalar_be.h5"
#define DATASETNAME   "comp_scalar_be"


int
main(void)
{

    /* First structure  and dataset*/
    typedef struct s1_t {
	int    a;
	float  b;
    } s1_t;
    s1_t       s1;
    hid_t      s1_tid;     /* File datatype identifier */
    hid_t      s2_tid;     /* File datatype identifier */


    int        i;
    hid_t      file, dataset, space; /* Handles */
    herr_t     status;


    /*
 *      * Initialize the data
 *           */
        s1.a = 10;
        s1.b = 100.1;

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
    H5Tinsert(s1_tid, "a_name", HOFFSET(s1_t, a), H5T_STD_I32BE);
    H5Tinsert(s1_tid, "b_name", HOFFSET(s1_t, b), H5T_IEEE_F32BE);
    s2_tid = H5Tcreate (H5T_COMPOUND, sizeof(s1_t));
    H5Tinsert(s2_tid, "a_name", HOFFSET(s1_t, a), H5T_NATIVE_INT);
    H5Tinsert(s2_tid, "b_name", HOFFSET(s1_t, b), H5T_NATIVE_FLOAT);
    
    /*
 *      * Create the dataset.
 *           */
    dataset = H5Dcreate2(file, DATASETNAME, s1_tid, space, H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

    /*
 *      * Wtite data to the dataset;
 *           */
    status = H5Dwrite(dataset, s2_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &s1);

    /*
 *      * Release resources
 *           */
    
    H5Tclose(s1_tid);
    H5Tclose(s2_tid);
    H5Sclose(space);
    H5Dclose(dataset);
    H5Fclose(file);

    return 0;
}
