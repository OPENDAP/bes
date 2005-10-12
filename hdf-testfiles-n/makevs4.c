/* makevs4.c: Creates testfile (testvs4.hdf) for HDF-EOS server
 * Creates one field multi-component Vdata's for each data type
 * Values written include 0, 1, -1, max, and min for each datatype
 * Also writes an attribute for each Vdata and for each field with the
 *  same values as the Vdata itself.
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testvs4.hdf"
#define VDATA_NAME "Vdata_%s"
#define VDATA_CLASS "Class_%s"
#define FIELD_NAME "Field_%s"
#define ATTR_NAME  "Attr_%s"
#define FATTR_NAME "FieldAttr_%s"

#define INT32_MAX       2147483647
#define INT32_MIN       (-2147483647-1)
#define UINT32_MAX      4294967295U
#define INT16_MAX       32767
#define INT16_MIN       (-32768)
#define UINT16_MAX      65535
#define INT8_MAX        127
#define INT8_MIN        (-128)
#define UINT8_MAX       255
// See makesds5.c for an explaination of these values. 6/5/2001 jhrg
#define FLOAT32_MAX	((float)3.40282346638528860e+38)
#define FLOAT32_MIN	((float)1.175494351e-38F)
#define FLOAT64_MAX	1.79769313486231570e+308
#define FLOAT64_MIN     2.2250738585072014e-308


struct vdinfo {
  int32 numtype;
  const char *typename;
  int32 numelts;
};

const struct vdinfo tests[] = {
  {DFNT_INT32, "DFNT_INT32", 5},
  {DFNT_UINT32, "DFNT_UINT32", 3},
  {DFNT_INT16, "DFNT_INT16", 5},
  {DFNT_UINT16, "DFNT_UINT16", 3},
  {DFNT_INT8, "DFNT_INT8", 5},
  {DFNT_UINT8, "DFNT_UINT8", 3},
  {DFNT_FLOAT32, "DFNT_FLOAT32", 7},
  {DFNT_FLOAT64, "DFNT_FLOAT64", 7},
  {DFNT_CHAR8, "DFNT_CHAR8", 5},
  {DFNT_UCHAR8, "DFNT_UCHAR8", 3}
};


main() {
  int32 file_id, vdata_ref, vdata_id;  /* ID of file and Vdata */
  int numvd = sizeof(tests) / sizeof(struct vdinfo);
  VOIDP buffer;  /* test data */
  int i;
  char vdname[32], vdclass[32], vdfield[32], vattrname[32], vfieldattr[32];
  
  if((file_id = Hopen(FILE_NAME, DFACC_CREATE, 0))==FAIL)
    return 1;

  if(Vstart(file_id)==FAIL)
    return 1;

  for(i=0; i<numvd; i++) {
    printf("Adding Vdata: %s\n", tests[i].typename);
    sprintf(vdname, VDATA_NAME, tests[i].typename);
    sprintf(vdclass, VDATA_CLASS, tests[i].typename);
    sprintf(vdfield, FIELD_NAME, tests[i].typename);
    sprintf(vattrname, ATTR_NAME, tests[i].typename);
    sprintf(vfieldattr, FATTR_NAME, tests[i].typename);

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
    if((vdata_ref = VHstoredata(file_id, vdfield, (uint8 *)buffer,
		tests[i].numelts, tests[i].numtype, vdname, vdclass))==FAIL)
      return 2;

    if((vdata_id = VSattach(file_id, vdata_ref, "w"))==FAIL) return 1;
    if(VSsetattr(vdata_id, _HDF_VDATA, vattrname, tests[i].numtype,
		 tests[i].numelts, buffer)==FAIL) return 1;
    if(VSsetattr(vdata_id, 0, vfieldattr, tests[i].numtype,
		 tests[i].numelts, buffer)==FAIL) return 1;
    if(VSdetach(vdata_id)==FAIL) return 1;
  }
  if(Vend(file_id)==FAIL)
    return 1;
  if(Hclose(file_id)==FAIL)
    return 1;
  printf("Success!\n");
  return 0;
}
  
