/* makegr1.c: Creates testfile (testgr1.hdf) for HDF-EOS server
 * Contains GR's of every data type
 * Values written include 0, 1, -1, minimum & maximum for each component
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testgr1.hdf"
#define RIG_NAME   "GR_%s"

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


struct grinfo {
  int32 numtype;
  const char *typename;
  int32 dimsizes[2];
  int32 numcmps;
};

/* Note: dimension sizes chosen more or less at random. */
const struct grinfo tests[] = {
  {DFNT_INT32, "DFNT_INT32", {5, 5}, 1},
  {DFNT_UINT32, "DFNT_UINT32", {3, 3}, 3},
  {DFNT_INT16, "DFNT_INT16", {5, 5}, 2},
  {DFNT_UINT16, "DFNT_UINT16", {3, 3}, 3},
  {DFNT_INT8, "DFNT_INT8", {5, 5}, 2},
  {DFNT_UINT8, "DFNT_UINT8", {3, 3}, 3},
  {DFNT_FLOAT32, "DFNT_FLOAT32", {7, 4}, 3},
  {DFNT_FLOAT64, "DFNT_FLOAT64", {7, 4}, 1},
  {DFNT_CHAR8, "DFNT_CHAR8", {5, 5}, 3},
  {DFNT_UCHAR8, "DFNT_UCHAR8", {3, 5}, 3}
};

main() {
  int32 file_id, gr_id, ri_id;  /* SD interface and data set ids */
  int numgr = sizeof(tests) / sizeof(struct grinfo);  /* # of tests */
  const int32 start[2] = {0, 0};
  VOIDP buffer;         /* test data (changes depending on type) */
  int i, j;
  char grname[32];

  if((file_id = Hopen(FILE_NAME, DFACC_CREATE, 0)) == FAIL)
    return 1;
  printf("Created HDF\n");

  gr_id = GRstart(file_id);  /* Initialize the GR interface. */

  for(i=0; i<numgr; i++) {
    printf("Adding GR: %s\n", tests[i].typename);
    sprintf(grname, RIG_NAME, tests[i].typename);
    ri_id = GRcreate(gr_id, grname, tests[i].numcmps, 
		    tests[i].numtype, MFGR_INTERLACE_PIXEL, tests[i].dimsizes);
    if(ri_id == FAIL)
      return 1;

    switch(tests[i].numtype) {
    case DFNT_INT32:
      {int32 *buf = (int32*)buffer = malloc(sizeof(int32)*25);
      for(j=0; j<5; j++) {
	buf[j*5+0] = 0;	buf[j*5+1] = 1;	buf[j*5+2] = -1;
	buf[j*5+3] = INT32_MAX;	buf[j*5+4] = INT32_MIN;
      }
      break;
      }
    case DFNT_UINT32:
      {uint32 *buf = (uint32*)buffer = malloc(sizeof(uint32)*27);
      for(j=0; j<9; j++) {
	buf[j*3+0] = 0;	buf[j*3+1] = 1;	buf[j*3+2] = UINT32_MAX;
      }
      break;
      }
    case DFNT_INT16:
      {int16 *buf = (int16*)buffer = malloc(sizeof(int16)*50);
      for(j=0; j<10; j++) {
	buf[j*5+0] = 0;	buf[j*5+1] = 1;	buf[j*5+2] = -1;
	buf[j*5+3] = INT16_MAX;	buf[j*5+4] = INT16_MIN;
      }
      break;
      }
    case DFNT_UINT16:
      {uint16 *buf = (uint16*)buffer = malloc(sizeof(uint16)*27);
      for(j=0; j<9; j++) {
	buf[j*3+0] = 0;	buf[j*3+1] = 1;	buf[j*3+2] = UINT16_MAX;
      }
      break;
      }
    case DFNT_INT8:
      {int8 *buf = (int8*)buffer = malloc(sizeof(int8)*50);
      for(j=0; j<10; j++) {
	buf[j*5+0] = 0;	buf[j*5+1] = 1;	buf[j*5+2] = -1;
	buf[j*5+3] = INT8_MAX;	buf[j*5+4] = INT8_MIN;
      }
      break;
      }
    case DFNT_UINT8:
      {uint8 *buf = (uint8*)buffer = malloc(sizeof(uint8)*27);
      for(j=0; j<9; j++) {
	buf[j*3+0] = 0;	buf[j*3+1] = 1;	buf[j*3+2] = UINT8_MAX;
      }
      break;
      }
    case DFNT_FLOAT32:
      {float32 *buf = (float32*)buffer = malloc(sizeof(float32)*84);
      for(j=0; j<12; j++) {
	buf[j*7+0] = 0.;	buf[j*7+1] = 1.;	buf[j*7+2] = -1.;
	buf[j*7+3] = FLOAT32_MAX;	buf[j*7+4] = FLOAT32_MIN;
	buf[j*7+5] = -FLOAT32_MAX;	buf[j*7+6] = -FLOAT32_MIN;
      }
      break;
      }
    case DFNT_FLOAT64:
      {float64 *buf = (float64*)buffer = malloc(sizeof(float64)*28);
      for(j=0; j<4; j++) {
	buf[j*7+0] = 0.;	buf[j*7+1] = 1.;	buf[j*7+2] = -1.;
	buf[j*7+3] = FLOAT64_MAX;	buf[j*7+4] = FLOAT64_MIN;
	buf[j*7+5] = -FLOAT64_MAX;	buf[j*7+6] = -FLOAT64_MIN;
      }
      break;
      }
    case DFNT_CHAR8:
      {char8 *buf = (char8*)buffer = malloc(sizeof(char8)*75);
      for(j=0; j<15; j++) {
	buf[j*5+0] = 0;	buf[j*5+1] = 1;	buf[j*5+2] = -1;
	buf[j*5+3] = INT8_MAX;	buf[j*5+4] = INT8_MIN;
      }
      break;
      }
    case DFNT_UCHAR8:
      {uchar8 *buf = (uchar8*)buffer = malloc(sizeof(uchar8)*45);
      for(j=0; j<15; j++) {
	buf[j*3+0] = 0;	buf[j*3+1] = 1;	buf[j*3+2] = UINT8_MAX;
      }
      break;
      }
    }
    if(GRwriteimage(ri_id, start, NULL, tests[i].dimsizes, buffer) == FAIL)
      return 1;
    free(buffer);
    GRendaccess(ri_id);  /* end access to dataset */
  }
  GRend(gr_id);  /* end access to GR interface */
  Hclose(file_id);  /* close file */
  printf("Success!\n");
  return 0;
}
  
