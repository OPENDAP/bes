/*
  Test all supported signed int types (16, 32) variables and attributes.
  Have 1d, 2d (16) /  3d, 5d (32) of size 2 in each dimension.
  Fill in min/max numbers for the first and last item.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_int t_int.c 

  To generate the test file, run
  %./t_int

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_int.h5

  Copyright (C) 2012 The HDF Group
 */

#include "hdf5.h"

#define H5FILE_NAME "t_int.h5"
#define SIZE   2                      /* dataset dimensions */

int
main (void)
{
    hid_t       file,  dataset1, dataset2, dataset3, dataset4;  /* file and dataset handles */
    hid_t       datatype16, datatype32; 	     	 	/* handles */
    hid_t 	dataspace1, dataspace2,	dataspace3, dataspace4; /* handles */
    hsize_t     dimsf1[1], dimsf2[2], dimsf3[3], dimsf4[5];     /* dataset dimensions */
    hid_t       aid;    						/* dataspace identifiers */
    hid_t       attr1, attr2, attr3, attr4, attr5, attr6, attr7, attr8; /* attribute identifiers */
    herr_t      status;
    int16_t     data1[SIZE], data2[SIZE][SIZE]; 		/* data to write*/
    int32_t	data3[SIZE][SIZE][SIZE], data4[SIZE][SIZE][SIZE][SIZE][SIZE]; /* data to write */
    int         i, j, k, l, m, n;

    /*
     * Data and output buffer initialization.
     */
    /*
     * Assigns minimal and maximal values of int16 to datat1 and 
     * they will used to check boudary values.
     */
    data1[0] = -32768; 
    data1[1] =  32767;

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

    n = 0;
    for(i = 0; i < SIZE; i++)
	for(j = 0; j < SIZE; j++)
	   for(k = 0; k < SIZE; k++)
	      data3[i][j][k] = n++;
    /*
     * Assigns minimal and maximal values of int32 to data3 and 
     * they will be used to check boudary values.
     */
    data3[0][0][0] = -2147483648;
    data3[1][1][1] =  2147483647;

    n = 0;
    for(i = 0; i < SIZE; i++)
	for(j = 0; j < SIZE; j++)
	   for(k = 0; k < SIZE; k++)
	       for(l = 0; l < SIZE; l++)
		  for(m = 0; m < SIZE; m++)
		     data4[i][j][k][l][m] = n++;
    /*
     * Assigns minimal and maximal values of int32 to data4 and 
     * they will be used to check boudary values.
     */
    data4[0][0][0][0][0] = -2147483648;
    data4[1][1][1][1][1] =  2147483647;

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
    dimsf1[0] = SIZE;
    dataspace1 = H5Screate_simple(1, dimsf1, NULL);

    dimsf2[0] = SIZE;
    dimsf2[1] = SIZE;
    dataspace2 = H5Screate_simple(2, dimsf2, NULL);

    dimsf3[0] = SIZE;
    dimsf3[1] = SIZE;
    dimsf3[2] = SIZE;
    dataspace3 = H5Screate_simple(3, dimsf3, NULL);
 
    dimsf4[0] = SIZE;
    dimsf4[1] = SIZE;
    dimsf4[2] = SIZE;
    dimsf4[3] = SIZE;
    dimsf4[4] = SIZE;
    dataspace4 = H5Screate_simple(5, dimsf4, NULL);

    /*
     * Define datatype for the data in the file.
     */
    datatype16  = H5Tcopy(H5T_NATIVE_SHORT);

    datatype32 = H5Tcopy(H5T_NATIVE_INT);

    /*
     * Create a new dataset within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    dataset1 = H5Dcreate2(file, "d16_1", datatype16, dataspace1, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    dataset2 = H5Dcreate2(file, "d16_2", datatype16, dataspace2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    dataset3 = H5Dcreate2(file, "d32_1", datatype32, dataspace3, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    dataset4 = H5Dcreate2(file, "d32_2", datatype32, dataspace4, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(dataset1, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data1);

    status = H5Dwrite(dataset2, H5T_NATIVE_SHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data2);

    status = H5Dwrite(dataset3, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data3);

    status = H5Dwrite(dataset4, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data4);


    /*
     * Create scalar attributes.
     */
    aid    = H5Screate(H5S_SCALAR);

    attr1   = H5Acreate2(dataset1, "minimum", H5T_NATIVE_SHORT, aid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr1, H5T_NATIVE_SHORT, &data1[0]);
    attr2   = H5Acreate2(dataset1, "maximum", H5T_NATIVE_SHORT, aid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr2, H5T_NATIVE_SHORT, &data1[1]);

    attr3   = H5Acreate2(dataset2, "minimum", H5T_NATIVE_SHORT, aid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr3, H5T_NATIVE_SHORT, &data2[0][0]);
    attr4   = H5Acreate2(dataset2, "maximum", H5T_NATIVE_SHORT, aid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr4, H5T_NATIVE_SHORT, &data2[1][1]);

    attr5   = H5Acreate2(dataset3, "minimum", H5T_NATIVE_INT, aid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr5, H5T_NATIVE_INT, &data3[0][0][0]);
    attr6   = H5Acreate2(dataset3, "maximum", H5T_NATIVE_INT, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr6, H5T_NATIVE_INT, &data3[1][1][1]);

    attr7   = H5Acreate2(dataset4, "minimum", H5T_NATIVE_INT, aid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr7, H5T_NATIVE_INT, &data4[0][0][0][0][0]);
    attr8   = H5Acreate2(dataset4, "maximum", H5T_NATIVE_INT, aid, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr8, H5T_NATIVE_INT, &data4[1][1][1][1][1]);

    /*
     * Close attributes.
     */
    H5Sclose(aid);
    H5Aclose(attr1);
    H5Aclose(attr2);
    H5Aclose(attr3);
    H5Aclose(attr4);
    H5Aclose(attr5);
    H5Aclose(attr6);
    H5Aclose(attr7);
    H5Aclose(attr8);

    /*
     * Create 1D/2D attributes.
     */
    attr1   = H5Acreate2(dataset2, "value_1d", datatype16, dataspace1, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr1, datatype16, data1);
    attr2   = H5Acreate2(dataset2, "value_2d", datatype16, dataspace2, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Awrite(attr2, datatype16, data2);

    H5Aclose(attr1);
    H5Aclose(attr2);

    /*
     * Close/release resources.
     */
    H5Sclose(dataspace1);
    H5Sclose(dataspace2);
    H5Sclose(dataspace3);
    H5Sclose(dataspace4);

    H5Tclose(datatype16);
    H5Tclose(datatype32);

    H5Dclose(dataset1);
    H5Dclose(dataset2);
    H5Dclose(dataset3);
    H5Dclose(dataset4);

    H5Fclose(file);

    return 0;
}

