/*
  Test scalar dataset.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_int_scalar t_int_scalar.c 

  To generate the test file, run
  %./t_int_scalar

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_int_scalar.h5

  Copyright (C) 2012 The HDF Group
 */

#include "hdf5.h"

#define H5FILE_NAME "t_int_scalar.h5"

int
main (void)
{
    hid_t       file,  dataset;		/* file and dataset handles */
    hid_t 	dataspace, datatype;	/* handles */
    hid_t       attr; 			/* attribute identifiers */
    herr_t      status;
    int wdata = 45;

    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Describe the size of the array and create the data space for fixed
     * size dataset.
     */
    dataspace = H5Screate(H5S_SCALAR);


    /*
     * Define datatype for the data in the file.
     */
    datatype = H5Tcopy(H5T_NATIVE_INT);


    /*
     * Create a new dataset within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    dataset = H5Dcreate2(file, "scalar", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);


    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &wdata);


    /*
     * Create scalar attributes.
     */
    attr = H5Acreate2(dataset, "value", H5T_NATIVE_INT, dataspace, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr, H5T_NATIVE_INT, &wdata);


    /*
     * Close attributes.
     */
    H5Aclose(attr);


    /*
     * Close/release resources.
     */
    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Dclose(dataset);

    H5Fclose(file);

    return 0;
}

