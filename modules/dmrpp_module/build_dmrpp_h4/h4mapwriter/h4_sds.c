/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2014  The HDF Group.                                   *
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

 \file h4_sds.c
 \brief Read SDS mapping information.

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
 \date Oct 7, 2014
 \note renamed definition that relies on --enable-netcdf.

 \date  Oct 11, 2013
 \note  made error message consistent in SDmapping() and read_chunk().


 \date  April 28, 2011
 \note removed read_fill_value().


 \date  April 26, 2011
 \note added extern unsigned int ID_D.

 \author Peter Cao (xcao@hdfgroup.org)
 \date  October 12, 2007
 \note created.

*/

#ifdef _MSC_VER
#define snprintf _snprintf
#endif
#include "hmap.h"

extern unsigned int ID_D;

/*!

  \fn SDfree_mapping_info(SD_mapping_info_t  *map_info)
  \brief release memory held mapping infomation

  Given \a map_info mapping information, release the offsets and lengths memory
  block inside \a map_info.

  \return SUCCEED
  \return FAIL

  \author  Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date April 26, 2010
  \note added a line for resetting nblocks to 0.

  \author  Peter Cao (xcao@hdfgroup.org)
  \date August 10, 2007
  \note created.

*/
intn
SDfree_mapping_info(SD_mapping_info_t  *map_info)
{
    intn  ret_value = SUCCEED;

    /* nothing to free */
    if (map_info == NULL) 
        return SUCCEED;

    map_info->nblocks = 0;

    if (map_info->offsets != NULL) {
        HDfree(map_info->offsets);
        map_info->offsets = NULL;
    }

    if (map_info->lengths) {
        HDfree(map_info->lengths);
        map_info->lengths = NULL;
    }

    return ret_value;    
} /* end of SDfree_mapping_info() */


/*!

  \fn SDmapping(int32 sdsid, SD_mapping_info_t *map_info)
  \brief Create mapping information for \a sdsid dataset.

  Given a SDS dataset id, retrieve the offset and length information 
  for mapping file.

  \return SUCCEED, if all information is retrieved successfully.
  \return FAIL, otherwise

  \author  Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note made error message consistent.

  \date April 26, 2010
  \note 
  removed code that resets the entire map_info to preserve fill value and
  is_empty field.

  \author  Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date March 15, 2010
  \note cleaned up error handlers.

  \author  Peter Cao (xcao@hdfgroup.org)
  \date August 10, 2007 


*/
intn
SDmapping(int32 sdsid, SD_mapping_info_t *map_info)
{

    intn  ret_value = SUCCEED;
    uintn info_count = 0;

    /* Reset map_info. */
    SDfree_mapping_info(map_info);
    /* HDmemset(map_info, 0, sizeof(SD_mapping_info_t)); */
    info_count = SDgetdatainfo(sdsid, NULL, 0, 0, NULL, NULL);

    if (info_count == FAIL){
        FAIL_ERROR("SDgetdatainfo() failed.");
    }

    if (info_count > 0)
        {
            map_info->nblocks = (int32) info_count;
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
            /* This fails on test case 17. */
            ret_value = SDgetdatainfo(sdsid, NULL, 0, info_count, 
                                      map_info->offsets, map_info->lengths);
            if (ret_value == FAIL){
                FAIL_ERROR("SDgetdatainfo() failed.");
            }

        }
    return ret_value;
} /* SDmapping */

/*!

  \fn read_chunk(int32 sdsid, SD_mapping_info_t *map_info, int32* index)
  \brief Read offset/length for \a sdsid chunked dataset.

  Given an SDS dataset id, retrieve the offset and length information 
  at \a index chunk index. 

  \return SUCCEED, if all information is retrieved successfully.
  \return FAIL, otherwise

  \author  Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date October 11, 2013
  \note  made error message consistent.

  \date October 19, 2010
  \note created.

*/
intn
read_chunk(int32 sdsid, SD_mapping_info_t *map_info, int32* index)
{

    intn  info_count = 0;
    intn  ret_value = SUCCEED;

    /* Free any info before resetting. */
    SDfree_mapping_info(map_info);
    /* Reset map_info. */
    /* HDmemset(map_info, 0, sizeof(SD_mapping_info_t)); */

    /* Save SDS id since HDmemset reset it. map_info->id will be resued. */
    /* map_info->id = sdsid; */

    info_count = SDgetdatainfo(sdsid, index, 0, 0, NULL, NULL);

    if (info_count == FAIL){
        fprintf(flog, "SDgetedatainfo() failed in read_chunk().\n");
        return FAIL;
    }

    if (info_count > 0) {
        map_info->nblocks = (int32) info_count;
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

        ret_value = SDgetdatainfo(sdsid, index, 0, info_count, 
                                  map_info->offsets, map_info->lengths);
        return ret_value;

    }

    return ret_value;

} /* read_chunk */


/*!

  \fn read_compression(SD_mapping_info_t *map_info)
  \brief Read compression information.

  See SDgetcompinfo() in mfhdf/libsrc/mfsd.c.

  \return SUCCEED, if compression information is retrieved successfully.
  \return FAIL, otherwise

  \author  Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date May 13, 2011
  \note cleaned up code surrounded by \#ifdef SCHEMA.

  \date March 11, 2011
  \note 
  cleaned up error message.
  copied UNMAPPABLE:(reason) into map_info->comp_info.

  \date October 13, 2010
  \note Created.


*/
intn
read_compression(SD_mapping_info_t *map_info)
{
    comp_coder_t comp_type;     /* type of compression */
    comp_info cinfo;            /* compression information */
    intn   ret_value = SUCCEED;

    /* Collect compression info. */
    ret_value = SDgetcompinfo(map_info->id, &comp_type, &cinfo);

    if(ret_value == FAIL){
        FAIL_ERROR("SDgetcompinfo() failed.\n");
    }

    switch(comp_type){          /* Case is sorted in alphabetical order. */

    case COMP_CODE_DEFLATE:     /* GZIP compression */
        {
            if(cinfo.deflate.level < 0 || cinfo.deflate.level > 9){
                fprintf(flog, "ERROR:Invalid deflate level:%d\n", 
                        cinfo.deflate.level);
                return FAIL;
            }

            snprintf(map_info->comp_info, COMP_INFO - 1, 
                    "compressionType=\"deflate\" deflate_level=\"%d\"", 
                    cinfo.deflate.level);
            break;
        }

    case COMP_CODE_NBIT:
        {
            snprintf(map_info->comp_info, COMP_INFO - 1, 
                     "UNMAPPABLE:n-bit compression type is not supported.");
#ifdef SCHEMA
            snprintf(map_info->comp_info, COMP_INFO - 1,
                    "compressionType=\"nbit\" nbit_signExtend=\"%d\" nbit_fillBit=\"%d\" nbit_startBit=\"%d\" nbit_bitLen=\"%d\"",
                    cinfo.nbit.sign_ext, 
                    cinfo.nbit.fill_one, 
                    cinfo.nbit.start_bit, 
                    cinfo.nbit.bit_len);
#endif
            break;
        }


    case COMP_CODE_NONE:        /* Do nothing. */
        map_info->comp_info[0] = '\0';
        break;

    case COMP_CODE_SKPHUFF:
        {
            snprintf(map_info->comp_info, COMP_INFO - 1, 
                     "UNMAPPABLE:adaptive Huffman compression type is not supported.");
            break;
        }

    case COMP_CODE_SZIP:
        {
            snprintf(map_info->comp_info, COMP_INFO - 1,  
                     "UNMAPPABLE:szip compression type is not supported.");
            break;
        }

    default:                    /* Verify other methods later. */
        map_info->comp_info[0] = '\0';
        fprintf(flog, "ERROR:Unknown compression type.\n");
        ret_value = FAIL;
        break;
    }
    return ret_value;
}


/*!
  \fn set_dimension_no_data(int32 sdsid, int32 rank)

  \brief Write dimension with no data.

  Save dimension data for named dimensions so that they will be printed
  at the end of the map. They may have attributes.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date April 26, 2011
  \note 
  removed useless arguments. 
  renamed to set_dimension_no_data().

  \date March 11, 2011
  \note 
  replaced return 1 with return SUCCEED.
  cleaned up error handling. 

  \date August 11, 2010
  \note created.

  \see write_dimension()

*/
intn 
set_dimension_no_data(int32 sds_id, int32 rank)
{

    char   sdsdim_name[H4_MAX_NC_NAME]; /* SDS dimensional name */

    int32  sdsdim_id = -1; /* SDS dimensional scale id */
    int32  sdsdim_type = 0; /* SDS dimensional scale data type */
    int32  sdsdim_nattrs; /* number of SDS dimensional scale attributes */
    int32  sdsdim_size; /* the size of SDS dimensional scale */
    int32  stat;   /* the "return" status of HDF4 APIs */

    int j=0;
    int id = 0;

    for (j=0; j < rank; j++) {

        sdsdim_id = SDgetdimid(sds_id, j);	
        if(sdsdim_id == FAIL) {
            FAIL_ERROR("SDgetdimid() failed.");
        }
    

        /* Get the information of this dimensional scale. */
        stat = SDdiminfo(sdsdim_id, sdsdim_name, &sdsdim_size,
                         &sdsdim_type, &sdsdim_nattrs);  
        if (stat == FAIL) {
            FAIL_ERROR("SDdiminfo() failed.");            
        }
        /* Check if sdsdim_type is same as 0. 0 means no data is involved. */
         if(sdsdim_type == 0 && 
           (sdsdim_nattrs > 0 || 
            strncmp(sdsdim_name, "fakeDim", 7))){
            /* Check if the dimension name is seen or not. */
            id = get_dn2id_list_id(sdsdim_name);
            if(id == 0){
                id = ++ID_D;
                set_dn2id_list_id(sdsdim_name, id);
                /* Save dimension id into list. Write it at the end. */
                set_dn_list(sdsdim_id);
            }
        }

    } 
    return SUCCEED;
}
