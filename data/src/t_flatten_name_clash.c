/*
  Test if groups are flattened properly and name clashing is handled after 
  flattening. 

  t_flatten_name_clash.h5 will create 2 groups of depth 1 (a_b_c) and 
  depth 3 (a/b/c) and a put a variable with the same name (d) under them.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_flatten_name_clash t_flatten_name_clash.c 

  To generate the test file, run
  %./t_flatten_name_clash

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_flatten_name_clash.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"

#define H5FILE_NAME "t_flatten_name_clash.h5"
#define NX     5                      /* dataset dimensions */
#define NY     6
#define NX2    2
#define NY2    2
#define RANK   2

int
main (void)
{
    hid_t       file; 			       /* file handle */
    hid_t	dataset, dataset2; 	       /* dataset handles */
    hid_t 	grp_abc, grp_a, grp_b, grp_c;  /* group handlers */
    hid_t       datatype; 		       /* handle */
    hid_t 	dataspace, dataspace2; 	       /* handles */
    hsize_t     dimsf[RANK], dimsf2[RANK];     /* dataset dimensions */
    herr_t      status;
    int         data[NX][NY], data2[NX2][NY2]; /* data to write */
    int         i, j, k;

    /*
     * Data and output buffer initialization.
     */
    for(i = 0; i < NX; i++)
	for(j = 0; j < NY; j++)
	    data[i][j] = i + j;

    k = 0;
    for(i = 0; i < NX2; i++)
	for(j = 0; j < NY2; j++)
	    data2[i][j] = k++; 
    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create groups in the file.
     */
    grp_abc = H5Gcreate2(file, "/a_b_c", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    grp_a = H5Gcreate2(file,  "/a",     H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    grp_b = H5Gcreate2(grp_a, "b", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    grp_c = H5Gcreate2(grp_b, "c", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Describe the size of the array and create the data space for fixed
     * size dataset.
     */
    dimsf[0] = NX;
    dimsf[1] = NY;
    dataspace = H5Screate_simple(RANK, dimsf, NULL);

    dimsf2[0]  = NX2;
    dimsf2[1]  = NY2;
    dataspace2 = H5Screate_simple(RANK, dimsf2, NULL);

    /*
     * Define datatype for the data in the file.
     * We will store little endian INT numbers.
     */
    datatype = H5Tcopy(H5T_NATIVE_INT);

    /*
     * Create new datasets within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    dataset = H5Dcreate2(grp_c, "d", datatype, dataspace, 
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    dataset2 = H5Dcreate2(grp_abc, "d", datatype, dataspace2, 
                          H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
                      data);

    status = H5Dwrite(dataset2, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
                      data2);
    
    /*
     * Close/release resources.
     */
    H5Sclose(dataspace);
    H5Sclose(dataspace2);
    H5Tclose(datatype);
    H5Dclose(dataset);
    H5Dclose(dataset2);
    H5Gclose(grp_abc);
    H5Gclose(grp_a);
    H5Gclose(grp_b);
    H5Gclose(grp_c);
    H5Fclose(file);

    return 0;
}

