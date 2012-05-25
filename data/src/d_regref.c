/*
  Test array data type for HDF5 handler with default option.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o d_regref d_regref.c 

  To generate the test file, run
  %./d_regref

  To view the test file, run
  %/path/to/hdf5/bin/h5dump d_regref.h5

  Copyright (C) 2012 The HDF Group
*/


#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"

#define FILE            "d_regref.h5"
#define DATASET         "DS1"
#define DATASET2        "DS2"
#define DIM0            2
#define DS2DIM0         3
#define DS2DIM1         16

int
main (void)
{
    hid_t               file, space, dset, dset2;      /* Handles */

    herr_t              status;
    hsize_t dims[1] = {DIM0};
    hsize_t dims2[2] = {DS2DIM0, DS2DIM1};
    hsize_t coords[4][2] = { {0,  1}, {2, 11}, {1,  0}, {2,  4} };
    hsize_t start[2] = {0, 0};
    hsize_t stride[2] = {2, 11};
    hsize_t count[2] = {2, 2};
    hsize_t block[2] = {1, 3};

    hdset_reg_ref_t     wdata[DIM0];                /* Write buffer */

    char wdata2[DS2DIM0][DS2DIM1] = 
        {{0,1,2,3,5,6,7,8,9,10,11,12,13,14,15},
         {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31},
         {32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47}};

    /*
     * Create a new file using the default properties.
     */
    file = H5Fcreate (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create a dataset with character data.
     */
    space = H5Screate_simple (2, dims2, NULL);
    dset2 = H5Dcreate (file, DATASET2, H5T_STD_I8LE, space, H5P_DEFAULT,
                       H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite (dset2, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                       wdata2);

    /*
     * Create reference to a list of elements in dset2.
     */
    status = H5Sselect_elements (space, H5S_SELECT_SET, 4, coords[0]);
    status = H5Rcreate (&wdata[0], file, DATASET2, H5R_DATASET_REGION, space);

    /*
     * Create reference to a hyperslab in dset2, close dataspace.
     */
    status = H5Sselect_hyperslab (space, H5S_SELECT_SET, start, stride, count,
                                  block);
    status = H5Rcreate (&wdata[1], file, DATASET2, H5R_DATASET_REGION, space);
    status = H5Sclose (space);

    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    space = H5Screate_simple (1, dims, NULL);

    /*
     * Create the dataset and write the region references to it.
     */
    dset = H5Dcreate (file, DATASET, H5T_STD_REF_DSETREG, space, H5P_DEFAULT,
                      H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite (dset, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                       wdata);

    /*
     * Close and release resources.
     */
    status = H5Dclose (dset);
    status = H5Dclose (dset2);
    status = H5Sclose (space);
    status = H5Fclose (file);



    return 0;
}
