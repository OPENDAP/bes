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
#define FILENAME "unicode.h5"
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
void test_vl_string(hid_t fid, const char *string);
void test_objnames(hid_t fid, const char *string);
void test_attrname(hid_t fid, const char *string);
void test_compound(hid_t fid, const char *string);

/* Utility function prototypes */
static hid_t mkstr(size_t len, H5T_str_t strpad);
unsigned int write_char(unsigned int c, char * test_string, unsigned int cur_pos);
void dump_string(const char * string);

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


/*
 * test_vl_string
 * Tests variable-length string datatype with UTF-8 strings.
 */
void test_vl_string(hid_t fid, const char *string)
{
  hid_t type_id, space_id, dset_id;
  hsize_t dims = 1;
  hsize_t size;  /* Number of bytes used */
  char *read_buf[1];
  herr_t ret;

  /* Create dataspace for datasets */
  space_id = H5Screate_simple(RANK, &dims, NULL);

  /* Create a datatype to refer to */
  type_id = H5Tcopy(H5T_C_S1);
  ret = H5Tset_size(type_id, H5T_VARIABLE);

  /* Create a dataset */
  dset_id = H5Dcreate2(fid, VL_DSET1_NAME, type_id, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Write dataset to disk */
  ret = H5Dwrite(dset_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, &string);

  /* Make certain the correct amount of memory will be used */
  ret = H5Dvlen_get_buf_size(dset_id, type_id, space_id, &size);

  /* Read dataset from disk */
  ret = H5Dread(dset_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, read_buf);

  /* Compare data read in */

  /* Reclaim the read VL data */
  ret = H5Dvlen_reclaim(type_id, space_id, H5P_DEFAULT, read_buf);

  /* Close all */
  ret = H5Dclose(dset_id);
  ret = H5Tclose(type_id);
  ret = H5Sclose(space_id);
}

/*
 * test_objnames
 * Tests that UTF-8 can be used for object names in the file.
 * Tests groups, datasets, named datatypes, and soft links.
 * Note that this test doesn't actually mark the names as being
 * in UTF-8.  At the time this test was written, that feature
 * didn't exist in HDF5, and when the character encoding property
 * was added to links it didn't change how they were stored in the file,
 * -JML 2/2/2006
 */
void test_objnames(hid_t fid, const char* string)
{
  hid_t grp_id, grp1_id, grp2_id, grp3_id;
  hid_t type_id, dset_id, space_id;
  char read_buf[MAX_STRING_LENGTH];
  char path_buf[MAX_PATH_LENGTH];
  hsize_t dims=1;
  hobj_ref_t obj_ref;
  herr_t ret;

  /* Create a group with a UTF-8 name */
  grp_id = H5Gcreate2(fid, string, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Set a comment on the group to test that we can access the group
   * Also test that UTF-8 comments can be read.
   */
  ret = H5Oset_comment_by_name(fid, string, string, H5P_DEFAULT);
  ret = H5Oget_comment_by_name(fid, string, read_buf, (size_t)MAX_STRING_LENGTH, H5P_DEFAULT);

  ret = H5Gclose(grp_id);


  /* Create a new dataset with a UTF-8 name */
  grp1_id = H5Gcreate2(fid, GROUP1_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  space_id = H5Screate_simple(RANK, &dims, NULL);
  dset_id = H5Dcreate2(grp1_id, string, H5T_NATIVE_INT, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Make sure that dataset can be opened again */
  ret = H5Dclose(dset_id);
  ret = H5Sclose(space_id);

  dset_id = H5Dopen2(grp1_id, string, H5P_DEFAULT);
  ret = H5Dclose(dset_id);
  ret = H5Gclose(grp1_id);

  /* Do the same for a named datatype */
  grp2_id = H5Gcreate2(fid, GROUP2_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  type_id = H5Tcreate(H5T_OPAQUE, (size_t)1);
  ret = H5Tcommit2(grp2_id, string, type_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  ret = H5Tclose(type_id);

  type_id = H5Topen2(grp2_id, string, H5P_DEFAULT);
  ret = H5Tclose(type_id);

  /* Don't close the group -- use it to test that object references
   * can refer to objects named in UTF-8 */

  space_id = H5Screate_simple(RANK, &dims, NULL);
  dset_id = H5Dcreate2(grp2_id, DSET3_NAME, H5T_STD_REF_OBJ, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Create reference to named datatype */
  ret = H5Rcreate(&obj_ref, grp2_id, string, H5R_OBJECT, (hid_t)-1);
  /* Write selection and read it back*/
  ret = H5Dwrite(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, &obj_ref);
  ret = H5Dread(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, &obj_ref);

  /* Ensure that we can open named datatype using object reference */
  type_id = H5Rdereference2(dset_id, H5P_DEFAULT,H5R_OBJECT, &obj_ref);
  ret = H5Tcommitted(type_id);

  ret = H5Tclose(type_id);
  ret = H5Dclose(dset_id);
  ret = H5Sclose(space_id);

  ret = H5Gclose(grp2_id);

  /* Create "group3".  Build a hard link from group3 to group2, which has
   * a datatype with the UTF-8 name.  Create a soft link in group3
   * pointing through the hard link to the datatype.  Give the soft
   * link a name in UTF-8.  Ensure that the soft link works. */

  grp3_id = H5Gcreate2(fid, GROUP3_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  ret = H5Lcreate_hard(fid, GROUP2_NAME, grp3_id, GROUP2_NAME, H5P_DEFAULT, H5P_DEFAULT);
  strcpy(path_buf, GROUP2_NAME);
  strcat(path_buf, "/");
  strcat(path_buf, string);
  ret = H5Lcreate_hard(grp3_id, path_buf, H5L_SAME_LOC, string, H5P_DEFAULT, H5P_DEFAULT);

  /* Open named datatype using soft link */
  type_id = H5Topen2(grp3_id, string, H5P_DEFAULT);

  ret = H5Tclose(type_id);
  ret = H5Gclose(grp3_id);
}

/*
 * test_attrname
 * Test that attributes can deal with UTF-8 strings
 */
void test_attrname(hid_t fid, const char * string)
{
  hid_t group_id, attr_id;
  hid_t dtype_id, space_id;
  hsize_t dims=1;
  char read_buf[MAX_STRING_LENGTH];
  herr_t ret;

 /* Create a new group and give it an attribute whose
  * name and value are UTF-8 strings.
  */
  group_id = H5Gcreate2(fid, GROUP4_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  space_id = H5Screate_simple(RANK, &dims, NULL);
  dtype_id = H5Tcopy(H5T_C_S1);
  ret = H5Tset_size(dtype_id, (size_t)MAX_STRING_LENGTH);

  /* Create the attribute and check that its name is correct */
  attr_id = H5Acreate2(group_id, string, dtype_id, space_id, H5P_DEFAULT, H5P_DEFAULT);
  ret = H5Aget_name(attr_id, (size_t)MAX_STRING_LENGTH, read_buf);
  ret = strcmp(read_buf, string);
  read_buf[0] = '\0';

  /* Try writing and reading from the attribute */
  ret = H5Awrite(attr_id, dtype_id, string);
  ret = H5Aread(attr_id, dtype_id, read_buf);
  ret = strcmp(read_buf, string);

  /* Clean up */
  ret = H5Aclose(attr_id);
  ret = H5Tclose(dtype_id);
  ret = H5Sclose(space_id);
  ret = H5Gclose(group_id);
}

/*
 * test_compound
 * Test that compound datatypes can have UTF-8 field names.
 */
void test_compound(hid_t fid, const char * string)
{
  /* Define two compound structures, s1_t and s2_t.
   * s2_t is a subset of s1_t, with two out of three
   * fields.
   * This is stolen from the h5_compound example.
   */
  typedef struct s1_t {
      int    a;
      double c;
      float b;
  } s1_t;
  typedef struct s2_t {
      double c;
      int    a;
  } s2_t;
  /* Actual variable declarations */
  s1_t       s1;
  s2_t       s2;
  hid_t      s1_tid, s2_tid;
  hid_t      space_id, dset_id;
  hsize_t    dim = 1;
  char      *readbuf;
  herr_t     ret;

  /* Initialize compound data */
  memset(&s1, 0, sizeof(s1_t));        /* To make purify happy */
  s1.a = COMP_INT_VAL;
  s1.c = COMP_DOUBLE_VAL;
  s1.b = COMP_FLOAT_VAL;

  /* Create compound datatypes using UTF-8 field name */
  s1_tid = H5Tcreate (H5T_COMPOUND, sizeof(s1_t));
  ret = H5Tinsert(s1_tid, string, HOFFSET(s1_t, a), H5T_NATIVE_INT);

  /* Check that the field name was stored correctly */
  readbuf = H5Tget_member_name(s1_tid, 0);
  ret = strcmp(readbuf, string);
  H5free_memory(readbuf);

  /* Add the other fields to the datatype */
  ret = H5Tinsert(s1_tid, "c_name", HOFFSET(s1_t, c), H5T_NATIVE_DOUBLE);
  ret = H5Tinsert(s1_tid, "b_name", HOFFSET(s1_t, b), H5T_NATIVE_FLOAT);

  /* Create second datatype, with only two fields. */
  s2_tid = H5Tcreate (H5T_COMPOUND, sizeof(s2_t));
  ret = H5Tinsert(s2_tid, "c_name", HOFFSET(s2_t, c), H5T_NATIVE_DOUBLE);
  ret = H5Tinsert(s2_tid, string, HOFFSET(s2_t, a), H5T_NATIVE_INT);

  /* Create the dataspace and dataset. */
  space_id = H5Screate_simple(1, &dim, NULL);
  dset_id = H5Dcreate2(fid, DSET4_NAME, s1_tid, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  /* Write data to the dataset. */
  ret = H5Dwrite(dset_id, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &s1);

  /* Ensure that data can be read back by field name into s2 struct */
  ret = H5Dread(dset_id, s2_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, &s2);

  /* Clean up */
  ret = H5Tclose(s1_tid);
  ret = H5Tclose(s2_tid);
  ret = H5Sclose(space_id);
  ret = H5Dclose(dset_id);
}

/*********************/
/* Utility functions */
/*********************/

/* mkstr
 * Borrwed from dtypes.c.
 * Creates a new string data type.  Used in string padding tests */
static hid_t mkstr(size_t len, H5T_str_t strpad)
{
    hid_t       t;
    if((t = H5Tcopy(H5T_C_S1)) < 0) return -1;
    if(H5Tset_size(t, len) < 0) return -1;
    if(H5Tset_strpad(t, strpad) < 0) return -1;
    return t;
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

/* dump_string
 * Print a string both as text (which will look like garbage) and as hex.
 * The text display is not guaranteed to be accurate--certain characters
 * could confuse printf (e.g., '\n'). */
void dump_string(const char * string)
{
  unsigned int length;
  unsigned int x;

  printf("The string was:\n %s", string);
  printf("Or in hex:\n");

  length = strlen(string);

  for(x=0; x<length; x++)
    printf("%x ", string[x] & (0x000000FF));

  printf("\n");
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
  test_vl_string(fid, test_string);
  test_objnames(fid, test_string);
  test_attrname(fid, test_string);
  test_compound(fid, test_string);

  /* Close file */
  ret = H5Fclose(fid);

  return 0;

}


