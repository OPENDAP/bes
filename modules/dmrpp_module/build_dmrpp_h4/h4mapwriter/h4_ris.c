/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2013 The HDF Group                                     *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file and in the print documentation copyright notice.         *
 * COPYING can be found at the root of the source code distribution tree;    *
 * the copyright notice in printed HDF documentation can be found on the     *
 * back of the title page.  If you do not have access to either version,     *
 * you may request a copy from help@hdfgroup.org.                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*! 
  \file h4_ris.c
  \brief Read raster image information.

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date October 11, 2013
  \note	made error message consistent in RISget_refs().

  \author Ruth Aydt (aydt@hdgroup.org)
  \date July 19, 2012
  \note revised RISmapping().

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date February 23, 2011
  \note revised for full release.

  \author Peter Cao (xcao@hdfgroup.org)
  \date February 20, 2008

*/

#include "hmap.h"


/*!

  \fn intn RISfree_mapping_info(RIS_mapping_info_t  *map_info)
  \brief Release memory held mapping infomation.

  \author Peter Cao (xcao@hdfgroup.org)
  \date February 20, 2008

  \return SUCCEED
  \return FAIL

*/
intn
RISfree_mapping_info(RIS_mapping_info_t  *map_info)
{
    intn    ret_value = SUCCEED;

    /* nothing to free */
    if (map_info == NULL) 
        return SUCCEED;

    if (map_info->offsets) {
        HDfree(map_info->offsets);
        map_info->offsets = NULL;
    }

    if (map_info->lengths) {
        HDfree(map_info->lengths);
        map_info->lengths = NULL;
    }
    return ret_value;    

} /* end RISfree_mapping_info */

/*!

  \fn RISfree_ris_info(RIS_info_t  *ris_info)
  \brief Free memory held by \a ris_info.

*/
intn
RISfree_ris_info(RIS_info_t  *ris_info)
{
    intn    ret_value = SUCCEED;

    /* nothing to free */
    if (ris_info == NULL) 
        return SUCCEED;

    if (ris_info->refs) {
        HDfree(ris_info->refs);
        ris_info->refs = NULL;
    }

    if (ris_info->tags) {
        HDfree(ris_info->tags);
        ris_info->tags = NULL;
    }

    return ret_value;    
} /* RISfree_info_t */

/*! 

  \fn  RISget_refs(int32 gr_id, RIS_info_t *ris_info)
  \brief  Get all tags/refs of raster images 

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date October 11, 2013
  \note	made error message consistent.

  \date March 11, 2010
  \note Cleaned up error handling.

  \date September 22, 2010
  \note Created.

*/
intn 
RISget_refs(int32 gr_id, RIS_info_t *ris_info) 
{
    int i = 0;
    int32 n_images = 0;
    int32 n_file_attrs = 0;
    int32 stat = SUCCEED;

    HDmemset(ris_info, 0, sizeof(RIS_info_t));

    stat = GRfileinfo(gr_id, &n_images, &n_file_attrs);
    if(stat == FAIL){
        FAIL_ERROR("GRfileinfo() failed.");
    }

    ris_info->nimages = n_images;
    ris_info->refs = (int32 *) HDmalloc(n_images * sizeof(int32));
    if (ris_info->refs == NULL){
        FAIL_ERROR("HDmalloc(): Out of Memory");
    }
    ris_info->tags = (int32 *) HDmalloc(n_images * sizeof(int32));
    if (ris_info->tags == NULL){
        FAIL_ERROR("HDmalloc(): Out of Memory");
    }
    for(i =0; i < n_images; i++){
        uint16 ref = 0;
        /* Select the existing raster image. */
        int32 gid = GRselect(gr_id, i);
        if( gid == FAIL ){
            FAIL_ERROR("GRselect() failed.");
        }
        ref = GRidtoref(gid);
        ris_info->tags[i] = DFTAG_RIG;
        ris_info->refs[i] = (int32) ref;
        GRendaccess(gid);
    }
    return SUCCEED;
}

/*!

  \fn RISmapping(int32 file_id, int32 ri_id, RIS_mapping_info_t *map_info )

  \brief Read mapping information for the raster image.

  Given a raster image id, retrieves the information for mapping file.

  \return SUCCEED
  \return FAIL

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 19, 2012
  \note 
  replaced grgetcomptype() with GRgetcomptype() 
  changed type of comp_type from int32 to comp_coder_t.
  changed map_info->pal.ref to map_info->pal_ref for revised palette handling.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date May 9, 2011
  \note 
  handled the case when map_info->nblocks = 0, which means the data is empty 
  with fill values.

  \date May 5, 2011
  \note 
  changed flog to ferr to redirect UNMAPPABLE errors.
  added error handler after GRgetdatainfo() call.

  \date May 4, 2011
  \note 
  changed the ERROR to UNMAPPABLE for unsupported number of components
  and palettes.
     
  \date March 17, 2011
  \note 
  checked the number of components.
  removed local num_attrs and saved the number of attributes in map_info.
  removed HEclear().
  replaced map_info->nblocks = 1 with GRgetatainfo() call to fix
  potential memory errors reported by valgrind.

  \date March 14, 2011
  \note 
  replaced GRgetcompinfo() with grgetcomptype() since GRgetcompinfo()
  cannot detect COMP_IMCOMP.
  removed cinfo variable.
  changed type of comp_type variable to int32.
  cleaned up error handlers.

  \date March 10, 2011
  \note added is_mappable variable.

  \author Peter Cao (xcao@hdfgroup.org)

  \date February 20, 2008 
  \note created.

  \see read_compression()
*/
intn
RISmapping(int32 file_id, int32 ri_id, RIS_mapping_info_t *map_info)
{

    comp_coder_t comp_type = -1;
    int32 pal_id = -1;
    int32 ret_value = SUCCEED;
    intn status = FAIL;

    uint16 ref = 0;

    HDmemset(map_info, 0, sizeof(RIS_mapping_info_t));
    status = GR2bmapped(ri_id, &map_info->is_mappable, &map_info->is_r8);

    if(status == FAIL) { 
        FAIL_ERROR("GR2bmapped() failed.");
    }

    ref = GRidtoref(ri_id);
    if (GRgetiminfo(ri_id, map_info->name, &(map_info->ncomp), 
                    &(map_info->nt), &(map_info->interlace), 
                    map_info->dimsizes, &map_info->num_attrs) == FAIL){
        FAIL_ERROR("GRgetiminfo() failed.");        
    }

    if(map_info->ncomp != 1){
        fprintf(ferr, "UNMAPPABLE:GRgetiminfo() returned %ld components.", 
                (long)map_info->ncomp);
        fprintf(ferr, ":expected 1.\n");
    }


    /* Retrieve the number of palettes. */
    map_info->npals = GRgetnluts(ri_id);
    if(map_info->npals == FAIL){
        FAIL_ERROR("GRgetnluts() failed");
    }

    if(map_info->npals > 1){
        fprintf(ferr, "UNMAPPABLE:GRgetnluts() returned %d palettes.", 
                map_info->npals);
        fprintf(ferr, ":expected 1 or 0.\n");
    }

    if (map_info->dimsizes[0] <= 0 || map_info->dimsizes[1] <= 0) {
        fprintf(flog, "ERROR:Raster Image dimension sizes are invalid.");
        fprintf(flog, ":dimsize[0]=%ld, dimsize[1]=%ld\n",
                (long)map_info->dimsizes[0], (long)map_info->dimsizes[1]);
                return FAIL;
    }

    /* If there's a palette assocated with the raster, */
    if(map_info->npals == 1){
        if (FAIL == (pal_id = GRgetlutid(ri_id, 0))) {
            FAIL_ERROR("GRgetlutid() failed.");
        }
        /* Save the palette's ref number. */
        map_info->pal_ref = GRluttoref(pal_id);
        if(map_info->pal_ref == 0){
            FAIL_ERROR("GRluttoref() failed.");
        }
    }

    /* Can there be multiple blocks? - Ruth Says "NO" for RIS8 on 2/23/2011 .*/
    /* Thus, I set map_info->nblocks = 1 here. */
    /* However, setting map_info->nblocks = 1 gives 
       valgrind error on gr2.hdf test file. */
    map_info->nblocks =  GRgetdatainfo(ri_id, 0, 0, NULL, NULL);

    if(map_info->nblocks == FAIL){
        FAIL_ERROR("GRgetdatainfo() failed.");
    }

    if(map_info->nblocks > 0){
        map_info->offsets = (int32 *) 
            HDmalloc(sizeof(int32)*map_info->nblocks);
        if(map_info->offsets == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        map_info->lengths = (int32 *) 
            HDmalloc(sizeof(int32)*map_info->nblocks);
        if(map_info->lengths == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }


        /* Use the convenience function */
        ret_value = GRgetdatainfo(ri_id, 0, (uintn)map_info->nblocks, 
                                  map_info->offsets, map_info->lengths);
        if(ret_value == FAIL){
            FAIL_ERROR("GRgetdatainfo() failed.");
        }
    }

    if ((ret_value=GRgetcomptype(ri_id, &comp_type)) == FAIL){
        FAIL_ERROR("GRgetcomptype() failed.");
    }
    /* 
       We will not support any compression except for RLE unless we
       see otherwise. See email "Rasters / Library functions" 
       from Ruth on 2/19/2011. 
    */
    switch(comp_type) {
    case COMP_CODE_NONE:        /* Do nothing. */
        map_info->comp_info[0] = '\0';
        break;
    case COMP_CODE_RLE:         /* This is the only one supported. */
        snprintf(map_info->comp_info,COMP_INFO - 1,"compressionType=\"rle\"");
        break;        
    case COMP_CODE_NBIT:
        snprintf(map_info->comp_info, COMP_INFO - 1, 
                 "UNMAPPABLE:n-bit compression type is not supported for Raster.");
        break;
    case COMP_CODE_SKPHUFF:
        snprintf(map_info->comp_info, COMP_INFO - 1, 
                 "UNMAPPABLE:adaptive Huffman compression type is not supported for Raster.");
        break;
    case COMP_CODE_DEFLATE:
        snprintf(map_info->comp_info, COMP_INFO - 1, 
                 "UNMAPPABLE:gzip compression type is not supported for Raster.");
        break;
    case COMP_CODE_SZIP:
        snprintf(map_info->comp_info, COMP_INFO - 1, 
                "UNMAPPABLE:szip compression type is not supported for Raster.");
        break;
    case COMP_CODE_JPEG:
        snprintf(map_info->comp_info, COMP_INFO - 1, 
                "UNMAPPABLE:JPEG compression type is not supported for Raster.");
        break;
    case COMP_IMCOMP:
        snprintf(map_info->comp_info, COMP_INFO - 1, 
                "UNMAPPABLE:IMCOMP compression type is not supported for Raster.");
        break;

    default:
        map_info->comp_info[0] = '\0';
        fprintf(flog, "ERROR:Unknown compression type for Raster.");
        ret_value = FAIL;
        break;
    }


    return ret_value;
} /* RISmapping */



