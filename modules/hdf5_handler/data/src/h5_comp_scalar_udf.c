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

#define H5FILE_NAME   "comp_scalar_udf_nfv.h5"
#define H5FILE_NAME2   "comp_scalar_udf_fv.h5"
#define DATASETNAME   "comp_scalar"

int
main(void)
{

    /* First structure  and dataset*/
    typedef struct s1_t {
        double a;
        short  b;
	float  c;
    } s1_t;
    s1_t       s1,sf;
    hid_t      s1_tid;     /* File datatype identifier */


    int        i;
    hid_t      file, space, dataset; /* Handles */
    herr_t     status;


    /*
 *      * Initialize the data
 *           */
        s1.a = 1.1;
        s1.b = 1;
        s1.c = -2.2;

        sf.a = -9999.9;
        sf.b = INT16_MAX;
        sf.c = 999.9;

    
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
    H5Tinsert(s1_tid, "a_name", HOFFSET(s1_t, a), H5T_NATIVE_DOUBLE);
    H5Tinsert(s1_tid, "b_name", HOFFSET(s1_t, b), H5T_NATIVE_SHORT);
    H5Tinsert(s1_tid, "c_name", HOFFSET(s1_t, c), H5T_NATIVE_FLOAT);
    
    /*
 *      * Create the dataset.
 *           */
    hid_t plist_id = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_fill_value(plist_id, s1_tid,&sf);
    dataset = H5Dcreate2(file, DATASETNAME, s1_tid, space, H5P_DEFAULT,plist_id,H5P_DEFAULT);

    /*
 *      * Wtite data to the dataset;
 *           */
    status = H5Dwrite(dataset, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &s1);
    
    H5Dclose(dataset);
    H5Fclose(file);

    file = H5Fcreate(H5FILE_NAME2, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    dataset = H5Dcreate2(file, DATASETNAME, s1_tid, space, H5P_DEFAULT,plist_id,H5P_DEFAULT);

    H5Dclose(dataset);
    H5Tclose(s1_tid);
    H5Sclose(space);
    H5Pclose(plist_id);
    H5Fclose(file);

    return 0;
}
