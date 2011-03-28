/* makesds5.c: Creates testfile (testsds1.hdf) for HDF-EOS server
 * Contains SDS's of every data type
 * Values written include 0, 1, -1, max, and min for each datatype
 * Also creates a dimension scale of the same data type and values,
 * and global, SDS, and dimension scale attributes using all types.
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testsds5.hdf"
#define SDS_NAME   "SDS_%s"
#define AXIS_NAME  "Axis_%s"
#define GLOBAL_ATTR_NAME   "GLOBAL_ATTR_%s"
#define SDS_ATTR_NAME   "SDS_ATTR_%s"
#define DIM_ATTR_NAME   "DIM_ATTR_%s"

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
#define FLOAT32_MIN	((float)1.175494351e-38F)
#define FLOAT64_MAX	1.79769313486231570e+308
#define FLOAT64_MIN     2.2250738585072014e-308

#if 0
// Original values. I replaced these since they are outside the limits of the
// IEEE spec. Some machines recognize them; other don't. HDF4 will gladly
// copy them into a file since it uses memcpy (from Rob M.) to load values.
// However, strtod() barfs, at least on Linux and Solaris. 6/5/2001 jhrg
#define FLOAT64_MIN	4.94065645841246544e-324
#define FLOAT32_MIN	((float)1.40129846432481707e-45)
#endif


struct sdsinfo {
  int32 numtype;
  const char *typename;
  int32 numelts[1];
};

/* Note: dimension sizes chosen more or less at random. */
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
  int32 dim_id;
  int numsds = sizeof(tests) / sizeof(struct sdsinfo);  /* # of tests */
  const int32 start[1] = {0};
  VOIDP buffer;         /* test data (changes depending on type) */
  // int i, j; jhrg 3/16/11
  char sdsname[32], axisname[32], attrname[32];

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
      SDsetrange(sds_id, &buf[3], &buf[4]);
      break;
      }
    case DFNT_UINT32:
      {uint32 *buf = (uint32*)buffer = malloc(sizeof(uint32)*3);
      buf[0] = 0;	buf[1] = 1;	buf[2] = UINT32_MAX;
      SDsetrange(sds_id, &buf[2], &buf[0]);
      break;
      }
    case DFNT_INT16:
      {int16 *buf = (int16*)buffer = malloc(sizeof(int16)*5);
      buf[0] = 0;	buf[1] = 1;	buf[2] = -1;
      buf[3] = INT16_MAX;	buf[4] = INT16_MIN;
      SDsetrange(sds_id, &buf[3], &buf[4]);
      break;
      }
    case DFNT_UINT16:
      {uint16 *buf = (uint16*)buffer = malloc(sizeof(uint16)*3);
      buf[0] = 0;	buf[1] = 1;	buf[2] = UINT16_MAX;
      SDsetrange(sds_id, &buf[2], &buf[0]);
      break;
      }
    case DFNT_INT8:
      {int8 *buf = (int8*)buffer = malloc(sizeof(int8)*5);
      buf[0] = 0;	buf[1] = 1;	buf[2] = -1;
      buf[3] = INT8_MAX;	buf[4] = INT8_MIN;
      SDsetrange(sds_id, &buf[3], &buf[4]);
      break;
      }
    case DFNT_UINT8:
      {uint8 *buf = (uint8*)buffer = malloc(sizeof(uint8)*3);
      buf[0] = 0;	buf[1] = 1;	buf[2] = UINT8_MAX;
      SDsetrange(sds_id, &buf[2], &buf[0]);
      break;
      }
    case DFNT_FLOAT32:
      {float32 *buf = (float32*)buffer = malloc(sizeof(float32)*7);
      buf[0] = 0.;	buf[1] = 1.;	buf[2] = -1.;
      buf[3] = FLOAT32_MAX;	buf[4] = FLOAT32_MIN;
      buf[5] = -FLOAT32_MAX;	buf[6] = -FLOAT32_MIN;
      SDsetrange(sds_id, &buf[3], &buf[5]);
      break;
      }
    case DFNT_FLOAT64:
      {float64 *buf = (float64*)buffer = malloc(sizeof(float64)*7);
      buf[0] = 0.;	buf[1] = 1.;	buf[2] = -1.;
      buf[3] = FLOAT64_MAX;	buf[4] = FLOAT64_MIN;
      buf[5] = -FLOAT64_MAX;	buf[6] = -FLOAT64_MIN;
      SDsetrange(sds_id, &buf[3], &buf[5]);
      break;
      }
    case DFNT_CHAR8:
      {char8 *buf = (char8*)buffer = malloc(sizeof(char8)*5);
      buf[0] = 0;	buf[1] = 1;	buf[2] = -1;
      buf[3] = INT8_MAX;	buf[4] = INT8_MIN;
      SDsetrange(sds_id, &buf[3], &buf[4]);
      break;
      }
    case DFNT_UCHAR8:
      {uchar8 *buf = (uchar8*)buffer = malloc(sizeof(uchar8)*3);
      buf[0] = 0;	buf[1] = 1;	buf[2] = UINT8_MAX;
      SDsetrange(sds_id, &buf[2], &buf[0]);
      break;
      }
    }
    if(SDwritedata(sds_id, start, NULL, tests[i].numelts, buffer) == FAIL)
      return 1;

    /* Add as a dimension */
    dim_id = SDgetdimid(sds_id, 0);
    sprintf(axisname, AXIS_NAME, tests[i].typename);
    if(SDsetdimname(dim_id, axisname)==FAIL)
      return 1;
    if(SDsetdimscale(dim_id,tests[i].numelts[0], tests[i].numtype, buffer)==FAIL)
      return 1;

    {
	/* Get dim info */
	int32 count_out, number_type_out, nattrs_out;
	// char name[32]; unused jhrg 3/16/11
	if (SDdiminfo(dim_id, name, &count_out, &number_type_out, &nattrs_out) < 0)
	    return 1;
	printf("%s: count %d numtype %d nattrs %d\n", name, count_out,
		number_type_out, nattrs_out);
    }

    /* Add as a global attribute */
    sprintf(attrname, GLOBAL_ATTR_NAME, tests[i].typename);
    if(SDsetattr(sd_id, attrname, tests[i].numtype, 
		 tests[i].numelts[0], buffer) == FAIL)
      return 1;

    /* Add as an SDS attribute */
    sprintf(attrname, SDS_ATTR_NAME, tests[i].typename);
    if(SDsetattr(sds_id, attrname, tests[i].numtype,
		 tests[i].numelts[0], buffer) == FAIL)
      return 1;

    /* Add as a dimension scale attribute */
    sprintf(attrname, DIM_ATTR_NAME, tests[i].typename);
    if(SDsetattr(dim_id, attrname, tests[i].numtype,
		 tests[i].numelts[0], buffer) == FAIL)
      return 1;

    free(buffer);
    SDendaccess(sds_id);  /* end access to dataset */
  }
  SDend(sd_id);  /* end access to SD interface and close file */
  printf("Success!\n");
  return 0;
}
  
