/* makevs3.c: Creates testfile (testvs3.hdf) for HDF-EOS server
 * Creates a Vdata with one field for each data type
 * Values written include 0, 1, -1, max, and min for each datatype
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testvs3.hdf"
#define VDATA_NAME "Vdata"
#define CLASS_NAME "Class"

#define ORDER      1     /* number of values in the field */
#define N_RECORDS  7     /* number of records in the vdata */
#define N_FIELDS  10     /* number of fields in the vdata */
#define FIELDNAME_LIST  "dINT32,dUINT32,dINT16,dUINT16,dINT8,dUINT8,dFLOAT32,dFLOAT64,dCHAR8,dUCHAR8"

/* number of bytes to be written */
#define BUF_SIZE (sizeof(int32)+sizeof(uint32)+sizeof(int16)+sizeof(uint16)+sizeof(int8)+sizeof(uint8)+sizeof(float32)+sizeof(float64)+sizeof(char8)+sizeof(uchar8)) * N_RECORDS

#define INT32_MAX       2147483647
#define INT32_MIN       (-2147483647-1)
#define UINT32_MAX      4294967295U
#define INT16_MAX       32767
#define INT16_MIN       (-32768)
#define UINT16_MAX      65535
#define INT8_MAX        127
#define INT8_MIN        (-128)
#define UINT8_MAX       255
#define FLOAT32_MAX     ((float)3.40282346638528860e+38)
#define FLOAT32_MIN     ((float)1.40129846432481707e-45)
#define FLOAT64_MAX     1.79769313486231570e+308
#define FLOAT64_MIN     4.94065645841246544e-324

main() {
  int32 file_id, vdata_id;  /* ID of file and Vdata */

  int32 field1[] = { 0, 1, -1, INT32_MAX, INT32_MIN, 4, 2 };
  uint32 field2[] = { 0, 1, UINT32_MAX, 1, 5, 4, 1 };
  int16 field3[] = {0, 1, -1, INT16_MAX, INT16_MIN, 4, 2 };
  uint16 field4[] = {0, 1, UINT16_MAX, 1, 5, 4, 1 };
  int8 field5[] = {0, 1, -1, INT8_MAX, INT8_MIN, 4, 2 };
  uint8 field6[] = {0, 1, UINT8_MAX, 1, 5, 4, 1 };
  float32 field7[] = {0., 1., -1., FLOAT32_MAX, FLOAT32_MIN,
		      -FLOAT32_MAX, -FLOAT32_MIN};
  float64 field8[] = {0., 1., -1., FLOAT64_MAX, FLOAT64_MIN,
		      -FLOAT64_MAX, -FLOAT64_MIN};
  char8 field9[]= {0, 1, -1, INT8_MAX, INT8_MIN, 4, 2};
  uchar8 field10[] = {0, 1, UINT8_MAX, 1, 5, 4, 1};
  VOIDP  fldbufptrs[N_FIELDS];  /* pointers to each field buffer */
  uint8 databuf[BUF_SIZE];     /* pointer to hold the data after packing */
  
  if((file_id = Hopen(FILE_NAME, DFACC_CREATE, 0))==FAIL)
    return 1;

  if(Vstart(file_id)==FAIL)
    return 1;

  if((vdata_id = VSattach(file_id, -1, "w"))==FAIL)
    return 1;

  if(VSsetname(vdata_id, VDATA_NAME)==FAIL)   return 1;
  if(VSsetclass(vdata_id, CLASS_NAME)==FAIL)  return 1;

  if(VSfdefine(vdata_id, "dINT32", DFNT_INT32, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dUINT32", DFNT_UINT32, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dINT16", DFNT_INT16, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dUINT16", DFNT_UINT16, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dINT8", DFNT_INT8, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dUINT8", DFNT_UINT8, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dFLOAT32", DFNT_FLOAT32, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dFLOAT64", DFNT_FLOAT64, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dCHAR8", DFNT_CHAR8, ORDER)==FAIL) return 1;
  if(VSfdefine(vdata_id, "dUCHAR8", DFNT_UCHAR8, ORDER)==FAIL) return 1;

  if(VSsetfields(vdata_id, FIELDNAME_LIST)==FAIL)  return 1;

  /* Build an array of pointers to each field */
  fldbufptrs[0] = &field1[0];
  fldbufptrs[1] = &field2[0];
  fldbufptrs[2] = &field3[0];
  fldbufptrs[3] = &field4[0];
  fldbufptrs[4] = &field5[0];
  fldbufptrs[5] = &field6[0];
  fldbufptrs[6] = &field7[0];
  fldbufptrs[7] = &field8[0];
  fldbufptrs[8] = &field9[0];
  fldbufptrs[9] = &field10[0];

  /* Pack all data in the field buffers into the buffer databuf */
  if(VSfpack(vdata_id, _HDF_VSPACK, NULL, databuf,
	     BUF_SIZE, N_RECORDS, NULL, fldbufptrs)==FAIL) return 1;

  /* Write all records to the vdata */
  if(VSwrite(vdata_id, (uint8*)databuf, N_RECORDS, FULL_INTERLACE)==FAIL)
    return 1;

  if(VSdetach(vdata_id)==FAIL)  return 1;
  if(Vend(file_id)==FAIL)    return 1;
  if(Hclose(file_id)==FAIL)    return 1;
  printf("Success!\n");
  return 0;
}
  
