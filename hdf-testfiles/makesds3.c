/* makesds3.c: Creates testfile (testsds1.hdf) for HDF-EOS server
 * Creates SDS's with a variety of dimensions, from 1 to 20
 * NOTE:  HDF server MAXDIMS = 20, HDF itself supports up to 32
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME  "testsds3.hdf"
#define SDS_NAME   "SDS_%d"

struct sdsinfo {
  int32 rank;
  int32 numelts[MAX_VAR_DIMS];
};

/* Note: dimension sizes chosen more or less at random. */
const struct sdsinfo tests[] = {
  {2, {8, 4}},
  {3, {4, 4, 2}},
  {4, {2, 2, 2, 2}},
  {5, {2, 2, 2, 2, 2}},
  {6, {2, 1, 2, 1, 2, 1}},
  {7, {2, 1, 2, 1, 2, 1, 2}},
  {8, {1, 2, 1, 2, 1, 2, 1, 2}},
  {16, {2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1}},
  {20, {2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1}}
#if 0
  {32, {1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 1,
	2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2}}
#endif
};

main() {
  int32 sd_id, sds_id;  /* SD interface and data set ids */
  int numsds = sizeof(tests) / sizeof(struct sdsinfo);  /* # of tests */
  const int32 start[MAX_VAR_DIMS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int32 **base[MAX_VAR_DIMS];         /* test data (changes depending on type) */
  int32 *lastdim;
  int i, j, k, rank, count, count2, val=1;
  char sdsname[32];

  if((sd_id = SDstart (FILE_NAME, DFACC_CREATE)) == FAIL)
    return 1;
  printf("Created HDF\n");

  for(i=0; i<numsds; i++) {
    printf("Adding SDS: %d dims\n", tests[i].rank);
    sprintf(sdsname, SDS_NAME, tests[i].rank);
    sds_id = SDcreate(sd_id, sdsname, DFNT_INT32, tests[i].rank, tests[i].numelts);
    if(sds_id == FAIL)
      return 1;

    /* This is a scary looking algorithm to allocate and initialize an array
     * of arrays for an arbitrary number of dimensions.  You might want to
     * look at it on paper or step through it in a visual debugger like "ddd"
     * to verify that it works.
     */

    rank = tests[i].rank;
    count = tests[i].numelts[0];
    base[0] = malloc(sizeof(int32**) * count);
    for(j=1; j<rank-1; j++) {
      count2 = count * tests[i].numelts[j];
      base[j] = malloc(sizeof(int32**) * count2);
      for(k=0; k<count; k++) {
	*(base[j-1] + k) = base[j] + k * tests[i].numelts[j];
      }
      count = count2;
    }
    count2 = count * tests[i].numelts[rank-1];
    lastdim = malloc(sizeof(int32) * count2);
    for(k=0; k<count; k++) {
      *(base[rank-2] + k) = lastdim + k * tests[i].numelts[rank-1];
    }
    for(k=0; k<count2; k++) {
      *(lastdim+k) = val++;
    }

    if(SDwritedata(sds_id, start, NULL, tests[i].numelts, lastdim) == FAIL)
      return 1;

    /* free(buffer); */  /* Don't even bother to free: too complicated */
    SDendaccess(sds_id);  /* end access to dataset */
  }
  SDend(sd_id);  /* end access to SD interface and close file */
  printf("Success!\n");
  return 0;
}
  
