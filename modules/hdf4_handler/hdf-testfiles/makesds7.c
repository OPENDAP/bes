/* makesds7.c: Creates testfile (testsds7.hdf) for HDF-EOS server
 * Writes chunked 2-dimension SDS's without compression, as well as
 * compressed with RLE, N-bit, Huffman, and Deflate
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testsds7.hdf"
#define SDS_NAME   "SDS_%s"
#define RANK       2

struct sdsinfo {
  comp_coder_t type;
  const char *typename;
};

const struct sdsinfo tests[] = {
  {COMP_CODE_NONE, "None"},
  {COMP_CODE_RLE, "RLE"},
  {COMP_CODE_NBIT, "N-bit"},
  {COMP_CODE_SKPHUFF, "Skip_Huffman"},
  {COMP_CODE_DEFLATE, "Deflate"}
};

int main() {
  int32 sd_id, sds_id;  /* SD interface and data set ids */
  int32         flag, maxcache, new_maxcache;
  int32         dim_sizes[2], origin[2];
  HDF_CHUNK_DEF c_def; /* Chunking definitions */ 
  int32         comp_flag;
  int32         start[2], edges[2];
  int16         row[2] = { 5, 5 };
  int16         column[3] = { 4, 4, 4 };
  int16         fill_value = 0;   /* Fill value */
  int           i;
  int numsds = sizeof(tests) / sizeof(struct sdsinfo);  /* # of tests */
  char sdsname[32];
   /*
   * Declare chunks data type and initialize some of them. 
   */
          int16 chunk1[3][2] = { 1, 1,
                                 1, 1,
                                 1, 1 }; 

          int16 chunk2[3][2] = { 2, 2,
                                 2, 2,
                                 2, 2 }; 

          int16 chunk3[3][2] = { 3, 3,
                                 3, 3,
                                 3, 3 }; 

          int16 chunk6[3][2] = { 6, 6,
                                 6, 6,
                                 6, 6 };

  if((sd_id = SDstart (FILE_NAME, DFACC_CREATE)) == FAIL)
    return 1;
  printf("Created HDF\n");

  /* Initialize constant values */
  dim_sizes[0] = 9;
  dim_sizes[1] = 4;

  for(i=0; i<numsds; i++) {
    printf("Adding SDS: %s\n", tests[i].typename);
    sprintf(sdsname, SDS_NAME, tests[i].typename);

    /* Create SDS */
    if((sds_id = SDcreate(sd_id, sdsname, DFNT_INT16, RANK, dim_sizes)) == FAIL)
      return 1;

    /* Fill the SDS array with the fill value. */
    if(SDsetfillvalue(sds_id, (VOIDP)&fill_value) == FAIL)
      return 1;
    
    /* Define chunk's dimensions. */
    switch(tests[i].type) {
    case COMP_CODE_NONE:
      c_def.chunk_lengths[0] = 3;
      c_def.chunk_lengths[1] = 2;
      break;
    case COMP_CODE_RLE:
    case COMP_CODE_SKPHUFF:
    case COMP_CODE_DEFLATE:
      c_def.comp.chunk_lengths[0] = 3;
      c_def.comp.chunk_lengths[1] = 2;
      break;
    case COMP_CODE_NBIT:
      c_def.nbit.chunk_lengths[0] = 3;
      c_def.nbit.chunk_lengths[1] = 2;
      break;
    }

    /* Create chunked SDS based on compression value. */
    switch(tests[i].type) {
    case COMP_CODE_NONE:
      comp_flag = HDF_CHUNK;
      break;
    case COMP_CODE_RLE:
      c_def.comp.comp_type = COMP_CODE_RLE;
      comp_flag = HDF_CHUNK | HDF_COMP;
      break;
    case COMP_CODE_SKPHUFF:
      c_def.comp.comp_type = COMP_CODE_SKPHUFF;
      c_def.comp.cinfo.skphuff.skp_size = sizeof(int16);
      comp_flag = HDF_CHUNK | HDF_COMP;
      break;
    case COMP_CODE_DEFLATE:
      c_def.comp.comp_type = COMP_CODE_DEFLATE;
      c_def.comp.cinfo.deflate.level = 9;
      comp_flag = HDF_CHUNK | HDF_COMP;
      break;
    case COMP_CODE_NBIT:
      comp_flag = HDF_CHUNK | HDF_NBIT;
      c_def.nbit.start_bit = 3;  /* only write 4 bits of data */
      c_def.nbit.bit_len = 4;
      c_def.nbit.sign_ext = 0;
      c_def.nbit.fill_one = 0;
      break;
    }
    if(SDsetchunk(sds_id, c_def, comp_flag) == FAIL)
      return 2;

    /* Set chunk cache to hold max 3 chunks */
    maxcache = 3;
    flag = 0;
    new_maxcache = SDsetchunkcache(sds_id, maxcache, flag);

    /* Write chunks using SDwritechunk */
    origin[0] = 0;
    origin[1] = 0;
    if(SDwritechunk (sds_id, origin, (VOIDP) chunk1)==FAIL)
      return 3;
    origin[0] = 1;
    origin[1] = 0;
    if(SDwritechunk (sds_id, origin, (VOIDP) chunk3)==FAIL)
      return 3;
    origin[0] = 0;
    origin[1] = 1;
    if(SDwritechunk (sds_id, origin, (VOIDP) chunk2)==FAIL)
      return 3;
    /* Write chunks using SDwritedata */
    start[0] = 6;
    start[1] = 2;
    edges[0] = 3;
    edges[1] = 2;
    if(SDwritedata (sds_id, start, NULL, edges, (VOIDP) chunk6)==FAIL)
      return 4;
    start[0] = 3;
    start[1] = 3;
    edges[0] = 3;
    edges[1] = 1;
    if(SDwritedata (sds_id, start, NULL, edges, (VOIDP) column)==FAIL)
      return 4;
    start[0] = 7;
    start[1] = 0;
    edges[0] = 1;
    edges[1] = 2;
    if(SDwritedata (sds_id, start, NULL, edges, (VOIDP) row)==FAIL)
      return 4;

    SDendaccess(sds_id);  /* end access to dataset */
  }
  SDend(sd_id);  /* end access to SD interface and close file */
  printf("Success!\n");
  return 0;
}

