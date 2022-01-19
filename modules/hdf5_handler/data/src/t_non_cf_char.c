/*
  Test group/variable/attribute that has all non-CF characters.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_non_cf_char t_non_cf_char.c

  To generate the test file, run
  %./t_non_cf_char

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_non_cf_char.h5

  Copyright (C) 2012 The HDF Group
 */

#include "hdf5.h"

#define H5FILE_NAME "t_non_cf_char.h5"

int
main(void)
{
     hid_t file; 		/* file handle */
     hid_t space;	 	/* handles */
     hid_t attr;	 	/* handles */
     hid_t datatype;		/* handles */
     hid_t grp;			/* handles */
     hid_t dataset;		/* handles */
     hsize_t dims[1]={10}, dims2[2]={1, 2};
     herr_t status;  
 
     /*
      * Data initialization.
      */
     int    data[]  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};  
     int    data2[1][2] = {{0, 1}};  
 
     /*
      * Create a new file using H5F_ACC_TRUNC access,
      * default file creation properties, and default file
      * access properties.
      */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create dataspace.
     */
    space  = H5Screate_simple (1, dims,  NULL);

    /*
     * Test non-CFcharacters in attribute.
     */

    /*
     * Create the attribute and write the string data to it.
     */
    attr   = H5Acreate2 (file, "3~!@#%^&*()-+={}[]\\|/:;\"`\v\n\t", H5T_NATIVE_INT, space, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr,  H5T_NATIVE_INT, data);

    /*
     * Close and release resources.
     */
    status = H5Aclose (attr);
    status = H5Sclose (space);

    /*
     * Test non-CF characters in group.
     */
    grp  = H5Gcreate2(file, "/2~!@#%^&*()-+={}[]\\|:;\"`\v\n\t", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Test non-CF attributes in dataset.
     */

    space = H5Screate_simple (2, dims2, NULL);

    dataset = H5Dcreate2(file, "3~!@#%^&*()-+={}[]\\|:;\"`\v\n\t", H5T_NATIVE_INT, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    status  = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data2);

    status = H5Dclose(dataset);

    status = H5Sclose(space);
    status = H5Gclose(grp);

    status = H5Fclose (file);

    return 0;
}
