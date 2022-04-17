/* makevs5.c: Creates testfile (testvs5.hdf) for HDF-EOS server
 * Creates a variety of Vgroups containing one of each type of HDF object
 * Also adds a test attribute to each Vgroup
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"

#define FILE_NAME "testvs5.hdf"

main() {
  int32 file_id, vgroup_id1, vgroup_id2;  /* ID of file and Vgroups */
  int32 sd_id, sds_id, sds_ref, dim_id;  /* SDS info */
  int32 start[2] = {0, 0}, dim_sizes[2] = {5, 5}; /* dimensions */
  int32 dimscale[5] = {1, 2, 3, 4, 5};  /* SDS dimension scale */
  int32 gr_id, ri_id, gr_ref;  /* GR info */
  int32 grbuffer[5][5], i, j;  /* empty image */
  uint8 rasbuffer[5][5][3];    /* image for DFR8 and DF24 */
  int32 vdata_ref;             /* Vdata info */

  if((file_id = Hopen(FILE_NAME, DFACC_CREATE, 0))==FAIL) return 1;
  if(Vstart(file_id)==FAIL) return 1;

  /* Create main Vgroup */
  if((vgroup_id1 = Vattach(file_id, -1, "w"))==FAIL) return 1;
  if(Vsetname(vgroup_id1, "Main_Vgroup")==FAIL) return 1;
  if(Vsetclass(vgroup_id1, "Main_Class")==FAIL) return 1;
  if(Vsetattr(vgroup_id1, "Main_Attr", DFNT_CHAR8, 5, "Hello")==FAIL) return 1;

  /* Create SDS Vgroup (DODS Array) and add to main Vgroup */
  if((vgroup_id2 = Vattach(file_id, -1, "w"))==FAIL) return 1;
  if(Vsetname(vgroup_id2, "Array_Vgroup")==FAIL) return 1;
  if(Vsetclass(vgroup_id2, "Array_Class")==FAIL) return 1;
  if(Vsetattr(vgroup_id2, "Array_Attr", DFNT_CHAR8, 5, "Hello")==FAIL) return 1;
  if(Vinsert(vgroup_id1, vgroup_id2)==FAIL) return 1;

  /* Create SDS (DODS Array) and add to Vgroup */
  if((sd_id=SDstart(FILE_NAME, DFACC_WRITE))==FAIL) return 2;
  if((sds_id=SDcreate(sd_id, "SDS_Array", DFNT_INT32, 1, dim_sizes))==FAIL)
    return 3;
  if((sds_ref=SDidtoref(sds_id))==FAIL) return 4;
  if(Vaddtagref(vgroup_id2, DFTAG_NDG, sds_ref)==FAIL) return 5;
  if(Vdetach(vgroup_id2)==FAIL) return 1;
  SDendaccess(sds_id);

  /* Create another SDS Vgroup */
  if((vgroup_id2 = Vattach(file_id, -1, "w"))==FAIL) return 1;
  if(Vsetname(vgroup_id2, "Grid_Vgroup")==FAIL) return 1;
  if(Vsetclass(vgroup_id2, "Grid_Class")==FAIL) return 1;
  if(Vsetattr(vgroup_id2, "Grid_Attr", DFNT_CHAR8, 5, "Hello")==FAIL) return 1;
  if(Vinsert(vgroup_id1, vgroup_id2)==FAIL) return 1;

  /* Create SDS with dimension scale (DODS Grid) and add to Vgroup */
  if((sds_id=SDcreate(sd_id, "SDS_Grid", DFNT_INT32, 1, dim_sizes))==FAIL)
    return 6;
  if((dim_id = SDgetdimid(sds_id, 0))==FAIL) return 7;
  if(SDsetdimscale(dim_id, 5, DFNT_INT32, dimscale)==FAIL) return 8;
  if((sds_ref=SDidtoref(sds_id))==FAIL) return 9;
  if(Vaddtagref(vgroup_id2, DFTAG_NDG, sds_ref)==FAIL) return 10;
  if(Vdetach(vgroup_id2)==FAIL) return 1;
  SDendaccess(sds_id);
  SDend(sd_id);

  /* Create a General Raster Vgroup */
  if((vgroup_id2 = Vattach(file_id, -1, "w"))==FAIL) return 1;
  if(Vsetname(vgroup_id2, "GR_Vgroup")==FAIL) return 1;
  if(Vsetclass(vgroup_id2, "GR_Class")==FAIL) return 1;
  if(Vsetattr(vgroup_id2, "GR_Attr", DFNT_CHAR8, 5, "Hello")==FAIL) return 1;
  if(Vinsert(vgroup_id1, vgroup_id2)==FAIL) return 1;

  /* Create General Raster and add to Vgroup */
  if((gr_id = GRstart(file_id))==FAIL) return 1;
  for(i=0; i<5; i++)
    for(j=0; j<5; j++)
      grbuffer[i][j] = i+j;
  if((ri_id = GRcreate(gr_id, "GRaster", 1, DFNT_INT32, MFGR_INTERLACE_PIXEL,
		       dim_sizes))==FAIL) return 11;
  if(GRwriteimage(ri_id, start, NULL, dim_sizes, grbuffer)==FAIL) return 11;
  if((gr_ref=GRidtoref(ri_id))==0) return 12;
  if(Vaddtagref(vgroup_id2, DFTAG_VG, gr_ref)==FAIL) return 13;
  if(Vdetach(vgroup_id2)==FAIL) return 1;
  GRendaccess(ri_id);
  GRend(gr_id);

#if 0
  /* Create a DFR8 Vgroup */
  if((vgroup_id2 = Vattach(file_id, -1, "w"))==FAIL) return 1;
  if(Vsetname(vgroup_id2, "DFR8_Vgroup")==FAIL) return 1;
  if(Vsetclass(vgroup_id2, "DFR8_Class")==FAIL) return 1;
  if(Vsetattr(vgroup_id2, "DFR8_Attr", DFNT_CHAR8, 5, "Hello")==FAIL) return 1;
  if(Vinsert(vgroup_id1, vgroup_id2)==FAIL) return 1;

  /* Create DFR8 and add to Vgroup */
  for(i=0; i<5; i++) {
    for(j=0; j<5; j++) {
      rasbuffer[i][j][0] = i+j;
      rasbuffer[i][j][1] = i+j+1;
      rasbuffer[i][j][2] = i+j+2;
    }
  }
  if(DFR8addimage(FILE_NAME, rasbuffer, 5, 5, 0)==FAIL) return 14;
  if((gr_ref=DFR8lastref())==FAIL) return 14;
  if(Vaddtagref(vgroup_id2, DFTAG_RI8, gr_ref)==FAIL) return 15;
  if(Vdetach(vgroup_id2)==FAIL) return 1;

  /* Create a DF24 Vgroup */
  if((vgroup_id2 = Vattach(file_id, -1, "w"))==FAIL) return 1;
  if(Vsetname(vgroup_id2, "DF24_Vgroup")==FAIL) return 1;
  if(Vsetclass(vgroup_id2, "DF24_Class")==FAIL) return 1;
  if(Vsetattr(vgroup_id2, "DF24_Attr", DFNT_CHAR8, 5, "Hello")==FAIL) return 1;
  if(Vinsert(vgroup_id1, vgroup_id2)==FAIL) return 1;

  /* Create DF24 and add to Vgroup */
  if(DF24addimage(FILE_NAME, rasbuffer, 5, 5)==FAIL) return 14;
  if((gr_ref=DF24lastref())==FAIL) return 14;
  if(Vaddtagref(vgroup_id2, DFTAG_RI, gr_ref)==FAIL) return 15;
  if(Vdetach(vgroup_id2)==FAIL) return 1;
#endif

  /* Create a Vgroup for Vdata */
  if((vgroup_id2 = Vattach(file_id, -1, "w"))==FAIL) return 1;
  if(Vsetname(vgroup_id2, "Vdata_Vgroup")==FAIL) return 1;
  if(Vsetclass(vgroup_id2, "Vdata_Class")==FAIL) return 1;
  if(Vsetattr(vgroup_id2, "Vdata_Attr", DFNT_CHAR8, 5, "Hello")==FAIL) return 1;
  if(Vinsert(vgroup_id1, vgroup_id2)==FAIL) return 1;

  /* Create Vdata and add to Vgroup */
  if((vdata_ref = VHstoredata(file_id, "Field", (uint8*)dimscale,
			      5, DFNT_INT32, "Vdata", "Vdata_Class"))==FAIL)
    return 16;
  if(Vaddtagref(vgroup_id2, DFTAG_VH, vdata_ref)==FAIL) return 1;
  if(Vdetach(vgroup_id2)==FAIL) return 1;

  /* Close all open handles */
  if(Vdetach(vgroup_id1)==FAIL) return 1;
  Vend(file_id);
  Hclose(file_id);

  printf("Success!\n");
  return 0;
}
  
