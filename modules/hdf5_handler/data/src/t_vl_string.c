/*
  Test variable length string type variables and attributes as
  as scalar, 1D, and 2D variables.

  Compilation instruction:

  %/path/to/hdf5/bin/h5cc -o t_vl_string t_vl_string.c

  To generate the test file, run
  %./t_vl_string

  To view the test file, run
  %/path/to/hdf5/bin/h5dump t_vl_string.h5

  Copyright (C) 2012 The HDF Group
 */


#include "hdf5.h"
#include <stdio.h>
#include <stdlib.h>

#define FILE            "t_vl_string.h5"
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
    hid_t       file, filetype, memtype, space, space2, space3, space4, dset, dset2, dset3, dset4, attr, attr2, attr3, attr4;	/* Handles */
    herr_t      status;
    hsize_t     dims[1] = {4}, dims2[2] = {2,2}, dims3[2]={1,1};
    char        *wdata[4] = {"Parting", "is su\0", "swe\0et", ""}, 
		*wdata2[1] = {"Parting is such a sweet sorrow."},
		*wdata3[1][1] = {"A"};	/* Write buffer */

    /*
     * Create a new file using the default properties.
     */
    file = H5Fcreate (FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    /*
     * Create file and memory datatypes.  For this example we will save
     * the strings as FORTRAN strings.
     */
    filetype = H5Tcopy (H5T_FORTRAN_S1);
    status   = H5Tset_size (filetype, H5T_VARIABLE);
    if(status < 0) ERROR("Fails to set the total size for H5T_FORTRAN_S1.");
    memtype  = H5Tcopy (H5T_C_S1);
    status   = H5Tset_size (memtype, H5T_VARIABLE);
    if(status < 0) ERROR("Fails to set the total size for H5T_C_S1.");

    /*
     * Create dataspace.
     */
    space  = H5Screate_simple (1, dims, NULL);
    space2 = H5Screate(H5S_SCALAR); 
    space3 = H5Screate_simple (2, dims2, NULL);
    space4 = H5Screate_simple (2, dims3, NULL);

    /*
     * Create the dataset and write the variable-length string data to
     * it.
     */
    dset  = H5Dcreate2 (file, DATASET,  filetype, space,  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset2 = H5Dcreate2 (file, DATASET2, filetype, space2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT); 
    dset3 = H5Dcreate2 (file, DATASET3, filetype, space3, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    dset4 = H5Dcreate2 (file, DATASET4, filetype, space4, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite (dset,  memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata);
    if(status < 0) ERROR("Fails to write raw data to dataset array_1d from a buffer.");
    status = H5Dwrite (dset2, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata2);
    if(status < 0) ERROR("Fails to write raw data to dataset scalar from a buffer.");
    status = H5Dwrite (dset3, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata);
    if(status < 0) ERROR("Fails to write raw data to dataset array_2d from a buffer.");
    status = H5Dwrite (dset4, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, wdata3);
    if(status < 0) ERROR("Fails to write raw data to dataset array_special_case from a buffer.");
   
    /*
     * Create the attribute and write the variable-length string data
     * to it.
     */
    attr  = H5Acreate2 (dset,  ATTRIBUTE, filetype, space,  H5P_DEFAULT, H5P_DEFAULT);
    attr2 = H5Acreate2 (dset2, ATTRIBUTE, filetype, space2, H5P_DEFAULT, H5P_DEFAULT);
    attr3 = H5Acreate2 (dset3, ATTRIBUTE, filetype, space3, H5P_DEFAULT, H5P_DEFAULT);
    attr4 = H5Acreate2 (dset4, ATTRIBUTE, filetype, space4, H5P_DEFAULT, H5P_DEFAULT);

    status = H5Awrite (attr,  memtype, wdata);
    if(status < 0) ERROR("Fails to write data to an attribute."); 
    status = H5Awrite (attr2, memtype, wdata2);
    if(status < 0) ERROR("Fails to write data to an attribute.");
    status = H5Awrite (attr3, memtype, wdata);
    if(status < 0) ERROR("Fails to write data to an attribute.");
    status = H5Awrite (attr4, memtype, wdata3);
    if(status < 0) ERROR("Fails to write data to an attribute.");

    /*
     * Close and release resources.
     */
    status = H5Aclose (attr);
    if(status < 0) ERROR("Fails to close the specified attribute.");
    status = H5Aclose (attr2);
    if(status < 0) ERROR("Fails to close the specified attribute.");
    status = H5Aclose (attr3);
    if(status < 0) ERROR("Fails to close the specified attribute.");
    status = H5Aclose (attr4);
    if(status < 0) ERROR("Fails to close the specified attribute.");

    status = H5Dclose (dset);
    if(status < 0) ERROR("Fails to close the specified dataset.");    
    status = H5Dclose (dset2);
    if(status < 0) ERROR("Fails to close the specified dataset.");
    status = H5Dclose (dset3);
    if(status < 0) ERROR("Fails to close the specified dataset.");
    status = H5Dclose (dset4);
    if(status < 0) ERROR("Fails to close the specified dataset.");

    status = H5Sclose (space);
    if(status < 0) ERROR("Fails to release access to the specified dataspace.");     status = H5Sclose (space2);
    if(status < 0) ERROR("Fails to release access to the specified dataspace.");
    status = H5Sclose (space3);
    if(status < 0) ERROR("Fails to release access to the specified dataspace.");
    status = H5Sclose (space4);
    if(status < 0) ERROR("Fails to release access to the specified dataspace.");

    status = H5Tclose (filetype);
    if(status < 0) ERROR("Fails to release the specified datatype.");
    status = H5Tclose (memtype);
    if(status < 0) ERROR("Fails to release the specified datatype.");
    status = H5Fclose (file);
    if(status < 0) ERROR("Fails to terminate access to the specified HDF5 file."); 

    return 0;
}

