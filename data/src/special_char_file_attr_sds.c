/*
  Copyright (C) 2013 The HDF Group
  All rights reserved.

  Test SDS file attribute that has special characters.
 
  The HDF4 handler will not escape some special characters to improve the 
  readability of DAS output. Yet, those characters should not break DDX XML 
  output. 

  Compilation instruction:

  %/path/to/hdf4/bin/h4cc -o special_char_file_attr_sds special_char_file_attr_sds.c

  To generate the test file, run
  %./special_char_file_attr_sds

  To view the test file, run
  %/path/to/hdf4/bin/hdp dumpsds special_char_file_attr_sds.hdf


*/

#include "mfhdf.h"

#define FILE_NAME      "special_char_file_attr_sds.hdf"
#define FILE_ATTR_NAME "file_attr_name"

int main()
{

    int32   sd_id = FAIL;
    intn    status = FAIL;
    
    /* number of values of the file attribute */
    int32   n_values = 29;   
    /* values of the file attribute */
    char8   file_values[] = "Storm_track_data&w\n\r<e>\twinds"; 

    /*
     * Open the file and initialize the SD interface.
     */
    sd_id = SDstart (FILE_NAME, DFACC_CREATE);
    if(sd_id == FAIL){
        fprintf(stderr, "SDstart() failed.\n");
        return 1;
    }

    /*
     * Set an attribute that describes the file contents.
     */
    status = SDsetattr (sd_id, FILE_ATTR_NAME, DFNT_CHAR8, n_values, 
                        (VOIDP)file_values);
    if(status == FAIL){
        fprintf(stderr, "SDsetattr() failed.\n");
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
