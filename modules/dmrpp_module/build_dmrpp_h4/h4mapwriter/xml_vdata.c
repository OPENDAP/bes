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
  \file xml_vdata.c
  \brief Write Vdata information.
  
  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date October 11, 2013
  \note added ref informatoin for error message.
  \note	made error message consistent.

  \date October 14, 2010
  \note revised for full release.

  \author Peter Cao (xcao@hdfgroup.org)
  \date August 23, 2007
  
*/

#include "hmap.h"



/*!

 \fn write_VSattrs(FILE *ofptr, int32 id, int32 nattrs, intn indent)
 \brief Write all Vdata attributes.

 \return FAIL if any HDF4 calls fail.
 \return SUCCEED otherwise

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

 \date February 7, 2012
 \note 	filtered reserved XML character	from the attribute name.

 \date June 13, 2011
 \note fixed dropping MSB in nt_type.

*/
intn write_VSattrs(FILE *ofptr, int32 id, int32 nattrs, intn indent)
{
    static unsigned int ID_TA = 0; /* table attribute id */
    
    VOIDP attr_buf = NULL;

    char attr_name[H4_MAX_NC_NAME];
    char *attr_nt_desc = NULL;
    char *name_xml = NULL;

    int32 attr_buf_size = 0;
    int32 attr_count = 0;
    int32 attr_index = 0;
    int32 attr_nt = 0;
    int32 attr_nt_no_endian = 0;
    int32 attr_size = 0;
    int32 length = 0;
    int32 offset = -1;


    intn status  = SUCCEED;
    

    /* For each file attribute, print its info and values. */
    for (attr_index = 0; attr_index < nattrs; attr_index++)
        {
            status = VSattrinfo(id, -1, attr_index, attr_name, &attr_nt, 
                                &attr_count, &attr_size);
            
            if( status == FAIL )
                continue;

            status = VSgetattdatainfo(id, _HDF_VDATA, attr_index,
                                      &offset, &length);
            if( status == FAIL)
                fprintf(flog, "VSgetattdatainfo returned FAIL.\n");            
            
      
            /* Get number type description of the attribute */
            attr_nt_desc = HDgetNTdesc(attr_nt);
            attr_buf_size = DFKNTsize(attr_nt) * attr_count;
            if (attr_nt_desc == NULL || attr_buf_size <= 0)
                continue;

            attr_nt_no_endian = (attr_nt & 0xff);
            if (attr_nt_no_endian == DFNT_CHAR || 
                attr_nt_no_endian == DFNT_UCHAR)
                attr_buf = (char *)HDmalloc(attr_buf_size+1);
            else
                attr_buf = (VOIDP)HDmalloc(attr_buf_size);

            status = VSgetattr(id, -1, attr_index, attr_buf);
            if( status == FAIL ) {
                HDfree (attr_buf);
                HDfree (attr_nt_desc);
                continue;
            }

            start_elm(ofptr, TAG_TATTR, indent);
            name_xml =  get_string_xml(attr_name, strlen(attr_name));
            if(name_xml != NULL){
                fprintf(ofptr, "name=\"%s\" id=\"ID_TA%d\">", name_xml, 
                        ++ID_TA);
                HDfree(name_xml);
            }
            else{
                fprintf(flog, "get_string_xml() returned NULL.\n");
                status = FAIL;
            }
            write_array_attribute(ofptr, attr_nt, attr_count, attr_buf, 
                                  attr_buf_size, offset, length, indent+1);
            end_elm(ofptr, TAG_TATTR, indent);

            HDfree(attr_buf);
            HDfree (attr_nt_desc);
        }  /* for each file attribute */
    return( status );
}   /* end of write_VDattrs */

/*! 

  \fn write_column_attr(FILE* ofptr, int32 vid, int32 findex,  intn indent)

  \brief Write table column attribute.

  \return FAIL if any HDF4 calls fail.
  \return SUCCEED otherwise

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note made error message consistent.

  \date February 7, 2012
  \note filtered reserved XML character	from the attribute name.

  \date June 13, 2011
  \note fixed dropping MSB in nt_type.

 */

intn write_column_attr(FILE* ofptr, int32 vid, int32 findex, intn indent)
{

    static unsigned int ID_CA = 0; /* table attribute id */

    VOIDP attr_buf = NULL;

    char  attr_name[H4_MAX_NC_NAME];
    char  *attr_nt_desc = NULL;
    char  *name_xml = NULL;

    int i = 0;

    int32 attr_buf_size = 0;
    int32 attr_count = 0;
    int32 attr_nt = 0;
    int32 attr_nt_no_endian = 0;
    int32 attr_size = 0;
    int32 length = 0;
    int32 offset = -1;
    int32 num_of_attrs = 0;

    intn  status = SUCCEED;          /* status from a called routine */

    /* Call VSfnattrs to determine the number of field attributes. */
    num_of_attrs = VSfnattrs(vid, findex);
    if(num_of_attrs == FAIL){
        fprintf(flog, "VSfnattrs() failed on vid=%ld and findex=%ld.\n",
                (long)vid, (long)findex);
        return FAIL;
    }

    if(num_of_attrs > 0){
        for(i = 0; i < num_of_attrs; i++){
            status = VSattrinfo(vid, findex, i, attr_name, &attr_nt, 
                                &attr_count, &attr_size);
            if( status == FAIL )
                continue;
            status = VSgetattdatainfo(vid, findex, i, &offset, &length);

            /* Get number type description of the attribute. */
            attr_nt_desc = HDgetNTdesc(attr_nt);
            attr_buf_size = DFKNTsize(attr_nt) * attr_count;
            if (attr_nt_desc == NULL || attr_buf_size <= 0)
                continue;

            attr_nt_no_endian = (attr_nt & 0xff);
            if (attr_nt_no_endian == DFNT_CHAR || 
                attr_nt_no_endian == DFNT_UCHAR)
                attr_buf = (char *)HDmalloc(attr_buf_size+1);
            else
                attr_buf = (VOIDP)HDmalloc(attr_buf_size);

            if(attr_buf == NULL){
                HDfree(attr_nt_desc);
                FAIL_ERROR("HDmalloc(): Out of Memory.");
            }

            status = VSgetattr(vid, findex, i, attr_buf);
            if( status == FAIL ) {
                fprintf(flog, "VSgetattr() failed.\n");
                HDfree(attr_buf);
                HDfree(attr_nt_desc);
                continue;
            }

            start_elm(ofptr, TAG_CATTR, indent);
            name_xml = get_string_xml(attr_name, strlen(attr_name));
            if(name_xml != NULL){
                fprintf(ofptr, "name=\"%s\" id=\"ID_CA%d\">", name_xml, 
                        ++ID_CA);
                HDfree(name_xml);
            }
            else{
                fprintf(flog, "get_string_xml() returned NULL.\n");
                status = FAIL;
            }
            write_array_attribute(ofptr, attr_nt, attr_count, attr_buf, 
                                  attr_buf_size, offset, length, indent+1);
            end_elm(ofptr, TAG_CATTR, indent);

            HDfree(attr_buf);
            HDfree(attr_nt_desc);
        }
    }
    return status;
}

/*!
  
  \fn write_map_lone_vdata(FILE *ofptr, int32 file_id, ref_count_t *vs_visited)
  \brief Write mapping information for all vdatas.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date May 3, 2011  
  \note increased the indentation.

  \date April 27, 2011  
  \note filtered out tables that are visited already.

  \date March 15, 2011
  \note
  changed return type.
  cleaned up error messages.
  added more error handlers.
  removed local num_errs variable.

 */
intn write_map_lone_vdata(FILE *ofptr, int32 file_id, ref_count_t *vs_visited)
{
    VS_mapping_info_t map_info; /* mapping information */
    
    int i = 0;

    int32 num_vdata = 0;        /* number of SDS in file */
    int32 *vdata_refs= NULL;
    int32 vdata_id = 0;         /* vdata id */

    intn status = SUCCEED;      /* status flag */


    num_vdata = VSlone(file_id, NULL, 0);
    if (num_vdata == FAIL){
        FAIL_ERROR("VSlone() failed.");
    }
    else if (num_vdata == 0)
        return SUCCEED;

    vdata_refs = (int32 *)HDmalloc(num_vdata*sizeof(int32));
    if(vdata_refs == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");        
    }
    
    num_vdata = VSlone(file_id, vdata_refs, num_vdata);
    if (num_vdata == FAIL){
        HDfree(vdata_refs);
        FAIL_ERROR("VSlone() failed.");
    }    

    /* Read mapping information for Vdata. */
    for (i=0; i < num_vdata; i++) {
        /* Filter out tables that are visited already. */
        if(vdata_refs[i] != ref_count(vs_visited, vdata_refs[i])){

            vdata_id = VSattach(file_id, vdata_refs[i], "r");
            if(vdata_id == FAIL){
                HDfree(vdata_refs);
                FAIL_ERROR("VSattach() failed.");
            }
            HDmemset(&map_info, 0, sizeof(map_info));
            status = write_map_vdata(ofptr, vdata_id, "/", &map_info, 2, 
                                     vs_visited);
            if(status == FAIL){
                fprintf(flog,  "write_map_vdata() failed.");
                fprintf(flog,  ":path=/ name=%s\n", map_info.name);
                HDfree(vdata_refs);
                VSfree_mapping_info(&map_info);
                status = VSdetach(vdata_id);
                if(status == FAIL){
                    FAIL_ERROR("VSdetach() failed.");
                }
                return FAIL;
            }

            VSfree_mapping_info(&map_info);
            status = VSdetach(vdata_id);
            if(status == FAIL){
                HDfree(vdata_refs);
                FAIL_ERROR("VSdetach() failed.");
            }
        } /* if not visited */
    } /* for */
    HDfree(vdata_refs);
    return status;
}


/*! 

  \fn write_map_vdata(FILE *ofptr, int32 vdata_id, const char *path,
                      VS_mapping_info_t *map_info, int indent,
                      ref_count_t *vs_visited)

  \brief Write Vdata information to map file.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Aug 2, 2016
  \note	removed ISO C90 warning.

  \date Oct 11, 2013
  \note	added ref information for error message.

  \date February 3, 2012
  \note	filtered reserved XML character	from the field name.

  \date June 30, 2011
  \note	skipped generating tableData element and verification values if
  no data is involved.

  \date April 27, 2011
  \note allowed a table to appear twice under different groups.

  \date March 16, 2011
  \note 
  moved handled 0 row case.

  \date March 15, 2011
  \note 
  changed return type.
  removed num_errs local variable.
  added more error handlers.

  \date March 14, 2011
  \note redirected unmppable object to ferr.

  \date March 10, 2011
  \note checked column storage order at the beginning to return error
  if column storage is used.

 */
intn write_map_vdata(FILE *ofptr, int32 vdata_id, const char *path,
                    VS_mapping_info_t *map_info, int indent,
                    ref_count_t *vs_visited)
{

    int32 i=0;
    int32 ref=0;                /* reference id of vdata */
    int32 type = 0;             /* column(field) type */
    int32 num_attrs = 0;        /* number of attributes */
    intn  status = SUCCEED;     /* status flag */
    char* name_xml = NULL;
    char* name_class = NULL;
    
    static unsigned int ID_C = 0; /* coumn(field) XML ID */
    static unsigned int ID_T = 0; /* table XML ID */

    ref = VSQueryref(vdata_id);
    if(ref == FAIL){
        FAIL_ERROR("VSQueryref() failed.");
    }

    /* Save the reference to avoid generating the same table element
       in write_map_lone_vdata().  */
    ref_count(vs_visited, ref);

    status = VSmapping(vdata_id, map_info);
    if(status == FAIL){
        fprintf(flog, "VSmapping() failed");
        fprintf(flog, ":vdata_id=%ld\n, ref=%ld.", (long)vdata_id, (long)ref);
        return status;
    }

    if (map_info->is_old_attr == 1) {
        return SUCCEED;
    }

    if(map_info->sorder == 1){ /* By column */
        fprintf(ferr, "UNMAPPABLE:");
        fprintf(ferr, "Column storage order is not supported.");
        fprintf(ferr, ":path=%s, name=%s\n", path, map_info->name);
        ++num_errs;
        return SUCCEED;         /* Skip generating map. */
    }

    if (map_info->nblocks < 0) {
        fprintf(flog, "VSmapping() returned the wrong number of blocks.");
        fprintf(flog, ":nblocks=%ld\n", (long)map_info->nblocks);
        return FAIL;
    }

    start_elm(ofptr, TAG_VS, indent);

    /* Clean up Table name and class string so that it doesn't contain 
       reserved XML characters. */
    name_xml =  get_string_xml(map_info->name, strlen(map_info->name));
    if(name_xml != NULL){
        fprintf(ofptr, "name=\"%s\" path=\"%s\"", name_xml, path);
        HDfree(name_xml);
    }
    else {
        fprintf(flog, "get_string_xml() returned NULL.\n");
        return FAIL;
    }

    /* Add an optional class attribute if any. */
    if(HDstrlen(map_info->class) > 0){
        name_class =  get_string_xml(map_info->class, 
                                     strlen(map_info->class));
        if(name_class != NULL){
            fprintf(ofptr, " class=\"%s\"", name_class);
            HDfree(name_class);
        }
        else {
            fprintf(flog, 
                    "get_string_xml() returned NULL.\n");
            return FAIL;
        }
    }

    fprintf(ofptr,
            " nRows=\"%d\" nColumns=\"%d\" id=\"ID_T%d\">", 
            (int)map_info->nrows, 
            map_info->ncols,
            ++ID_T);
    
    /* Write vdata table attribute. */
    num_attrs = (int32)VSnattrs(vdata_id);

    if(num_attrs == FAIL){
        FAIL_ERROR("VSnattrs() failed.");
    }

    if (num_attrs > 0 ) {
        write_VSattrs(ofptr, vdata_id, num_attrs, indent+1);
    }

    /* Write fields information. */
    for (i=0; i < map_info->ncols; i++){
        start_elm(ofptr, TAG_FIELD, indent+1);
        name_xml =  
            get_string_xml(map_info->fnames[i], strlen(map_info->fnames[i]));
        if(name_xml != NULL){
            /* nEntries is same as field order. */
            fprintf(ofptr, "name=\"%s\" nEntries=\"%ld\" id=\"ID_C%d\"",
                    name_xml, 
                    (long)map_info->forders[i],  
                    ++ID_C);        
            fputs(">", ofptr);
            HDfree(name_xml);
        }
        else {
            fprintf(flog, "get_string_xml() returned NULL.\n");
            return FAIL;
        }

        type = map_info->ftypes[i];
        if (type == DFNT_CHAR || 
            type == DFNT_UCHAR){
            write_map_datum_char(ofptr, type, indent+2);
        }
        else{
            write_map_datum(ofptr, type, indent+2);
        }
                   
        /* Write column attribute. */
        status = write_column_attr(ofptr, vdata_id, i, indent+2);
        if(status == FAIL){
            fprintf(flog,
                    "write_column_attr() failed.");
            fprintf(flog, ":name=%s, field=%s, vid=%ld, col=%ld\n",
                    map_info->name,
                    map_info->fnames[i], 
                    (long)vdata_id, (long)i);
            return FAIL;
        }
        end_elm(ofptr, TAG_FIELD, indent+1);
    }

    /* Write offset/size information of data blocks. */
    if (map_info->nblocks > 0) {
        
        if(map_info->nrows == 1 || 
           map_info->ncols == 1){
            
            /*
              If row is 1 or column is 1, you can skip generating
              "storageOrder".
            */
            
            start_elm_0(ofptr, TAG_TDATA, indent+1);
        }
        else{
            start_elm(ofptr, TAG_TDATA, indent+1);
            if(map_info->sorder == 1){ /* By column */
                fprintf(ofptr, "storageOrder=\"by column\">");

            }
            else{               /* By row */
                fprintf(ofptr, "storageOrder=\"by row\">");
            }

        }
        write_byte_streams(ofptr, map_info->nblocks, map_info->offsets, 
                          map_info->lengths, indent+2);
        end_elm(ofptr, TAG_TDATA, indent+1);
        /* Write verify table values. */
        status = write_verify_table_values(ofptr, vdata_id, map_info, 
                                           indent+1);
        if(status == FAIL){
            fprintf(flog,
                    "write_verify_table_values() failed.");
            fprintf(flog, ":name=%s, vid=%ld\n",
                    map_info->name,
                    (long)vdata_id);
            return FAIL;

        }


    }
    end_elm(ofptr, TAG_VS, indent);
    return status;
}


/*!

 \fn write_verify_table_values(FILE *ofptr, int32 id,
                               VS_mapping_info_t *map_info, intn indent)

 \brief Write the first and last row values.

 \return FAIL
 \return SUCCEED

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
 \date March 16, 2011
 \note 
 moved the comment block outside if() condition for 0 row case.

 \param[in] ofptr map file pointer
 \param[in] id Vdata id
 \param[in] map_info pointer to mapped info
 \param[in] indent indentation level
*/
intn 
write_verify_table_values(FILE *ofptr, int32 id, VS_mapping_info_t *map_info,
                          intn indent)
{
    intn status = SUCCEED;

    start_elm(ofptr, "!--", indent);
    fprintf(ofptr, "row(s) for verification; csv format");

    if(map_info->nrows > 0){
        
        status = write_verify_table_values_row(ofptr, id, map_info, 0, indent);

        /* Move to the last row. */
        if(map_info->nrows > 1){
            status = write_verify_table_values_row(ofptr, id, map_info,
                                                   map_info->nrows-1, indent);
        }
    }
    indentation(ofptr, indent);
    fprintf(ofptr, "-->");

    return status;
}

/*!

  \fn write_verify_table_values_field(FILE *ofp, int32 nt, int32 cnt, 
  VOIDP databuf, intn indent)

  \brief Write field values.
  \see write_attr_value().

*/
void 
write_verify_table_values_field(FILE *ofp, int32 nt, int32 cnt, VOIDP databuf, 
                                intn indent)
{
    intn i = 0;

    if( NULL == databuf || NULL == ofp)
        return;

    switch (nt & 0xff )
    {
        case DFNT_CHAR:
        case DFNT_UCHAR: 
            /* You can't use get_string_xml() since each value must be
             delimited by space character. The decision was made during
            teleconference. */
            for (i=0; i < cnt; i++) {
                if(isprint(((char *)databuf)[i])){
                    fprintf(ofp, "%c", ((char *)databuf)[i]);
                }
                else {
                    fprintf(ofp, " ");
                }
                if(i != cnt-1){ /* Put space between value. */
                    fprintf(ofp, " ");
                }

            }
            break;
        case DFNT_UINT8:
            for (i=0; i < cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%d ", ((uint8 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%d", ((uint8 *)databuf)[i]);
                }
            }
            break;
        case DFNT_INT8:
            for (i=0; i < cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%d ", ((int8 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%d", ((int8 *)databuf)[i]);
                }
            }
            break;
        case DFNT_UINT16:
            for (i=0; i < cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%u ", ((uint16 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%u", ((uint16 *)databuf)[i]);
                }
            }
            break;
        case DFNT_INT16:
            for (i=0; i < cnt; i++) {
                if(i != cnt-1){
                      fprintf(ofp, "%d ", ((int16 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%d", ((int16 *)databuf)[i]);

                }
            }
            break;
        case DFNT_UINT32:
            for (i=0; i < cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%lu ", (long)((uint32 *)databuf)[i]);
                }
                else {
                    fprintf(ofp, "%lu", (long)((uint32 *)databuf)[i]);
                }
            }
            break;
        case DFNT_INT32:
            for (i=0; i < cnt; i++) {
                /* The (long) casting is required for 64-bit machine. */
                if(i != cnt-1){
                    fprintf(ofp, "%ld ", (long)((int32 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%ld",  (long)((int32 *)databuf)[i]);
                }


            }
            break;
        case DFNT_FLOAT32:
            for (i=0; i < cnt; i++) {
                if(i != cnt-1){
                    fprintf(ofp, "%f ", ((float32 *)databuf)[i]);
                }
                else{
                    fprintf(ofp, "%f", ((float32 *)databuf)[i]);
                }
            }
            break;
        case DFNT_FLOAT64:
            for (i=0; i < cnt; i++) {
                if( i!= cnt-1){
                    fprintf(ofp, "%lf ", ((float64 *)databuf)[i]);
                }
                else {
                    fprintf(ofp, "%lf", ((float64 *)databuf)[i]);
                }
            }
            break;
        default:
            break;
   }
}


/*!

  \fn write_verify_table_values_row(FILE *ofptr, int32 id,
  VS_mapping_info_t *map_info, int32 pos,
  intn indent)

  \brief Write the \a pos row values.

  \param[in] ofptr map file pointer
  \param[in] id Vdata id
  \param[in] map_info pointer to mapped info
  \param[in] pos row position
  \param[in] indent indentation level

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date August 2, 2016
  \note removed ISO C90 warning.

  \date February 7, 2012
  \note 
  Although there is no need to clean a variable name within a comment
  block, Ruth Aydt wants to make it look consistent with the filtered
  variable name in the non-comment block. Thus, I filtered the variable
  name in the verification values comment block.

  \date Mar 15, 2011
  \note 
  cleaned up error messages.


*/
intn 
write_verify_table_values_row(FILE *ofptr, int32 id,
                              VS_mapping_info_t *map_info,
                              int32 pos,
                              intn indent)
{
    char* name_xml = NULL;
    
    int i = 0;
    
    int32 num_of_records = 0;
    int32 eltsz = 0;
    int32 size = 0;
    
    intn status = SUCCEED;
    
    uint8* databuf = NULL;

    indentation(ofptr, indent+1);

    name_xml =  get_string_xml(map_info->name, strlen(map_info->name));
    if(name_xml != NULL){
        fprintf(ofptr, "%s[%ld]=", name_xml, (long)pos);
        HDfree(name_xml);
    }
    else{
        fprintf(flog, "get_string_xml() returned NULL.\n");
        return FAIL;
    }
        
    for (i=0; i < map_info->ncols; i++){
        num_of_records = 0;
        eltsz = DFKNTsize(map_info->ftypes[i]
                                | DFNT_NATIVE);
        size = map_info->forders[i] *
            eltsz;
        databuf = HDmalloc(size * sizeof(uint8));

        if(databuf == NULL) {
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        /* Set the column to be read. */
        if(VSsetfields(id, map_info->fnames[i]) == FAIL) {
            HDfree(databuf);
            FAIL_ERROR("VSsetfields() failed.");            
            return FAIL;
        }

        status = VSseek(id, pos); /* Rewind file position every time. */
        if(status == FAIL){
            HDfree(databuf);
            FAIL_ERROR("VSseek() failed.");
        }

        num_of_records = VSread(id, (uint8*)databuf, 1, 
                                map_info->sorder); 


        if(num_of_records == FAIL){
            HDfree(databuf);
            FAIL_ERROR("VSread() failed.");
        }

        /* Wrap the values in quote if a field has multiple values. */
        if(map_info->forders[i] > 1){
            fprintf(ofptr, "\"");
        }

        write_verify_table_values_field(ofptr, map_info->ftypes[i],
                                        map_info->forders[i],
                                        databuf, indent);
        if(map_info->forders[i] > 1){
            fprintf(ofptr, "\"");
        }            
        if(i != map_info->ncols - 1){
            fprintf(ofptr, ",");
        }

        if(databuf != NULL)
            HDfree(databuf);
            
    }
    return SUCCEED;
            
}

