/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2016  The HDF Group                                    *
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
  \file h4_vdata.c
  \brief Read Vdata mapping information.

  \author H. Joe Lee <hyoklee@hdfgroup.org>
  \date March 16, 2011
  \note handled Vdata with no data.

  \author Peter Cao <xcao@hdfgroup.org>
  \date October 12, 2007
  \note created.
 */

#include "hmap.h"


/*!

  \fn intn VSfree_mapping_info(VS_mapping_info_t  *map_info)
  \brief Release memory held mapping infomation

  Given \a map_info, release the memory held by \a map_info pointer.
  \return SUCCEED
  \return FAIL

  \author  Peter Cao (xcao@hdfgroup.org)
  \date  December 17, 2007 

*/
intn
VSfree_mapping_info(VS_mapping_info_t  *map_info)
{
    intn    ret_value = SUCCEED;

    /* If pointer is NULL, there is nothing to free */
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

    if(map_info->fnames){
        int j = 0;
        /* Free memory for names. */
        for(j=0; j < map_info->ncols; j++){
            if(map_info->fnames[j] != NULL)
                HDfree(map_info->fnames[j]);
        }
        HDfree(map_info->fnames);
    }

    if(map_info->forders){
        HDfree(map_info->forders);
    }


    if(map_info->ftypes){
        HDfree(map_info->ftypes);
    }

    return ret_value;    
} /* end VSfree_mapping_info */


/*!

   \fn intn VSmapping(int32 vdata_id, VS_mapping_info_t *map_info)
   \brief  Read Vdata mapping information.

   Given \a vdata_id, retrieve \a map_info for mapping file.

   \return SUCCEED
   \return FAIL

   \author  H. Joe Lee (hyoklee@hdfgroup.org)
   \date  August 2, 2016
   \note  removed ISO C90 warning.

   \date  March 16, 2011
   \note  cleaned up error messages.

   \date  March 15, 2011
   \note  
   cleaned up error messages.
   corrected the use of is_old_attr.
   removed the use of local old flag.
   added more error handlers.

   \date  February 24, 2011
   \note  removed the use of VDATA.

   \author  Peter Cao (xcao@hdfgroup.org)
   \date  December 17, 2007 

*/
intn
VSmapping(int32 vdata_id, VS_mapping_info_t *map_info )
{
    char* fname = NULL;
    
    int i = 0;                  /* for loop */
    
    int32 forder = 0;
    int32 ftype = 0;    
    int32 nblocks = 0;   /* Verify the blocks that are read. */
    
    /* Reset map_info. */
    HDmemset(map_info, 0, sizeof(VS_mapping_info_t));
    if (VSgetclass(vdata_id, map_info->class) == FAIL){
        FAIL_ERROR("VSgetclass() failed.");
    }

    /* Get the number of records. */
    map_info->nrows = VSelts(vdata_id);
    if(map_info->nrows == FAIL){
        FAIL_ERROR("VSelts() failed.");
    }

    /* Get the number of fields. */
    map_info->ncols = VFnfields(vdata_id);
    if(map_info->ncols == FAIL){
        FAIL_ERROR("VFnfields() failed.");
    }

    /* Get the interlace mode. */
    map_info->sorder = VSgetinterlace(vdata_id);
    if(map_info->sorder == FAIL){
        FAIL_ERROR("VSgetinterlace() failed.");
    }

    /* Save vdata name. */
    if(VSgetname(vdata_id, map_info->name) == FAIL) {
        FAIL_ERROR("VSgetname() failed.");
    };


    /* Skip processing if it should be mapped as a Vgroup's attribute. */
    /* See the usage of Vattrinfo2() and Vgetattr2() inside 
       write_group_attrs() defined in xml_vg.c  */
    if((strncmp(map_info->class, "Attr0.0", 7) == 0) 
       && (map_info->ncols == 1)){
        map_info->is_old_attr = 1;
        return SUCCEED;
    }
    /* Skip processing if it's internal, not an old attribute, and not 
       class "Attr0.0". */
    if (VSisinternal(map_info->class) && 
        (map_info->is_old_attr != 1) && 
        (strncmp(map_info->class,"Attr0.0", 7) != 0)){
        map_info->is_old_attr = 1;
        return SUCCEED;
    }

    /* Read field name, order, and type. */
    if(map_info->ncols > 0){
        /* Allocate memory for field names. */
        map_info->fnames = 
            (char **) HDmalloc(sizeof(char*)*map_info->ncols);
        if(map_info->fnames == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        /* Allocate memory for field orders. */
        map_info->forders = 
            (int32 *) HDmalloc(sizeof(int32)*map_info->ncols);
        if(map_info->forders == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        /* Allocate memory for field types. */
        map_info->ftypes = 
            (int32 *) HDmalloc(sizeof(int32)*map_info->ncols);
        if(map_info->ftypes == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        for(i=0; i < map_info->ncols; i++){
            /* Save the field name. */
            fname = VFfieldname(vdata_id, i);
            if(fname == NULL){
                FAIL_ERROR("VFfieldname() failed.");
            }
            map_info->fnames[i] = (char*) HDmalloc(sizeof(char) * 
                                                   strlen(fname)+1);
            if(map_info->fnames[i] == NULL){
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }
            
            strncpy(map_info->fnames[i], fname, strlen(fname));
            map_info->fnames[i][strlen(fname)] = '\0';

            /* Save the field order. */
            forder = VFfieldorder(vdata_id, i);
            if(forder == FAIL){
                FAIL_ERROR("VFfieldorder() failed.");
            }
            else{
                map_info->forders[i] = forder;
            }
            /* Save the type. */
            ftype = VFfieldtype(vdata_id, i);
            if(ftype == FAIL){
                FAIL_ERROR("VFfieldtype() failed.");
            }
            else{
                map_info->ftypes[i] = ftype;
            }
            
        }
    }

    /* This returns 0 on olslit1994.apr_digital_10.hdf /Strip/strip Vdata */
    map_info->nblocks = VSgetdatainfo(vdata_id, 0, 0, NULL, NULL);
    if(map_info->nblocks == FAIL){
        FAIL_ERROR("VSgetdatainfo() failed.");
    }
    if(map_info->nblocks > 0){
        map_info->offsets = 
            (int32 *) HDmalloc(sizeof(int32)*map_info->nblocks);
        if(map_info->offsets == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        map_info->lengths = 
            (int32 *) HDmalloc(sizeof(int32)*map_info->nblocks);
        if(map_info->lengths == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        nblocks = VSgetdatainfo(vdata_id, 0, map_info->nblocks, 
                                map_info->offsets, map_info->lengths);
        if(nblocks == FAIL){
            FAIL_ERROR("VSgetdatainfo() failed.");
        }
        if(nblocks != map_info->nblocks){
            fprintf(flog, "VSgetdatainfo() returned the wrong number of blocks.");
            fprintf(flog, ":expected=%ld, got=%ld\n", (long)map_info->nblocks, 
                    (long)nblocks);
            return FAIL;
        }
    }
    return SUCCEED;
} /* VSmapping */


