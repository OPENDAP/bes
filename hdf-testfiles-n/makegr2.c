/* makegr2.c: Creates testfile (testgr2.hdf) for HDF-EOS server
 * Contains GR's from 1 to 32 components
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testgr2.hdf"
#define RIG_NAME   "GR_%d"
#define MAX_COMP   32

#define Y_LENGTH 5
#define X_LENGTH 5

int main() {
  int32 file_id, gr_id, ri_id;  /* SD interface and data set ids */
  int numgr = MAX_COMP;  /* # of tests */
  const int32 start[2] = {0, 0};
  const int32 dims[2] = {Y_LENGTH, X_LENGTH};
  int16 image_data[Y_LENGTH][X_LENGTH][MAX_COMP];         /* test data (changes depending on type) */
  int i, j, k;
  char grname[32];

  /* Initialize image */
  for(j=0; j<Y_LENGTH; j++) {
    for(i=0; i<X_LENGTH; i++) {
      for(k=0; k<MAX_COMP; k++) {
	image_data[j][i][k] = j*160 + i*32 + k + 1;
      }
    }
  }

  if((file_id = Hopen(FILE_NAME, DFACC_CREATE, 0)) == FAIL)
    return 1;
  printf("Created HDF\n");

  gr_id = GRstart(file_id);  /* Initialize the GR interface. */

  for(i=1; i<=numgr; i*=2) {
    printf("Adding GR: %d\n", i);
    sprintf(grname, RIG_NAME, i);
    ri_id = GRcreate(gr_id, grname, i, 
		    DFNT_INT16, MFGR_INTERLACE_PIXEL, dims);
    if(ri_id == FAIL)
      return 1;

    if(GRwriteimage(ri_id, start, NULL, dims, image_data) == FAIL)
      return 2;
    GRendaccess(ri_id);  /* end access to dataset */
  }
  GRend(gr_id);  /* end access to GR interface */
  Hclose(file_id);  /* close file */
  printf("Success!\n");
  return 0;
}
  
