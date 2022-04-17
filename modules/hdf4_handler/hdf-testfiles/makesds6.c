/* makesds6.c: Creates testfile (testsds6.hdf) for HDF-EOS server
 * Writes 2-dimensional SDS's compressed with RLE, N-bit, Huffman, and Deflate
 * as well as non-standard bit length (N-bit) data.
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testsds6.hdf"
#define SDS_NAME   "SDS_%s"
#define DIM_NAME_X "X_Axis"
#define DIM_NAME_Y "Y_Axis"
#define NAME_LENGTH 6
#define X_LENGTH   5
#define Y_LENGTH   16
#define RANK       2

struct sdsinfo {
  comp_coder_t type;
  const char *typename;
};

const struct sdsinfo tests[] = {
  {COMP_CODE_RLE, "RLE"},
  {COMP_CODE_NBIT, "N-bit"},
  {COMP_CODE_SKPHUFF, "Skip_Huffman"},
  {COMP_CODE_DEFLATE, "Deflate"}
};

main() {
  int32 sd_id, sds_id;  /* SD interface and data set ids */
  int numsds = sizeof(tests) / sizeof(struct sdsinfo);  /* # of tests */
  const int32 start[2] = {0, 0};
  const int32 dim_sizes[2] = {Y_LENGTH, X_LENGTH};
  int32 data[Y_LENGTH][X_LENGTH];
  char sdsname[32];
  int i, j;
  comp_info c_info;

  /* Initialize data */
  for(i=0; i<Y_LENGTH; i++)
    for(j=0; j<X_LENGTH; j++)
      data[i][j] = i*3 + j;

  if((sd_id = SDstart (FILE_NAME, DFACC_CREATE)) == FAIL)
    return 1;
  printf("Created HDF\n");

  for(i=0; i<numsds; i++) {
    printf("Adding SDS: %s\n", tests[i].typename);
    sprintf(sdsname, SDS_NAME, tests[i].typename);
    sds_id = SDcreate(sd_id, sdsname, DFNT_INT32, 2, dim_sizes);
    if(sds_id == FAIL)
      return 1;
    
    switch(tests[i].type) {
    case COMP_CODE_RLE:
      if(SDsetcompress(sds_id, tests[i].type, &c_info)==FAIL)
	return 2;
      break;
    case COMP_CODE_NBIT:
      if(SDsetnbitdataset(sds_id, 5, 6, 0, 0)==FAIL)
	return 2;
      break;
    case COMP_CODE_SKPHUFF:
      c_info.skphuff.skp_size = sizeof(int32);
      if(SDsetcompress(sds_id, tests[i].type, &c_info)==FAIL)
	return 1;
      break;
    case COMP_CODE_DEFLATE:
      c_info.deflate.level = 9;
      if(SDsetcompress(sds_id, tests[i].type, &c_info)==FAIL)
	return 1;
      break;
    }
    
    if(SDwritedata(sds_id, start, NULL, dim_sizes, data) == FAIL)
      return 3;

    SDendaccess(sds_id);  /* end access to dataset */
  }
  SDend(sd_id);  /* end access to SD interface and close file */
  printf("Success!\n");
  return 0;
}
  
