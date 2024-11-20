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

#define H5FILE_NAME   "comp_array2_udf_nfv.h5"
#define H5FILE_NAME2   "comp_array2_udf_fv.h5"
#define DATASETNAME   "comp_array"

int
main(void)
{

    /* First structure  and dataset*/
    typedef struct s1_t {
        double a;
        short  b[3];
	float  c[2];
    } s1_t;
    s1_t       s1[2],sf;
    hid_t      s1_tid;     /* File datatype identifier */
    hsize_t array_dims[] ={3};
    hsize_t array_dimf[] ={2};


    int        i,j;
    hid_t      file, space, dataset; /* Handles */
    herr_t     status;


    /*
 *      * Initialize the data
 *           */
     for (j = 0; j<2;j++) {
        s1[j].a = 1.1 +j*10;
        for (i = 0; i<3;i++)
            s1[j].b[i] = i +1+j*10;
        for (i = 0; i<2;i++)
            s1[j].c[i] = -2.2 +i+j*10;
     }

        sf.a = -9999.9;
        for (i = 0; i<3;i++)
            sf.b[i] = INT16_MAX-i;
        for (i = 0; i<2;i++)
            sf.c[i] = 999.9-i*100;

    
    /*
 *      * Create the data space.
 *           */
    space = H5Screate_simple(1,array_dimf,NULL);

    /*
 *      * Create the file.
 *           */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /* Create the array data type for the character array             */
    hid_t array_short_tid = H5Tarray_create2(H5T_NATIVE_SHORT, 1, array_dims);

    /* Create the array data type for the character array             */
    hid_t array_float_tid = H5Tarray_create2(H5T_NATIVE_FLOAT, 1, array_dimf);

    /*
 *      * Create the memory data type.
 *           */
    s1_tid = H5Tcreate (H5T_COMPOUND, sizeof(s1_t));
    H5Tinsert(s1_tid, "a_name", HOFFSET(s1_t, a), H5T_NATIVE_DOUBLE);
    H5Tinsert(s1_tid, "b_name", HOFFSET(s1_t, b), array_short_tid);
    H5Tinsert(s1_tid, "c_name", HOFFSET(s1_t, c), array_float_tid);
    
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

    H5Tclose(array_short_tid);
    H5Tclose(array_float_tid);
    H5Dclose(dataset);
    H5Tclose(s1_tid);
    H5Sclose(space);
    H5Pclose(plist_id);
    H5Fclose(file);

    return 0;
}
