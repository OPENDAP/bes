/*
  Test soft/hard link and comment together.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_link_comment t_link_comment.c

  To generate the test file, run
  %./t_link_comment

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_link_comment.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"

#define FILE "t_link_comment.h5"

/* 
 * soft link example
 */
static void write_soft_link(hid_t fid)
{
    hid_t root;
    hid_t g2;

    root = H5Gopen2(fid, "/", H5P_DEFAULT);
    H5Lcreate_soft("/dest1", root, "slink1", H5P_DEFAULT, H5P_DEFAULT);
    H5Lcreate_soft("/g1", root, "slink2", H5P_DEFAULT, H5P_DEFAULT);

    g2 = H5Gopen2(fid, "/g2", H5P_DEFAULT);
    H5Lcreate_soft("/dset1", g2, "slink3", H5P_DEFAULT, H5P_DEFAULT);
    H5Lcreate_soft("/g1", g2, "slink4", H5P_DEFAULT, H5P_DEFAULT); 
    H5Gclose(g2);

    H5Gclose(root);
}

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
 *  comment example
 */
static void write_comment(hid_t fid)
{
    hid_t g1, g2, g3, g4, dataset, space;
    hsize_t dim = 5;
    int i, dset[5];

    space = H5Screate_simple(1, &dim, NULL);
    dataset = H5Dcreate2(fid, "/dset1", H5T_STD_I32LE, space, 
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    for(i = 0; i < 5; i++) 
        dset[i] = i;

    H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset);
    H5Oset_comment(dataset, "This is a dataset.");
    H5Sclose(space);
    H5Dclose(dataset);

    g1 = H5Gcreate2(fid, "/g1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Lcreate_hard(g1, "/dset1", H5L_SAME_LOC, "dset2", H5P_DEFAULT, H5P_DEFAULT);
    H5Oset_comment(g1, "This is a group.");

    /* Create group g3 under g1. */
    g3 = H5Gcreate2(g1, "g3", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(g3);
    H5Gclose(g1);


    /* Create gorup g2 under root. */
    g2 = H5Gcreate2(fid, "/g2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(g2);

    /* Create gorup g4 under root. */
    g4 = H5Gcreate2(fid, "/g4", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(g4);


}

int
main(void)
{
    hid_t fid;

    fid = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    write_comment(fid);
    write_soft_link(fid);
    write_hard_link(fid);

    H5Fclose(fid);

    return 0;
}

