/* makean1.c: Creates testfile (testan1.hdf) for HDF-EOS server
 * Contains file and object annotations
 * Written by Jake Hamby
 */

#include <stdio.h>
#include <stdlib.h>
#include "hdf.h"

#define FILE_NAME  "testan1.hdf"
#define VG_NAME    "AN Vgroup"
#define  FILE_LABEL_TXT "General HDF objects"
#define  FILE_DESC_TXT  "This is an HDF file that contains general HDF objects"
#define  DATA_LABEL_TXT "Common AN Vgroup"
#define  DATA_DESC_TXT  "This is a vgroup that is used to test data annotations"

int main() {
  int32 file_id, an_id;  /* File and AN id */
  int32 file_label_id, file_desc_id, data_label_id, data_desc_id;
  int32 vgroup_id;
  uint16 vgroup_tag, vgroup_ref;

  if((file_id = Hopen(FILE_NAME, DFACC_CREATE, 0)) == FAIL)
    return 1;
  printf("Created HDF\n");

  if((an_id=ANstart(file_id))==FAIL)
    return 1;

  if((file_label_id=ANcreatef(an_id, AN_FILE_LABEL))==FAIL)
    return 1;

  if(ANwriteann(file_label_id, FILE_LABEL_TXT, strlen(FILE_LABEL_TXT))==FAIL)
    return 1;

  if(ANendaccess(file_label_id)==FAIL)
    return 1;

  if((file_desc_id=ANcreatef(an_id, AN_FILE_DESC))==FAIL)
    return 1;

  if(ANwriteann(file_desc_id, FILE_DESC_TXT, strlen(FILE_DESC_TXT))==FAIL)
    return 1;

  if(ANendaccess(file_desc_id)==FAIL)
    return 1;

  /* Create a vgroup */
  if(Vstart(file_id)==FAIL)
    return 1;

  if((vgroup_id = Vattach(file_id, -1, "w"))==FAIL)
    return 1;

  if(Vsetname(vgroup_id, VG_NAME)==FAIL)
    return 1;

  /* Obtain the tag and ref number for subsequent references. */
  vgroup_tag = (uint16) VQuerytag(vgroup_id);
  vgroup_ref = (uint16) VQueryref(vgroup_id);

  /* Terminate access to the vgroup */
  if(Vdetach(vgroup_id)==FAIL)
    return 1;
  if(Vend(file_id)==FAIL)
    return 1;

  /* Create the data label for the vgroup identified by its tag and ref # */
  if((data_label_id=ANcreate(an_id, vgroup_tag, vgroup_ref, AN_DATA_LABEL))
     ==FAIL)
    return 1;

  if(ANwriteann(data_label_id, DATA_LABEL_TXT, strlen(DATA_LABEL_TXT))==FAIL)
    return 1;

  if(ANendaccess(data_label_id)==FAIL)
    return 1;

  if((data_desc_id = ANcreate(an_id, vgroup_tag, vgroup_ref, AN_DATA_DESC))
      ==FAIL)
    return 1;

  if(ANwriteann(data_desc_id, DATA_DESC_TXT, strlen(DATA_DESC_TXT))==FAIL)
    return 1;


  if(ANendaccess(data_desc_id)==FAIL)
    return 1;

  ANend(an_id);
  Hclose(file_id);  /* close file */
  printf("Success!\n");
  return 0;
}
  
