/*
  Test soft/hard link for default handler.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o d_link_hard d_link_hard.c

  To generate the test file, run
  %./d_link_hard

  To view the test file, run
  %/path/to/hdf5/bin/h5dump d_link_hard.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"

#define FILE "d_link_hard.h5"

/* 
 * hard link example
 */
static void write_hard_link(hid_t fid)
{
    hid_t g2, g3;

    g2 = H5Gopen2(fid, "/g2", H5P_DEFAULT);
    H5Lcreate_hard(g2, "/dset1", H5L_SAME_LOC, "dset1_hard", 
                   H5P_DEFAULT, H5P_DEFAULT);
    H5Lcreate_hard(g2, "/g4", H5L_SAME_LOC, "g4_hard", 
                   H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(g2);

    g3 = H5Gopen2(fid, "/g1/g3", H5P_DEFAULT);
    H5Lcreate_hard(g3, "/dset1", H5L_SAME_LOC, "dset1_hard", 
                   H5P_DEFAULT, H5P_DEFAULT);
    H5Lcreate_hard(g3, "/g4", H5L_SAME_LOC, "g4_hard", 
                   H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(g3);
}

/*
 * groups and datasets.
 */

static void write_groups_and_datasets(hid_t fid)
{
    hid_t g1, g2, g3, g4, dataset, space;
    hsize_t dim = 5;
    int i, dset[5];

    /* Create a data set and fill in integer. */
    space = H5Screate_simple(1, &dim, NULL);
    dataset = H5Dcreate2(fid, "/dset1", H5T_STD_I32LE, space, 
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    for(i = 0; i < 5; i++) 
        dset[i] = i;

    H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset);
    H5Sclose(space);
    H5Dclose(dataset);


    /* Create gorup g2 under root. */
    g2 = H5Gcreate2(fid, "/g2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(g2);

    /* Create gorup g4 under root. */
    g4 = H5Gcreate2(fid, "/g4", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(g4);

    /* Create gorup g1 under root. */
    g1 = H5Gcreate2(fid, "/g1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /* Create group g3 under g1. */
    g3 = H5Gcreate2(g1, "g3", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(g3);
    H5Gclose(g1);

}

int
main(void)
{
    hid_t fid;

    fid = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    write_groups_and_datasets(fid);

    write_hard_link(fid);

    H5Fclose(fid);

    return 0;
}

