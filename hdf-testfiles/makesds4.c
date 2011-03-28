/* makesds4.c: Creates testfile (testsds4.hdf) for HDF-EOS server
 * Tests SDS with dimension scales (DODS Grid type) and predefined attributes
 *  (except SDsetrange() which is tested in makesds5.c)
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testsds4.hdf"
#define SDS_NAME   "SDS_1"
#define DIM_NAME_X "X_Axis"
#define DIM_NAME_Y "Y_Axis"
#define NAME_LENGTH 6
#define X_LENGTH   5
#define Y_LENGTH   16
#define RANK       2

main() {
  int32 sd_id, sds_id;  /* SD interface and data set ids */
  int32 dim_index, dim_id; /* Dimension index and ID */
  const int32 start[2] = {0, 0};
  const int32 dim_sizes[2] = {16, 5};
  int16 data_X[X_LENGTH];  /* X dimension scale */
  float64 data_Y[Y_LENGTH];  /* Y dimension scale */
  // char  dim_name[NAME_LENGTH]; unused jhrg 3/16/11
  int32 fill_value = 42;
  int i; // , j; unused jhrg 3/16/11

  if((sd_id = SDstart (FILE_NAME, DFACC_CREATE)) == FAIL)
    return 1;
  printf("Created HDF\n");

  printf("Adding SDS: %s\n", SDS_NAME);
  sds_id = SDcreate(sd_id, SDS_NAME, DFNT_INT32, 2, dim_sizes);
  if(sds_id == FAIL)
    return 1;

  if(SDsetcal(sds_id, 5, 0.001, 42, 0.002, DFNT_INT8) == FAIL)
    return 1;

  if(SDsetfillvalue(sds_id, (VOIDP)&fill_value) == FAIL)
    return 1;

  /* Initialize dim scales. */
  for (i=0; i<X_LENGTH; i++) data_X[i] = i;
  for (i=0; i<Y_LENGTH; i++) data_Y[i] = 0.1 * i;

  /* For each dimension, get its dimension id and set dimension name & scale.
   * Note that each dimension can have its own data type
   */
  for (dim_index = 0; dim_index < RANK; dim_index++) {
    /* Select the dimension at position dim_index. */
    dim_id = SDgetdimid(sds_id, dim_index);

    /* Assign name and dimension scale to selected dimension. */
    switch(dim_index) {
    case 0:  
      if(SDsetdimname(dim_id, DIM_NAME_Y)==FAIL)
	return 1;
      if(SDsetdimscale(dim_id,Y_LENGTH,DFNT_FLOAT64,(VOIDP)data_Y)==FAIL)
	return 1;
      if(SDsetdimstrs(dim_id, "Ylabel", "Yunit", "Yformat")==FAIL)
	return 1;
      break;
    case 1:  
      if(SDsetdimname(dim_id, DIM_NAME_X)==FAIL)
	return 1;
      if(SDsetdimscale(dim_id,X_LENGTH,DFNT_INT16,(VOIDP)data_X))
	return 1;
      if(SDsetdimstrs(dim_id, "Xlabel", "Xunit", "Xformat")==FAIL)
	return 1;
      break;
    }
  }

  /* Set string attributes */
  if (SDsetdatastrs(sds_id, "label", "unit", "F7.2", "cartesian") == FAIL)
    return 1;

  SDendaccess(sds_id);  /* end access to dataset */
  SDend(sd_id);  /* end access to SD interface and close file */
  printf("Success!\n");
  return 0;
}
  
