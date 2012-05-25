/*
  Test soft/hard link for default handler.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o d_links d_links.c

  To generate the test file, run
  %./d_links

  To view the test file, run
  %/path/to/hdf5/bin/h5dump d_links.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"

#define FILE "d_links.h5"

/* 
 * soft link example
 */
static void write_soft_link(hid_t fid)
{
    hid_t root;

    root = H5Gopen2(fid, "/", H5P_DEFAULT);
    H5Lcreate_soft("/dset1", root, "slink1", H5P_DEFAULT, H5P_DEFAULT);
    H5Lcreate_soft("/g1", root, "slink2", H5P_DEFAULT, H5P_DEFAULT);

    H5Gclose(root);
}

/*
 * hard link
 */
static void write_hard_link(hid_t fid)
{
    hid_t group, dataset, space;
    hsize_t dim = 5;
    int i, dset[5];

    space = H5Screate_simple(1, &dim, NULL);
    dataset = H5Dcreate2(fid, "/dset1", H5T_STD_I32LE, space, 
                         H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    for(i = 0; i < 5; i++) 
        dset[i] = i;

    H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, dset);
    H5Sclose(space);
    H5Dclose(dataset);

    group = H5Gcreate2(fid, "/g1", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Lcreate_hard(group, "/dset1", H5L_SAME_LOC, "dset2", 
                   H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(group);

    group = H5Gcreate2(fid, "/g2", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5Lcreate_hard(group, "/dset1", H5L_SAME_LOC, "dset3", 
                   H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(group);

    group = H5Gopen2(fid, "/g1", H5P_DEFAULT);
    H5Lcreate_hard(group, "/g2", H5L_SAME_LOC, "g1_1", 
                   H5P_DEFAULT, H5P_DEFAULT);
    H5Gclose(group);

    /* Create a link to the root group. */
    H5Lcreate_hard(fid, "/", H5L_SAME_LOC, "g3", H5P_DEFAULT, H5P_DEFAULT);
}

int
main(void)
{
    hid_t fid;

    fid = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    write_soft_link(fid);

    write_hard_link(fid);

    H5Fclose(fid);

    return 0;
}

