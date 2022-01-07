/*
  Copyright (C) 2013 The HDF Group
  All rights reserved.

  Test SDS dataset attribute that has special characters.
 
  The HDF4 handler will not escape some special characters to improve the 
  readability of DAS output. Yet, those characters should not break DDX XML 
  output. 

  Compilation instruction:

  %/path/to/hdf4/bin/h4cc -o special_char_var_attr_sds \
  special_char_var_attr_sds.c

  To generate the test file, run
  %./special_char_var_attr_sds

  To view the test file, run
  %/path/to/hdf4/bin/hdp dumpsds special_char_var_attr_sds.hdf

*/

#include "mfhdf.h"
#define FILE_NAME "special_char_var_attr_sds.hdf"
#define SDS_ATTR_NAME "sds_attr_name"
#define SDS_NAME "sds"
#define RANK 2

int main()
{

    int32   sd_id = FAIL;
    int32   sds_id = FAIL;
    int32   dim_id = FAIL;
    int32   dim_index = FAIL;
    int32   dim_sizes[2] = {2, 2};
    int32   n_values = 29;    /* number of values of SDS attribute  */
    intn    status = FAIL;
    
    /* values of the sds attribute */
    char8   sds_values[] = "Storm_track_data&w\n\r<e>\twinds"; 

    /*
     * Open the file and initialize the SD interface.
     */
    sd_id = SDstart (FILE_NAME, DFACC_CREATE);
    if(sd_id == FAIL){
        fprintf(stderr, "SDstart() failed.\n");
        return 1;
    }

    /*
     * Create the data set with the name defined in SDS_NAME. 
     */
    sds_id = SDcreate (sd_id, SDS_NAME, DFNT_INT32, RANK, dim_sizes);
    if(sds_id == FAIL){
        fprintf(stderr, "SDcreate() failed.\n");
        return 1;
    }

    /* 
     * Assign attribute to the first SDS. Note that attribute values
     * may have different data type than SDS data.
     */
    status = SDsetattr (sds_id, SDS_ATTR_NAME, DFNT_CHAR8, n_values, 
                        (VOIDP)sds_values);
    if(status == FAIL){
        fprintf(stderr, "SDsetattr() failed.\n");
        return 1;
    }

    /*
     * Terminate access to the data set.
     */
    status = SDendaccess (sds_id);
    if(status == FAIL){
        fprintf(stderr, "SDendaccess() failed.\n");
        return 1;
    }


    /*
     * Terminate access to the SD interface and close the file.
     */
    status = SDend (sd_id);
    if(status == FAIL){
        fprintf(stderr, "SDend() failed.\n");
        return 1;
    }

    return 0;
}
