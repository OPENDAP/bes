/* makedfr1.c: Creates testfile (testdfr1.hdf) for HDF-EOS server
 * Contains a sample DFR8 and DF24 image, plus DFR8 palette
 * Values written include 0, 1, and maximum for each component
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hdf.h"

#define FILE_NAME  "testdfr1.hdf"
#define WIDTH 5
#define HEIGHT 6
#define PIXEL_DEPTH 3

main() {
  uint8 colors[256*3];
  int i;

  /* Initialize the image array */
  static uint8 raster_data8[HEIGHT][WIDTH] = {
    0, 1, 2, 3, 4,
    50, 51, 52, 53, 54,
    100, 101, 102, 103, 104,
    150, 151, 152, 153, 154,
    200, 201, 202, 203, 204,
    251, 252, 253, 254, 255
  };

  static uint8 raster_data24[HEIGHT][WIDTH][PIXEL_DEPTH] = {
     0, 1, 2,     3, 4, 5,     6, 7, 8,      9,10,11,     12,13,14,
     50,51,52,   53,54,55,    56,57,58,     59,60,61,     62,63,64,
    100,101,102, 103,104,105, 106,107,108, 109,110,111,  112,113,114,
    150,151,152, 153,154,155, 156,157,158, 159,160,161,  162,163,164,
    200,201,202, 203,204,205, 206,207,208, 209,210,211,  212,213,214,
    241,242,243, 244,245,246, 247,248,249, 250,251,252,  253,254,255
  };

  for(i=0; i<256; i++) {
    colors[i*3+0] = i;
    colors[i*3+1] = i;
    colors[i*3+2] = 255-i;
  }

  /* remove old HDF file, if it exists */
  unlink(FILE_NAME);
  
  if(DFR8setpalette(colors)==FAIL)
    return 1;

  if(DFR8addimage(FILE_NAME, raster_data8, WIDTH, HEIGHT, 0)==FAIL)
    return 1;

  if(DF24addimage(FILE_NAME, raster_data24, WIDTH, HEIGHT)==FAIL)
    return 1;

  if(DF24addimage(FILE_NAME, raster_data24, WIDTH, HEIGHT)==FAIL)
    return 1;

  printf("%d 8-bit images and %d 24-bit images\n",
	 DFR8nimages(FILE_NAME), DF24nimages(FILE_NAME));

  printf("Success!\n");
  return 0;
}
  
