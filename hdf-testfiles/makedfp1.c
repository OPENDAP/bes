/* makedfp1.c: Creates testfile (testdfp1.hdf) for HDF-EOS server
 * Writes a standalone palette
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hdf.h"

#define FILE_NAME  "testdfp1.hdf"

main() {
  uint8 colors[256*3];
  int i;

  for(i=0; i<256; i++) {
    colors[i*3+0] = i;
    colors[i*3+1] = i;
    colors[i*3+2] = 255-i;
  }

  /* remove old HDF file, if it exists */
  unlink(FILE_NAME);
  
  if(DFPaddpal(FILE_NAME, colors)==FAIL)
    return 1;

  printf("Success!\n");
  return 0;
}
  
