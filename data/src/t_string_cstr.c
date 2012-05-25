/*
  Test both string type variables and attributes as scalar,
  1D, and 2D variables with H5T_STR_NULLPAD.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_string_cstr t_string_cstr.c

  To generate the test file, run
  %./t_string_cstr

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_string_cstr.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE            "t_string_cstr.h5"
#define DATASET         "array_1d"
#define DATASET2	"scalar"
#define DATASET3	"array_2d"
#define DATASET4	"array_special_case"
#define ATTRIBUTE       "value"

#define ERROR(msg) \
{ \
fprintf(stderr, "%s at line %d\n", msg, __LINE__); \
exit(1); \
}

int
main (void)
{
    hid_t       file, datatype, datatype2, datatype3, space, space2, space3, space4, dset, dset2, dset3, dset4, attr, attr2, attr3, attr4;/* Handles */
    herr_t      status;
    hsize_t     dims[1] = {4}, dims2[2]={2, 2}, dims3[2]={1, 1};
    size_t      sdim;
    char        wdata[4][7] = {"Parting", "is so", "swe\0et", ""}, 
		*wdata2 = "Parting is such sweet sorrow.",
		wdata3[1][1] = {"A"};	/* Write buffer */
    
    /*
     * Create a new file using the default properties.
     */
    file = H5Fcreate (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create datatypes.  For this example we will save the strings 
     * as C strings.
     */
    datatype = H5Tcopy (H5T_C_S1); 
    status   = H5Tset_size (datatype, 7);
    if(status < 0) ERROR("Fails to set the total size for H5T_C_S1.");

    datatype2 = H5Tcopy(H5T_C_S1); 
    status    = H5Tset_size (datatype2, strlen(wdata2));
    if(status < 0) ERROR("Fails to set the total size for H5T_C_S1.");

    datatype3 =  H5Tcopy (H5T_C_S1);
    status    = H5Tset_size (datatype3, 1);
    if(status < 0) ERROR("Fails to set the total size for H5T_C_S1.");

    /*
     * Create dataspace.
     */
    space  = H5Screate_simple (1, dims,  NULL);
    space2 = H5Screate (H5S_SCALAR);
    space3 = H5Screate_simple (2, dims2, NULL);
    space4 = H5Screate_simple (2, dims3, NULL);

    /*
     * Create the dataset and write the string data to it.
     */
    dset  = H5Dcreate2 (file, DATASET,  datatype,  space,  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset2 = H5Dcreate2 (file, DATASET2, datatype2, space2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset3 = H5Dcreate2 (file, DATASET3, datatype,  space3,  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset4 = H5Dcreate2 (file, DATASET4, datatype3,  space4,  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 


    status = H5Dwrite (dset,  datatype,  H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata[0]);
    if(status < 0) ERROR("Fails to write raw data to dataset array_1d from a buffer.");
    status = H5Dwrite (dset2, datatype2, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata2);
    if(status < 0) ERROR("Fails to write raw data to dataset scalar from a buffer.");
    status = H5Dwrite (dset3, datatype,  H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata[0]);
    if(status < 0) ERROR("Fails to write raw data to dataset array_2d from a buffer.");
    status = H5Dwrite (dset4, datatype3,  H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata3);
    if(status < 0) ERROR("Fails to write raw data to dataset array_special_case from a buffer.");

    /*
     * Create the attribute and write the string data to it.
     */
    attr   = H5Acreate2 (dset, ATTRIBUTE, datatype, space, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr, datatype, wdata[0]);
    if(status < 0) ERROR("Fails to write data to an attribute.");

    attr2  = H5Acreate2 (dset2, ATTRIBUTE, datatype2, space2, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr2, datatype2, wdata2);
    if(status < 0) ERROR("Fails to write data to an attribute.");

    attr3 =  H5Acreate2 (dset3, ATTRIBUTE, datatype, space3, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr3, datatype, wdata[0]);
    if(status < 0) ERROR("Fails to write data to an attribute.");

    attr4 =  H5Acreate2 (dset4, ATTRIBUTE, datatype3, space4, H5P_DEFAULT, H5P_DEFAULT);
    status = H5Awrite (attr4, datatype3, wdata3[0]);
    if(status < 0) ERROR("Fails to write data to an attribute.");
   

    /*
     * Close and release resources.
     */
    status = H5Aclose (attr);
    status = H5Aclose (attr2);
    status = H5Aclose (attr3);
    status = H5Aclose (attr4);
    status = H5Dclose (dset);
    status = H5Dclose (dset2);
    status = H5Dclose (dset3);
    status = H5Dclose (dset4);
    status = H5Sclose (space);
    status = H5Sclose (space2);
    status = H5Sclose (space3);
    status = H5Sclose (space4);
    status = H5Tclose (datatype);
    status = H5Tclose (datatype2);
    status = H5Tclose (datatype3);
    status = H5Fclose (file);

    return 0;
}

