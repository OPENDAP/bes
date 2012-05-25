/*
Test array data type for HDF5 handler with default option.

Compilation instruction:

%/path/to/hdf5/bin/h5cc -o d_array d_array.c 

To generate the test file, run
%./d_array

To view the test file, run
%/path/to/hdf5/bin/h5dump d_array.h5

Copyright (C) 2012 The HDF Group
*/

#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"

#define FILE            "d_array.h5"
#define DATASET         "Array"
#define DIM0            4
#define ADIM0           3
#define ADIM1           5

int
main (void)
{
    hid_t       file, filetype, memtype, space, dset;
                                                /* Handles */
    herr_t      status;
    hsize_t     dims[1] = {DIM0},
                adims[2] = {ADIM0, ADIM1};
    int         wdata[DIM0][ADIM0][ADIM1],      /* Write buffer */
                i, j, k;

    /*
     * Initialize data.  i is the element in the dataspace, j and k the
     * elements within the array datatype.
     */
    for (i=0; i<DIM0; i++)
        for (j=0; j<ADIM0; j++)
            for (k=0; k<ADIM1; k++)
                wdata[i][j][k] = i * j - j * k + i * k;

    /*
     * Create a new file using the default properties.
     */
    file = H5Fcreate (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create array datatypes for file and memory.
     */
    filetype = H5Tarray_create (H5T_STD_I32LE, 2, adims);
    memtype = H5Tarray_create (H5T_NATIVE_INT, 2, adims);

    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    space = H5Screate_simple (1, dims, NULL);

    /*
     * Create the dataset and write the array data to it.
     */
    dset = H5Dcreate (file, DATASET, filetype, space, H5P_DEFAULT, H5P_DEFAULT,
                H5P_DEFAULT);
    status = H5Dwrite (dset, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                wdata[0][0]);

    /*
     * Close and release resources.
     */
    status = H5Dclose (dset);
    status = H5Sclose (space);
    status = H5Tclose (filetype);
    status = H5Tclose (memtype);
    status = H5Fclose (file);

    return 0;
}
