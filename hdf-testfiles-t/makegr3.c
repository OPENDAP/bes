/* makegr3.c: Creates testfile (testgr3.hdf) for HDF-EOS server
 * Demonstrates various interlace formats
 * NOTE: this isn't much of a test for DODS because the HDF library
 *   converts all formats to MFGR_INTERLACE_PIXEL before writing.
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testgr3.hdf"
#define RIG_NAME   "GR_%s"
#define MAX_COMP   3

#define Y_LENGTH 5
#define X_LENGTH 5

struct grinfo {
  int32 interlace_mode;
  const char *interlace_name;
};

const struct grinfo tests[] = {
  {MFGR_INTERLACE_PIXEL, "Pixel"},
  {MFGR_INTERLACE_LINE, "Line"},
  {MFGR_INTERLACE_COMPONENT, "Component"}
};

int main() {
  int32 file_id, gr_id, ri_id;  /* SD interface and data set ids */
  int numgr = sizeof(tests) / sizeof(struct grinfo);  /* # of tests */
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

  for(i=0; i<numgr; i++) {
    printf("Adding GR: %s\n", tests[i].interlace_name);
    sprintf(grname, RIG_NAME, tests[i].interlace_name);
    ri_id = GRcreate(gr_id, grname, MAX_COMP, 
		    DFNT_INT16, tests[i].interlace_mode, dims);
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
  
