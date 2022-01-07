/* makesds1.c: Creates testfile (testsds1.hdf) for HDF-EOS server
 * Contains SDS's of every data type
 * Values written include 0, 1, -1, max, and min for each datatype
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testsds1.hdf"
#define SDS_NAME   "SDS_%s"

#define INT32_MAX	2147483647
#define INT32_MIN	(-2147483647-1)
#define UINT32_MAX	4294967295U
#define INT16_MAX	32767
#define INT16_MIN	(-32768)
#define UINT16_MAX	65535
#define INT8_MAX	127
#define INT8_MIN	(-128)
#define UINT8_MAX	255
#define FLOAT32_MAX	((float)3.40282346638528860e+38)
#define FLOAT32_MIN	((float)1.40129846432481707e-45)
#define FLOAT64_MAX	1.79769313486231570e+308
#define FLOAT64_MIN	4.94065645841246544e-324


struct sdsinfo {
  int32 numtype;
  const char *typename;
  int32 numelts[1];
};

const struct sdsinfo tests[] = {
  {DFNT_INT32, "DFNT_INT32", {5}},
  {DFNT_UINT32, "DFNT_UINT32", {3}},
  {DFNT_INT16, "DFNT_INT16", {5}},
  {DFNT_UINT16, "DFNT_UINT16", {3}},
  {DFNT_INT8, "DFNT_INT8", {5}},
  {DFNT_UINT8, "DFNT_UINT8", {3}},
  {DFNT_FLOAT32, "DFNT_FLOAT32", {7}},
  {DFNT_FLOAT64, "DFNT_FLOAT64", {7}},
  {DFNT_CHAR8, "DFNT_CHAR8", {5}},
  {DFNT_UCHAR8, "DFNT_UCHAR8", {3}},
};

main() {
  int32 sd_id, sds_id;  /* SD interface and data set ids */
  int numsds = sizeof(tests) / sizeof(struct sdsinfo);  /* # of tests */
  const int32 start[1] = {0};
  VOIDP buffer;         /* test data (changes depending on type) */
  // int i, j; j unused jhrg 3/16/11
  char sdsname[32];

  if((sd_id = SDstart (FILE_NAME, DFACC_CREATE)) == FAIL)
    return 1;
  printf("Created HDF\n");

  for(int i=0; i<numsds; i++) {
    printf("Adding SDS: %s\n", tests[i].typename);
    sprintf(sdsname, SDS_NAME, tests[i].typename);
    sds_id = SDcreate(sd_id, sdsname, tests[i].numtype, 1, tests[i].numelts);
    if(sds_id == FAIL)
      return 1;

    switch(tests[i].numtype) {
    case DFNT_INT32:
      {int32 *buf = (int32*)buffer = malloc(sizeof(int32)*5);
      buf[0] = 0;	buf[1] = 1;	buf[2] = -1;
      buf[3] = INT32_MAX;	buf[4] = INT32_MIN;
      break;
      }
    case DFNT_UINT32:
      {uint32 *buf = (uint32*)buffer = malloc(sizeof(uint32)*3);
      buf[0] = 0;	buf[1] = 1;	buf[2] = UINT32_MAX;
      break;
      }
    case DFNT_INT16:
      {int16 *buf = (int16*)buffer = malloc(sizeof(int16)*5);
      buf[0] = 0;	buf[1] = 1;	buf[2] = -1;
      buf[3] = INT16_MAX;	buf[4] = INT16_MIN;
      break;
      }
    case DFNT_UINT16:
      {uint16 *buf = (uint16*)buffer = malloc(sizeof(uint16)*3);
      buf[0] = 0;	buf[1] = 1;	buf[2] = UINT16_MAX;
      break;
      }
    case DFNT_INT8:
      {int8 *buf = (int8*)buffer = malloc(sizeof(int8)*5);
      buf[0] = 0;	buf[1] = 1;	buf[2] = -1;
      buf[3] = INT8_MAX;	buf[4] = INT8_MIN;
      break;
      }
    case DFNT_UINT8:
      {uint8 *buf = (uint8*)buffer = malloc(sizeof(uint8)*3);
      buf[0] = 0;	buf[1] = 1;	buf[2] = UINT8_MAX;
      break;
      }
    case DFNT_FLOAT32:
      {float32 *buf = (float32*)buffer = malloc(sizeof(float32)*7);
      buf[0] = 0.;	buf[1] = 1.;	buf[2] = -1.;
      buf[3] = FLOAT32_MAX;	buf[4] = FLOAT32_MIN;
      buf[5] = -FLOAT32_MAX;	buf[6] = -FLOAT32_MIN;
      break;
      }
    case DFNT_FLOAT64:
      {float64 *buf = (float64*)buffer = malloc(sizeof(float64)*7);
      buf[0] = 0.;	buf[1] = 1.;	buf[2] = -1.;
      buf[3] = FLOAT64_MAX;	buf[4] = FLOAT64_MIN;
      buf[5] = -FLOAT64_MAX;	buf[6] = -FLOAT64_MIN;
      break;
      }
    case DFNT_CHAR8:
      {char8 *buf = (char8*)buffer = malloc(sizeof(char8)*5);
      buf[0] = 0;	buf[1] = 1;	buf[2] = -1;
      buf[3] = INT8_MAX;	buf[4] = INT8_MIN;
      break;
      }
    case DFNT_UCHAR8:
      {uchar8 *buf = (uchar8*)buffer = malloc(sizeof(uchar8)*3);
      buf[0] = 0;	buf[1] = 1;	buf[2] = UINT8_MAX;
      break;
      }
    }
    if(SDwritedata(sds_id, start, NULL, tests[i].numelts, buffer) == FAIL)
      return 1;
    free(buffer);
    SDendaccess(sds_id);  /* end access to dataset */
  }
  SDend(sd_id);  /* end access to SD interface and close file */
  printf("Success!\n");
  return 0;
}
  
