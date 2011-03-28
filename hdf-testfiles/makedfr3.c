/* makedfr3.c: Creates testfile (testdfr3.hdf) for HDF-EOS server
 * Contains two JPEG compressed DF24's (due to an HDF bug the first is hidden)
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hdf.h"
#include "hcomp.h"

#define FILE_NAME  "testdfr3.hdf"
#define WIDTH 5
#define HEIGHT 6
#define PIXEL_DEPTH 3

main() {
  int i;
  static comp_info compress_info;

  /* Initialize the image array */
  static uint8 raster_data24[HEIGHT][WIDTH][PIXEL_DEPTH] = {
     0, 1, 2,     3, 4, 5,     6, 7, 8,      9,10,11,     12,13,14,
     50,51,52,   53,54,55,    56,57,58,     59,60,61,     62,63,64,
    100,101,102, 103,104,105, 106,107,108, 109,110,111,  112,113,114,
    150,151,152, 153,154,155, 156,157,158, 159,160,161,  162,163,164,
    200,201,202, 203,204,205, 206,207,208, 209,210,211,  212,213,214,
    241,242,243, 244,245,246, 247,248,249, 250,251,252,  253,254,255
  };

  compress_info.jpeg.quality = 60;
  compress_info.jpeg.force_baseline = 1;

  /* remove old HDF file, if it exists */
  unlink(FILE_NAME);
  
#if 0
  // I think NCSA maybe changed the JPEG code (or the JPEG Folks did) since
  // this now includes some very odd values when run through JPEG. Also, I
  // don't think we should be testing the values of a lossy compression
  // algorithm! jhrg 10/10/05
  if(DF24setcompress(COMP_JPEG, &compress_info)==FAIL)
    return 1;
#endif
  if(DF24addimage(FILE_NAME, raster_data24, WIDTH, HEIGHT)==FAIL)
    return 1;
  if(DF24addimage(FILE_NAME, raster_data24, WIDTH, HEIGHT)==FAIL)
    return 1;

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
  
