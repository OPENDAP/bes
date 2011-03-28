/* readgr4.c:  demonstrates a bug in the HDF library (the palette's
 * number type mysteriously changes from DFNT_UINT8 to DFNT_UCHAR8).
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
  // int i, j, k; unused jhrg 3/16/11

  if((file_id = Hopen(FILE_NAME, DFACC_RDONLY, 0)) == FAIL)
    return 1;

  gr_id = GRstart(file_id);  /* Initialize the GR interface. */

  ri_id = GRselect(gr_id, GRnametoindex(gr_id, RIG_NAME));
  if(ri_id == FAIL)
    return 1;

  if((pal_id=GRgetlutid(ri_id, 0))==FAIL)
    return 3;
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
  
