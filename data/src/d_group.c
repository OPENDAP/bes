/*
  Test groups for HDF5 handler with default option. 

  Unlike CF option, groups should not be flattened.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o d_group d_group.c 

  To generate the test file, run
  %./d_group

  To view the test file, run
  %/path/to/hdf5/bin/h5dump d_group.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"

#define H5FILE_NAME "d_group.h5"

int
main (void)
{
    hid_t       file;           /* file handle */
    hid_t 	grp_a, grp_b, grp_c; /* group handlers */

    /*
     * Create a new file using H5F_ACC_TRUNC access,
     * default file creation properties, and default file
     * access properties.
     */
    file = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create groups in the file.
     */
    grp_a = H5Gcreate2(file,  "/a",     H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    grp_b = H5Gcreate2(grp_a, "b", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    grp_c = H5Gcreate2(grp_b, "c", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Gclose(grp_a);
    H5Gclose(grp_b);
    H5Gclose(grp_c);
    H5Fclose(file);

    return 0;
}

