/*
  Copyright (C) 2013 The HDF Group
  All rights reserved.

  Test group attribute that has special characters.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_special_char_attr t_special_char_attr.c

  To generate the test file, run
  %./t_special_char_attr

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_special_char_attr.h5

*/

#include "hdf5.h"

#define H5FILE_NAME "t_special_char_attr.h5"

int
main(void)
{
    hid_t file; 		/* file handle */
    hid_t space;	 	/* handle */
    hid_t attr;                 /* handle */
    hid_t atype;
    hid_t grp;			/* handle */
    herr_t status;  
    hsize_t dims[1]={1};

    /* 34 special characters */
    char string[] ="~`!@#%^&*()-_+={}[]|\\:;\"'?,<>/\v\n\r\t";

 
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
      Create string attribute.
    */
    space  = H5Screate(H5S_SCALAR);
    atype = H5Tcopy(H5T_C_S1);
    H5Tset_size(atype, 34);
    H5Tset_strpad(atype,H5T_STR_NULLTERM);
    attr = H5Acreate2(file, "special_char_attr", atype, space, 
                      H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Write string attribute.
     */
    status = H5Awrite(attr, atype, string);

    /*
     * Close and release resources.
     */
    status = H5Tclose(atype);
    status = H5Aclose(attr);
    status = H5Sclose(space);
    status = H5Fclose (file);

    return 0;
}
