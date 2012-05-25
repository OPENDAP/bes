/*
  Test dataspace with 0 element.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_space_zero t_space_zero.c 

  To generate the test file, run
  %./t_space_zero

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_space_zero.h5

  Copyright (C) 2012 The HDF Group
 */

#include "hdf5.h"

#define H5FILE_NAME "t_space_zero.h5"
#define SIZE   2                      /* dataset dimensions */

int
main (void)
{
    hid_t       file,  dataset2;        /* file and dataset handles */
    hid_t       datatype16;             /* handles */
    hid_t 	dataspace2;     /* handles */
    hsize_t     dimsf2[2];       /* dataset dimensions */
    hid_t       aid;           /* dataspace identifiers */
    hid_t       attr2;         /* attribute identifiers */
    herr_t      status;
    int16_t     data2[SIZE][SIZE]; /* data to write*/
    int         i, j, n;

    n = 0;
    for(i = 0; i < SIZE; i++)
	for(j = 0; j < SIZE; j++)
	    data2[i][j] = n++;
    /*
     * Assigns minimal and maximal values of int16 to data2 and 
     * they will be used to check boudary values.
     */
    data2[0][0] = -32768;
    data2[1][1] =  32767; 

    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Set each dimension size to 0.
     */
    dimsf2[0] = 0;
    dimsf2[1] = 0;

    dataspace2 = H5Screate_simple(2, dimsf2, NULL);

    /*
     * Define datatype for the data in the file.
     */

    datatype16  = H5Tcopy(H5T_NATIVE_SHORT);
    /*
     * Create a new dataset within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    dataset2 = H5Dcreate2(file, "dataset_2d", datatype16, dataspace2, 
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data although it has no effect because each dim size is 0.
     */
    status = H5Dwrite(dataset2, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, 
       H5P_DEFAULT, data2); 

    /*
     * Create 2D attributes.
     */
    attr2   = H5Acreate2(dataset2, "attribute_2d", datatype16, dataspace2, 
                         H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data although it has no effect because each dim size is 0.
     */
    status  = H5Awrite(attr2, datatype16, data2);
    H5Aclose(attr2);

    /*
     * Close/release resources.
     */
    H5Dclose(dataset2);
    H5Tclose(datatype16);
    H5Sclose(dataspace2);
    H5Fclose(file);

    return 0;
}

