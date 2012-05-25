/*
  Test if groups are flattened properly. 

  t_flatten will create 2 groups of depth 1 (a_b_c) and depth 3 (a/b/c).

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_flatten t_flatten.c 

  To generate the test file, run
  %./t_flatten

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_flatten.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"

#define H5FILE_NAME "t_flatten.h5"

int
main (void)
{
    hid_t       file; 			       /* file handle */
    hid_t 	grp_abc, grp_a, grp_b, grp_c;  /* group handlers */
    herr_t      status;

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

    H5Gclose(grp_abc);
    H5Gclose(grp_a);
    H5Gclose(grp_b);
    H5Gclose(grp_c);
    H5Fclose(file);

    return 0;
}

