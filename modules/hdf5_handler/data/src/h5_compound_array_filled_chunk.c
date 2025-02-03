/*
Test scalar dataset of a compound data type for HDF5 handler with default option.

Compilation instruction:

%/path/to/hdf5/bin/h5cc -o h5_comp_array h5_comp_array.c 

To generate the test file, run
%./h5_comp_array

To view the test file, run
%/path/to/hdf5/bin/h5dump comp_array.h5

Copyright (C) 2012-2015 The HDF Group
*/


#include "hdf5.h"

#define H5FILE_NAME   "compound_filled_chunk.h5"
#define DATASETNAME   "comp_array"

int
main(void)
{

    /* First structure  and dataset*/
    typedef struct s1_t {
        double a;
        short  b;
	float  c[2];
    } s1_t;
    s1_t       s1[4],sf;
    hid_t      s1_tid;     /* File datatype identifier */
    hsize_t array_dims[] ={4};
    hsize_t array_dimf[] ={2};
    hsize_t offset[1];


    int        i,j;
    hid_t      file, space, mspace, dataset; /* Handles */
    herr_t     status;


     for (j = 0; j<2;j++) {
        s1[j].a = j;
        s1[j].b = j+2;
        for (i = 0; i<2;i++)
            s1[j].c[i] = i+j+3;
     }

    
    /*
 *      * Create the data space.
 *           */
    space = H5Screate_simple(1,array_dims,NULL);
    mspace = H5Screate_simple(1,array_dimf,NULL);

    /*
 *      * Create the file.
 *           */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /* Create the array data type for the character array             */
    hid_t array_float_tid = H5Tarray_create2(H5T_NATIVE_FLOAT, 1, array_dimf);

    /*
 *      * Create the memory data type.
 *           */
    s1_tid = H5Tcreate (H5T_COMPOUND, sizeof(s1_t));
    H5Tinsert(s1_tid, "a_name", HOFFSET(s1_t, a), H5T_NATIVE_DOUBLE);
    H5Tinsert(s1_tid, "b_name", HOFFSET(s1_t, b), H5T_NATIVE_SHORT);
    H5Tinsert(s1_tid, "c_name", HOFFSET(s1_t, c), array_float_tid);
    
    /*
 *      * Create the dataset.
 *           */
    hid_t plist_id = H5Pcreate(H5P_DATASET_CREATE);

    //default fill value.
    H5Pset_chunk(plist_id,1,array_dimf);
    dataset = H5Dcreate2(file, DATASETNAME, s1_tid, space, H5P_DEFAULT,plist_id,H5P_DEFAULT);

    offset[0] = 0;
    status = H5Sselect_hyperslab(space, H5S_SELECT_SET, offset, NULL,
                                 array_dimf, NULL);

    // The first chunk has real data and the second chunk is filled with the default one.
    status = H5Dwrite(dataset, s1_tid, mspace, space, H5P_DEFAULT, &s1);
    
    H5Pclose(plist_id);
    H5Tclose(s1_tid);
    H5Sclose(space);
    H5Sclose(mspace);
    H5Dclose(dataset);
    H5Fclose(file);


    return 0;
}
