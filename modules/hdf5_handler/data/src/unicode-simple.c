/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * The copyright can be found at                                             *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.                              *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Unicode test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "hdf5.h"

#define NUM_CHARS 16
#define MAX_STRING_LENGTH ((NUM_CHARS * 4) + 1) /* Max length in bytes */
#define MAX_PATH_LENGTH (MAX_STRING_LENGTH + 20) /* Max length in bytes */
#define MAX_CODE_POINT 0x200000
#define FILENAME "unicode-simple.h5"
/* A buffer to hold two copies of the UTF-8 string */
#define LONG_BUF_SIZE (2 * MAX_STRING_LENGTH + 4)

#define DSET1_NAME "fl_string_dataset"
#define DSET3_NAME "dataset3"
#define DSET4_NAME "dataset4"
#define VL_DSET1_NAME "vl_dset_1"
#define GROUP1_NAME "group1"
#define GROUP2_NAME "group2"
#define GROUP3_NAME "group3"
#define GROUP4_NAME "group4"

#define RANK 1
#define COMP_INT_VAL 7
#define COMP_FLOAT_VAL -42.0F
#define COMP_DOUBLE_VAL 42.0F

/* Test function prototypes */
void test_fl_string(hid_t fid, const char *string);
unsigned int write_char(unsigned int c, char * test_string, unsigned int cur_pos);
/*
 * test_fl_string
 * Tests that UTF-8 can be used for fixed-length string data.
 * Writes the string to a dataset and reads it back again.
 */
void test_fl_string(hid_t fid, const char *string)
{
  hid_t dtype_id, space_id, dset_id;
  hid_t attr_id;
  hsize_t dims = 1;
  char read_buf[MAX_STRING_LENGTH];
  H5T_cset_t cset;
  herr_t ret;

  /* Create the datatype, ensure that the character set behaves
   * correctly (it should default to ASCII and can be set to UTF8)
   */
  dtype_id = H5Tcopy(H5T_C_S1);
  ret = H5Tset_size(dtype_id, (size_t)MAX_STRING_LENGTH);
  cset = H5Tget_cset(dtype_id);
  ret = H5Tset_cset(dtype_id, H5T_CSET_UTF8);
  cset = H5Tget_cset(dtype_id);

  /* Create dataspace for a dataset */
  space_id = H5Screate_simple(RANK, &dims, NULL);

  /* Create a dataset */
  dset_id = H5Dcreate2(fid, DSET1_NAME, dtype_id, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Write UTF-8 string to dataset */
  ret = H5Dwrite(dset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, string);

  /* Read string back and make sure it is unchanged */
  ret = H5Dread(dset_id, dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, read_buf);

  attr_id = H5Acreate2(dset_id, "utf8_attr", dtype_id, space_id, H5P_DEFAULT, H5P_DEFAULT);
  ret = H5Awrite(attr_id, dtype_id, string);
  H5Aclose(attr_id);


  /* Close all */
  ret = H5Dclose(dset_id);

  ret = H5Tclose(dtype_id);
  ret = H5Sclose(space_id);
}

/* write_char
 * Append a unicode code point c to test_string in UTF-8 encoding.
 * Return the new end of the string.
 */
unsigned int write_char(unsigned int c, char * test_string, unsigned int cur_pos)
{
  if (c < 0x80) {
    test_string[cur_pos] = c;
    cur_pos++;
  }
  else if (c < 0x800) {
    test_string[cur_pos] = (0xC0 | c>>6);
    test_string[cur_pos+1] = (0x80 | (c & 0x3F));
    cur_pos += 2;
  }
  else if (c < 0x10000) {
    test_string[cur_pos] = (0xE0 | c>>12);
    test_string[cur_pos+1] = (0x80 | (c>>6 & 0x3F));
    test_string[cur_pos+2] = (0x80 | (c & 0x3F));
    cur_pos += 3;
  }
  else if (c < 0x200000) {
    test_string[cur_pos] = (0xF0 | c>>18);
    test_string[cur_pos+1] = (0x80 | (c>>12 & 0x3F));
    test_string[cur_pos+2] = (0x80 | (c>>6 & 0x3F));
    test_string[cur_pos+3] = (0x80 | (c & 0x3F));
    cur_pos += 4;
  }

  return cur_pos;
}

/* Main test.
 * Create a string of random Unicode characters, then run each test with
 * that string.
 */
int main() 
{
  char test_string[MAX_STRING_LENGTH];
  unsigned int cur_pos=0;      /* Current position in test_string */
  unsigned int unicode_point;  /* Unicode code point for a single character */
  hid_t fid;                   /* ID of file */
  int x;                       /* Temporary variable */
  herr_t ret;                  /* Generic return value */


  /* Create a random string with length NUM_CHARS */
  srandom((unsigned)time(NULL));

  memset(test_string, 0, sizeof(test_string));
  for(x=0; x<NUM_CHARS; x++)
  {
    /* We need to avoid unprintable characters (codes 0-31) and the
     * . and / characters, since they aren't allowed in path names.
     */
    unicode_point = (random() % (MAX_CODE_POINT-32)) + 32;
    if(unicode_point != 46 && unicode_point != 47)
      cur_pos = write_char(unicode_point, test_string, cur_pos);
  }

  /* Avoid unlikely case of the null string */
  if(cur_pos == 0)
  {
    test_string[cur_pos] = 'Q';
    cur_pos++;
  }
  test_string[cur_pos]='\0';

  /* Create file */
  fid = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  test_fl_string(fid, test_string);

  /* Close file */
  ret = H5Fclose(fid);

  return 0;

}


