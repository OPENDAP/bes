/*
  Test array data type for HDF5 handler with default option.
 
  Compilation instruction:
 
  %/path/to/hdf5/bin/h5cc -o d_objref d_objref.c 
 
  To generate the test file, run
  %./d_objref
 
  To view the test file, run
  %/path/to/hdf5/bin/h5dump d_objref.h5
 
  Copyright (C) 2012 The HDF Group
*/
 
 
#include <stdio.h>
#include <stdlib.h>
#include "hdf5.h"
 
#define H5FILE_NAME            "d_objref.h5"
#define DATASET         "DS1"
#define DIM0            2
#define NX              5
#define NY              6
#define RANK            2
 
int
main (void)
{
    hid_t       file, space, dset, obj;     /* Handles */
    herr_t      status;
    hsize_t     dims[1] = {DIM0};
    hsize_t     dimsf[2];  
    hobj_ref_t  wdata[DIM0];                /* Write buffer */
 
    int         data[NX][NY];          /* data to write */
    int         i, j;

    /*
     * Data  and output buffer initialization.
     */
    for(j = 0; j < NX; j++)
        for(i = 0; i < NY; i++)
            data[j][i] = i + j;
    /*
     * 0 1 2 3 4 5
     * 1 2 3 4 5 6
     * 2 3 4 5 6 7
     * 3 4 5 6 7 8
     * 4 5 6 7 8 9
     */


    /*
     * Create a new file using the default properties.
     */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Describe the size of the array and create the data space for fixed
     * size dataset.
     */
    dimsf[0] = NX;
    dimsf[1] = NY;

 
    /*
     * Create a dataset with a null dataspace.
     */
    space = H5Screate_simple (RANK,dimsf,NULL);
    obj = H5Dcreate(file, "DS2", H5T_STD_I32LE, space, H5P_DEFAULT,
                    H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(obj, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

   
    status = H5Dclose (obj);
    status = H5Sclose (space);
 
    /*
     * Create a group.
     */
    obj = H5Gcreate (file, "G1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Gclose (obj);
 
    /*
     * Create references to the previously created objects.  Passing -1
     * as space_id causes this parameter to be ignored.  Other values
     * besides valid dataspaces result in an error.
     */
    status = H5Rcreate (&wdata[0], file, "G1", H5R_OBJECT, -1);
    status = H5Rcreate (&wdata[1], file, "DS2", H5R_OBJECT, -1);
 
    /*
     * Create dataspace.  Setting maximum size to NULL sets the maximum
     * size to be the current size.
     */
    space = H5Screate_simple (1, dims, NULL);
 
    /*
     * Create the dataset and write the object references to it.
     */
    dset = H5Dcreate(file, DATASET, H5T_STD_REF_OBJ, space, H5P_DEFAULT,
                     H5P_DEFAULT, H5P_DEFAULT);
    status = H5Dwrite(dset, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                      wdata);
 
    /*
     * Close and release resources.
     */
    status = H5Dclose(dset);
    status = H5Sclose(space);
    status = H5Fclose(file);
    return 0;
}
