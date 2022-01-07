/* makegr4.c: Creates testfile (testgr4.hdf) for HDF-EOS server
 * Creates a GR with "old-style" palette
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testgr4.hdf"
#define RIG_NAME   "GR_Palette"
#define MAX_COMP   3
#define N_ENTRIES  256
#define N_COMPS_PAL 3

#define Y_LENGTH 5
#define X_LENGTH 5

int main() {
  int32 file_id, gr_id, ri_id;  /* GR interface and data set ids */
  int32 pal_id; /* Palette id */
  const int32 start[2] = {0, 0};
  const int32 dims[2] = {Y_LENGTH, X_LENGTH};
  int16 image_data[Y_LENGTH][X_LENGTH][MAX_COMP];         /* test data */
  uint8 palette_buf[N_ENTRIES][N_COMPS_PAL];
  int i, j, k;

  /* Initialize image */
  for(j=0; j<Y_LENGTH; j++) {
    for(i=0; i<X_LENGTH; i++) {
      for(k=0; k<MAX_COMP; k++) {
	image_data[j][i][k] = j*160 + i*32 + k + 1;
      }
    }
  }

  /* Initialize palette to grayscale. */
  for(i=0; i<N_ENTRIES; i++) {
    palette_buf[i][0] = i;
    palette_buf[i][1] = i;
    palette_buf[i][2] = i;
  }

  if((file_id = Hopen(FILE_NAME, DFACC_CREATE, 0)) == FAIL)
    return 1;
  printf("Created HDF\n");

  gr_id = GRstart(file_id);  /* Initialize the GR interface. */

  printf("Adding GR\n");
  ri_id = GRcreate(gr_id, RIG_NAME, MAX_COMP, 
		   DFNT_INT16, MFGR_INTERLACE_PIXEL, dims);
  if(ri_id == FAIL)
    return 1;

  if(GRwriteimage(ri_id, start, NULL, dims, image_data) == FAIL)
    return 2;
  if((pal_id=GRgetlutid(ri_id, 0))==FAIL)
    return 3;
  if(GRwritelut(pal_id, N_COMPS_PAL, DFNT_UINT8, MFGR_INTERLACE_PIXEL,
		N_ENTRIES, palette_buf)==FAIL)
    return 4;

  {
    int32 ncomp, nt, junk, num_entries;
    if(GRgetlutinfo(pal_id, &ncomp, &nt, &junk, &num_entries) == FAIL)
	return 5;
    printf("number type: %d, num entries: %d\n", nt, num_entries);
  }
  GRendaccess(ri_id);  /* end access to dataset */

  GRend(gr_id);  /* end access to GR interface */
  Hclose(file_id);  /* close file */
  printf("Success!\n");
  return 0;
}
  
