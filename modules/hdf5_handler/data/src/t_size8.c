/*
  Test character, int8, and uint8 type variables and attributes.	
  Have 1d (char) / 2d (int8) / 3d, 5d (uint8) of size 2 in each dimension.
  Fill in min/max numbers for the first and last item.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_size8 t_size8.c 

  To generate the test file, run
  %./t_size8

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_size8.h5

  Copyright (C) 2012 The HDF Group
 */

#include "hdf5.h"

#define H5FILE_NAME "t_size8.h5"
#define SIZE   2                      /* dataset dimensions */

int
main (void)
{
    hid_t       file,  dataset1, dataset2, dataset3, dataset4;  /* file and dataset handles */
    hid_t       datatype1, datatype2, datatype3;     	 	/* handles */
    hid_t 	dataspace1, dataspace2,	dataspace3, dataspace4; /* handles */
    hsize_t     dimsf1[1], dimsf2[2], dimsf3[3], dimsf4[5];     /* dataset dimensions */
    hid_t	aid;	/* dataspace identifiers */
    hid_t   	attr1, attr2, attr3, attr4, attr5, attr6, attr7, attr8;	/* attribute identifiers */
    herr_t      status;
    char	data1[SIZE];
    int8_t	data2[SIZE][SIZE]; 		
    uint8_t	data3[SIZE][SIZE][SIZE], data4[SIZE][SIZE][SIZE][SIZE][SIZE]; /* data to write */
    int         i, j, k, l, m, n;

    /*
     * Data and output buffer initialization.
     */
    data1[0] = 0;   //NUL
    data1[1] = 127; //DEL

    n = 0;
    for(i = 0; i < SIZE; i++)
	for(j = 0; j < SIZE; j++)
	    data2[i][j] = n++;
    data2[0][0] = -128;
    data2[1][1] =  127; 

    n = 0;
    for(i = 0; i < SIZE; i++)
	for(j = 0; j < SIZE; j++)
	   for(k = 0; k < SIZE; k++)
	      data3[i][j][k] = n++;
    data3[0][0][0] = 0;
    data3[1][1][1] = 255;

    n = 0;
    for(i = 0; i < SIZE; i++)
	for(j = 0; j < SIZE; j++)
	   for(k = 0; k < SIZE; k++)
	       for(l = 0; l < SIZE; l++)
		  for(m = 0; m < SIZE; m++)
		     data4[i][j][k][l][m] = n++;
    data4[0][0][0][0][0] = 0;
    data4[1][1][1][1][1] = 255;

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
    datatype1  = H5Tcopy(H5T_NATIVE_CHAR);

    datatype2 = H5Tcopy(H5T_NATIVE_SCHAR);

    datatype3 = H5Tcopy(H5T_NATIVE_UCHAR);

    /*
     * Create a new dataset within the file using defined dataspace and
     * datatype and default dataset creation properties.
     */
    dataset1 = H5Dcreate2(file, "dchar", datatype1, dataspace1, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    dataset2 = H5Dcreate2(file, "dschar", datatype2, dataspace2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    dataset3 = H5Dcreate2(file, "duchar_1", datatype3, dataspace3, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    dataset4 = H5Dcreate2(file, "duchar_2", datatype3, dataspace4, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write the data to the dataset using default transfer properties.
     */
    status = H5Dwrite(dataset1, datatype1, H5S_ALL, H5S_ALL, H5P_DEFAULT, data1);

    status = H5Dwrite(dataset2, datatype2, H5S_ALL, H5S_ALL, H5P_DEFAULT, data2);

    status = H5Dwrite(dataset3, datatype3, H5S_ALL, H5S_ALL, H5P_DEFAULT, data3);

    status = H5Dwrite(dataset4, datatype3, H5S_ALL, H5S_ALL, H5P_DEFAULT, data4);

    /*
     * Create scalar attributes.
     */
    aid    = H5Screate(H5S_SCALAR);
    
    attr1  = H5Acreate2(dataset1, "minimum", H5T_NATIVE_CHAR, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr1, H5T_NATIVE_CHAR, &data1[0]);
    attr2  =  H5Acreate2(dataset1, "maximum", H5T_NATIVE_CHAR, aid, H5P_DEFAULT, H5P_DEFAULT); 
    status = H5Awrite(attr2, H5T_NATIVE_CHAR, &data1[1]);

    attr3  = H5Acreate2(dataset2, "minimum", H5T_NATIVE_SCHAR, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr3, H5T_NATIVE_SCHAR, &data2[0][0]);
    attr4  =  H5Acreate2(dataset2, "maximum", H5T_NATIVE_SCHAR, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr4, H5T_NATIVE_SCHAR, &data2[1][1]);

    attr5  = H5Acreate2(dataset3, "minimum", H5T_NATIVE_UCHAR, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr5, H5T_NATIVE_UCHAR, &data3[0][0][0]);
    attr6  = H5Acreate2(dataset3, "maximum", H5T_NATIVE_UCHAR, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr6, H5T_NATIVE_UCHAR, &data3[1][1][1]);

    attr7  = H5Acreate2(dataset4, "minimum", H5T_NATIVE_UCHAR, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr7, H5T_NATIVE_UCHAR, &data4[0][0][0][0][0]);
    attr8  = H5Acreate2(dataset4, "maximum", H5T_NATIVE_UCHAR, aid, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite(attr8, H5T_NATIVE_UCHAR, &data4[1][1][1][1][1]);

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
     * Close/release resources.
     */
    H5Sclose(dataspace1);
    H5Sclose(dataspace2);
    H5Sclose(dataspace3);
    H5Sclose(dataspace4);

    H5Tclose(datatype1);
    H5Tclose(datatype2);
    H5Tclose(datatype3);

    H5Dclose(dataset1);
    H5Dclose(dataset2);
    H5Dclose(dataset3);
    H5Dclose(dataset4);

    H5Fclose(file);

    return 0;
}

