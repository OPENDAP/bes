/* makedfr2.c: Creates testfile (testdfr2.hdf) for HDF-EOS server
 * Contains a DFR8 compressed with RLE, JPEG, and IMCOMP compression
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hdf.h"
#include "hcomp.h"

#define FILE_NAME  "testdfr2.hdf"
#define WIDTH 5
#define HEIGHT 6
#define PIXEL_DEPTH 3

main() {
  uint8 colors[256*3];
  int i;
  static comp_info compress_info;

  /* Initialize the image array */
  static uint8 raster_data8[HEIGHT][WIDTH] = {
    0, 1, 2, 3, 4,
    50, 51, 52, 53, 54,
    100, 101, 102, 103, 104,
    150, 151, 152, 153, 154,
    200, 201, 202, 203, 204,
    251, 252, 253, 254, 255
  };

  for(i=0; i<256; i++) {
    colors[i*3+0] = i;
    colors[i*3+1] = i;
    colors[i*3+2] = 255-i;
  }

  compress_info.jpeg.quality = 60;
  compress_info.jpeg.force_baseline = 1;

  /* remove old HDF file, if it exists */
  unlink(FILE_NAME);
  
  if(DFR8setpalette(colors)==FAIL)
    return 1;

  if(DFR8addimage(FILE_NAME, raster_data8, WIDTH, HEIGHT, COMP_RLE)==FAIL)
    return 1;
  if(DFR8addimage(FILE_NAME, raster_data8, WIDTH, HEIGHT, COMP_JPEG)==FAIL)
    return 1;
#if 0
  // with hdf4 4.2r1 this seems to produce values that vary given the default
  // palette in ways that I don't really understand (but other programs like
  // hdfview also show the variation. I'm skipping this so that the data test
  // won't have to use 'match.' jhrg 10/10/05
  if(DFR8addimage(FILE_NAME, raster_data8, WIDTH, HEIGHT, COMP_IMCOMP)==FAIL)
    return 1;
#endif
  printf("%d 8-bit images and %d 24-bit images\n",
	 DFR8nimages(FILE_NAME), DF24nimages(FILE_NAME));

  /* Verify names with GR interface */
  {
    int32 file_id, gr_id, ri_id;
    int32 n_images, n_attrs;
    int32 ncomp, type, lace, dims[2], nattrs;
    // char name[32]; Unused jhrg 3/16/11

    file_id = Hopen(FILE_NAME, DFACC_READ, 0);
    gr_id = GRstart(file_id);
    GRfileinfo(gr_id, &n_images, &n_attrs);
    printf("%d images in GR and %d attributes\n", n_images, n_attrs);
    for(i=0; i<n_images; i++) {
      ri_id = GRselect(gr_id, i);
      GRgetiminfo(ri_id, name, &ncomp, &type, &lace, dims, &nattrs);
      printf("%d: %s, ncomp=%d, type=%d\n", i, name, ncomp, type);
      GRendaccess(ri_id);
    }
    GRend(gr_id);
    Hclose(file_id);
  }

  printf("Success!\n");
  return 0;
}
  
