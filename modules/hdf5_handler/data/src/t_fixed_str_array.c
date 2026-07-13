/*
  Test both string type variables and attributes as scalar,
  1D, and 2D variables with H5T_STR_NULLTERM.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_string_cstr t_string_cstr.c

  To generate the test file, run
  %./t_string_cstr

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_string_cstr.h5

  Copyright (C) The HDF Group
 */


#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE            "special_fixed_str.h5"
#define DATASET		"One_chunk_unlimited"
#define DATASET2	"One_chunk_unlimited_comp"
#define DATASET3	"More_null_term_cont_str"
#define DATASET4	"More_null_term_chunk_comp_str"

#define ERROR(msg) \
{ \
fprintf(stderr, "%s at line %d\n", msg, __LINE__); \
exit(1); \
}

int
main (void)
{
    hid_t       file,datatype, space, space2, dset, dset2, dset3, dset4,plist,plist2,plist3;/* Handles */
    herr_t      status;
    hsize_t      dims[2]={3, 3};
    hsize_t      max_dims[2]= {3,H5S_UNLIMITED};
    hsize_t      chunk_dims[2] = {3,4};
    hsize_t      chunk_dims2[2] = {2,2};
    size_t      sdim;
    char        wdata[9][3] = {"abc", "ef", "g", "","hij","klm","no","p",""};
    
    /*
     * Create a new file using the default properties.
     */
    file = H5Fcreate (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create datatypes.  For this example we will save the strings 
     * as C strings.
     */
    datatype = H5Tcopy (H5T_C_S1); 
    status   = H5Tset_size (datatype, 3);
    if(status < 0) ERROR("Fails to set the total size for H5T_C_S1.");
    H5Tset_strpad(datatype,H5T_STR_NULLTERM);

    space = H5Screate_simple (2, dims, NULL);
    space2 = H5Screate_simple (2, dims, max_dims);

    plist = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(plist, 2, chunk_dims);

    plist2 = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(plist2, 2, chunk_dims);
    H5Pset_deflate(plist2,1);

    plist3 = H5Pcreate(H5P_DATASET_CREATE);
    H5Pset_chunk(plist3, 2, chunk_dims2);
    H5Pset_deflate(plist3,1);
 
    /*
     * Create the dataset and write the string data to it.
     */
    dset  = H5Dcreate2 (file, DATASET,  datatype,  space2,  H5P_DEFAULT, plist, H5P_DEFAULT);
    dset2  = H5Dcreate2 (file, DATASET2,  datatype,  space2,  H5P_DEFAULT, plist2, H5P_DEFAULT);
    dset3 = H5Dcreate2 (file, DATASET3, datatype, space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset4 = H5Dcreate2 (file, DATASET4, datatype,  space,  H5P_DEFAULT, plist3, H5P_DEFAULT);


    status = H5Dwrite (dset,  datatype,  H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata[0]);
    if(status < 0) ERROR("Fails to write raw data to dataset array_1d from a buffer.");
    status = H5Dwrite (dset2, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata[0]);
    if(status < 0) ERROR("Fails to write raw data to dataset scalar from a buffer.");
    status = H5Dwrite (dset3, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata[0]);
    if(status < 0) ERROR("Fails to write raw data to dataset array_2d from a buffer.");
    status = H5Dwrite (dset4, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata[0]);
    if(status < 0) ERROR("Fails to write raw data to dataset array_special_case from a buffer.");

    status = H5Dclose (dset);
    status = H5Dclose (dset2);
    status = H5Dclose (dset3);
    status = H5Dclose (dset4);
    status = H5Sclose (space);
    status = H5Sclose (space2);
    status = H5Tclose (datatype);

    status = H5Pclose(plist);
    status = H5Pclose(plist2);
    status = H5Pclose(plist3);

    status = H5Fclose (file);

    return 0;
}

