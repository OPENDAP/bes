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
  \file xml_sds.c
  \brief Write SDS information.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Aug 2, 2016
  \note added get_consistent_metadata_name().

  \date Jul 27, 2016
  \note added global parse_odl variable.

  \date Jul 20, 2016
  \note added global uuid variable.

  \date Oct 7, 2014
  \note renamed definition that relies on --enable-netcdf.

  \date Oct 11, 2013
  \note added ref/tag informatoin for error message.
  \note made error message consistent.

  \date August 19, 2010
  \note revised for full release.

  \author Peter Cao (xcao@hdfgroup.org)
  \date August 23, 2007

*/

#include <stdlib.h>
#include <time.h>               /* For pseudo random number generator. */
#include <string.h>

#include "hmap.h"
#include "xml_sds.h"

void *odl_string(const char *yy_str); /* Glue routine declared in odl.lex */
struct yy_buffer_state;
struct yy_buffer_state *odl_scan_string(char *str);
extern int odlparse(FILE *arg);

extern unsigned int ID_FA;   
extern mattr_list_ptr list_mattr;


int trim = 0;                   /*!< Trim extra NULL characters. */
int merge = 0;                  /*!< Merge  SDS file attributes. */
int parse_odl = 0;              /*!< Parse ODL contents.  */

/*
  
  The following variables must be global so that the order of processing 
dimension variables and array variables doesn't matter.

*/

cv_list_ptr cv_list = NULL;  /*!< Coordinate variable SDS id list.  */
did2sid_list_ptr did2sid_list = NULL; /*!< Dimension id to SDS id.  */
dn_list_ptr dn_list = NULL; /*!< Dimension names with no data SDS id list.*/
dn2id_list_ptr dn2id_list = NULL; /*!< Dimension name to XML ref id.  */


unsigned int ID_D = 0;   /*!< Unique dimension ID.  */


/*!

  \fn free_cv_list()
  \brief  Free coordinate variable list memory.

 */
void 
free_cv_list()
{
    cv_list_ptr p = cv_list;
    while(p != NULL){

        cv_list_ptr temp = p->next;
        HDfree(p);
        p = temp;
    }
}

/*!

  \fn free_did2sid_list()
  \brief  Free dimension id to SDS id list memory.

 */
void 
free_did2sid_list()
{
    did2sid_list_ptr p = did2sid_list;
    while(p != NULL){

        did2sid_list_ptr temp = p->next;
        HDfree(p);
        p = temp;
    }
}

/*!

  \fn free_dn_list()
  \brief  Free dimension name list memory.

 */
void 
free_dn_list()
{
    dn_list_ptr p = dn_list;
    while(p != NULL){

        dn_list_ptr temp = p->next;
        HDfree(p);
        p = temp;
    }
}

/*!

  \fn free_dn2id_list()
  \brief  Free dimension name to XML id list memory.

 */
void 
free_dn2id_list()
{
    dn2id_list_ptr p = dn2id_list;
    while(p != NULL){

        dn2id_list_ptr temp = p->next;
        HDfree(p);
        p = temp;
    }
}

/*!

  \fn get_adjusted_attr_datainfo(char* name, int32* a_offset, int32* a_length)
  \brief  Get the SDS id of \a did dimension id.

  Some pre-defined attributes are stored in sequence off by fixed numbers.
  Instead of defining several APIs for obtaining offsets, the writer computes
  them here.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date January 21, 2011
  \note created the function.

 */
intn 
get_adjusted_attr_datainfo(char* name, int32* offset, int32* length)
{
    if(!strncmp(name, "valid_max", 9)){
        *length = 4;
    }

    if(!strncmp(name, "valid_min", 9)){
        *offset += 4;
        *length = 4;
    }

    if(!strncmp(name, "scale_factor", 12)){
        *length = 8;
    }

    if(!strncmp(name, "scale_factor_err", 16)){
        *offset += 8;
        *length = 8;
    }

    if(!strncmp(name, "add_offset", 10)){
        *offset += 16;
        *length = 8;
    }

    if(!strncmp(name, "add_offset_err", 14)){
        *offset += 24;
        *length = 8;
    }

    if(!strncmp(name, "calibrated_nt", 14)){
        *offset += 32;
        *length = 4;
    }
    return SUCCEED;
}

/*! 
  \fn get_consistent_metadata_name(char* orig_name, char* new_name)
  \brief  Gets the consistent name of \a orig_name.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Aug 2, 2016
  \note added.
 */

int
get_consistent_metadata_name(char* orig_name, char* new_name)
{
    /* Allocate memory. */
    if (strncmp(orig_name, "StructMetadata", 14) == 0) {
        strncpy(new_name, "structmetadata", 15);
        return SUCCEED;        
    }

    if (strncmp(orig_name, "CoreMetadata", 12) == 0) {
        strncpy(new_name, "coremetadata", 13);
        return SUCCEED;        
    }

    if (strncmp(orig_name, "coremetadata", 12) == 0) {
        strncpy(new_name, "coremetadata", 13);
        return SUCCEED;        
    }

    if (strncmp(orig_name, "ArchiveMetadata", 15) == 0) {
        strncpy(new_name, "archivemetadata", 16);
        return SUCCEED;        
    }

    if (strncmp(orig_name, "archivemetadata", 15) == 0) {
        strncpy(new_name, "archivemetadata", 16);
        return SUCCEED;        
    }
    
    if (strncmp(orig_name, "ProductMetadata", 15) == 0) {
        strncpy(new_name, "productmetadata", 16);
        return SUCCEED;        
    }

    if (strncmp(orig_name, "productmetadata", 15) == 0) {
        strncpy(new_name, "productmetadata", 16);
        return SUCCEED;        
    }
    
    return FAIL;
}
/*!

  \fn get_did2sid_list(int32 did)
  \brief  Gets the SDS id of \a did dimension id.

 */
int32 
get_did2sid_list(int32 did)
{

    did2sid_list_ptr p = did2sid_list;

    while(p != NULL){

        if(p->elmt.did == did){
            return p->elmt.sid;
        }
        p = p->next;
    }
    return 0;

}

/*!

  \fn get_dn2id_list_id(char* name)
  \brief  Gets the XML element id of \a name dimension name.

 */
unsigned int 
get_dn2id_list_id(char* name)
{

    dn2id_list_ptr p = dn2id_list;

    if (name == NULL){
        return 0;
    }

    while(p != NULL){

        if(!strncmp(name, p->elmt.name, strlen(name))){
            return p->elmt.id;
        }
        p = p->next;
    }
    return 0;

}

/*!
  \fn get_dn2id_list_written(char* name)

  \brief Check if \a name dimension name is already written in the map file.

  \return 0, if \a name is not already written.
  \return 1, otherwise.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date September 29, 2010
  \see set_dn2id_list_written()

*/
unsigned int 
get_dn2id_list_written(char* name)
{

    dn2id_list_ptr p = dn2id_list;

    while(p != NULL){

        if(!strncmp(name, p->elmt.name, strlen(name))){
            return p->elmt.written;
        }
        p = p->next;
    }
    return 0;

}

/*!
  \fn get_trimmed_xml(char *s, int* trimmed, int count)
  \brief Trim NULL characters at the end from \a s string.

  \param[in] s source character buffer.
  \param[out] trimmed number of null characters trimmed at the end.
  \param[in] count total number of characters in \a s.
  \return pointer to escaped string character buffer.


  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \see get_string_xml()

  \date Aug 31, 2010
 */
char* 
get_trimmed_xml(char *s, int* trimmed, int count)
{
    int i = 0;
    int found = 0;

    if(trim == 1){
        /* First, count the number of extra characters. */
        for (i = count-1; i >= 0 && (found == 0); i--){
            if (s[i] == '\0'){
                ++(*trimmed);
            }
            else{
                found = 1;
            }
        }
    }
    return get_string_xml(s, count - *trimmed);
}

/*!
  \fn is_diff(VOIDP buf1, VOIDP buf2, int32 data_type)
  \brief  Checks if \a buf1 value is same as \a buf2 value.

  This function is adapted from array_diff() in hdiff_array.c of hdiff
  program.
  
  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date May 29, 2011
  \note return -1 when type is not expected.

  \return 0 if values are same.
  \return 1 otherwise.
  \return -1 if error.

*/
int
is_diff(VOIDP buf1, VOIDP buf2, int32 type)
{
    int result = 0;
    
    int8    *i1ptr1, *i1ptr2;
    int16   *i2ptr1, *i2ptr2;
    int32   *i4ptr1, *i4ptr2;
    float32 *fptr1, *fptr2;
    float64 *dptr1, *dptr2;

     switch(type) {
         
     case DFNT_INT8:
     case DFNT_UINT8:
     case DFNT_UCHAR8:
     case DFNT_CHAR8:
         i1ptr1 = (int8 *) buf1;
         i1ptr2 = (int8 *) buf2;
         if(*i1ptr1 != *i1ptr2){
             result = 1;
         }
         break;
         
     case DFNT_INT16:
     case DFNT_UINT16:
         i2ptr1 = (int16 *) buf1;
         i2ptr2 = (int16 *) buf2;
         if(*i2ptr1 != *i2ptr2){
             result = 1;
         }
         break;
     case DFNT_INT32:
     case DFNT_UINT32:
         i4ptr1 = (int32 *) buf1;
         i4ptr2 = (int32 *) buf2;
         if(*i4ptr1 != *i4ptr2){
             result = 1;
         }
         break;         
     case DFNT_FLOAT:
         fptr1 = (float32 *) buf1;
         fptr2 = (float32 *) buf2;
         if(*fptr1 != *fptr2){
             result = 1;
         }
         break;
     case DFNT_DOUBLE:
         dptr1 = (float64 *) buf1;
         dptr2 = (float64 *) buf2;
         if(*dptr1 != *dptr2){
             result = 1;
         }
         break;
     default:
         result = -1;
         break;
     
     }
    return result;
}


/*!
  \fn set_cv_list(int32 sdsid)
  \brief  Add \a sdsid into coordinate variable list.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note made error message consistent.

  \return size of the current list if successful
  \return 0 otherwise

*/
unsigned int 
set_cv_list(int32 sdsid)
{
    unsigned int count = 1;
    cv_list_ptr p = NULL;
    cv_list_ptr tail = NULL;

    p = (cv_list_ptr) HDmalloc(sizeof(cv_list_elmt)); 

    if (p != NULL) { 
        p->id = sdsid; 
        p->next = NULL; 
    } 
    else {
        fprintf(flog, "HDmalloc() failed: Out of Memory.\n");
        return 0;
    }

    if(cv_list == NULL){
        cv_list = p;
    }
    else{
        tail = cv_list;
        while(tail->next != NULL){
            ++count;
            tail = tail->next;
        }
        tail->next = p;
    }
    return count;
}

/*!
  \fn set_did2sid_list(int32 did, int32 sid)
  \brief  Associate \a did dimension id with \a sid SDS id and store them
  into the global linked list.

  \note It doesn't check the uniqueness of the \a did in the list.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note made error message consistent.

*/
void 
set_did2sid_list(int32 did, int32 sid)
{

    did2sid_list_ptr p = NULL;
    did2sid_list_ptr tail = NULL;

    p = (did2sid_list_ptr) HDmalloc(sizeof(did2sid_list_elmt)); 

    if (p != NULL) { 
        p->elmt.did = did; 
        p->elmt.sid = sid;
        p->next = NULL; 
    } 
    else {
        fprintf(flog, "HDmalloc() failed: Out of Memory\n");
    }

    if(did2sid_list == NULL){
        did2sid_list = p;
    }
    else{
        tail = did2sid_list;
        while(tail->next != NULL){
            tail = tail->next;
        }
        tail->next = p;
    }
}


/*!
  \fn set_dn_list(int32 id)
  \brief  Add \a SDS dimension id into the global dimension variable id list.

  \return size of the current list if successful
  \return 0 otherwise

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note made error message consistent.
*/
unsigned int 
set_dn_list(int32 id)
{
    unsigned int count = 1;
    dn_list_ptr p = NULL;
    dn_list_ptr tail = NULL;

    p = (dn_list_ptr) HDmalloc(sizeof(dn_list_elmt)); 

    if (p != NULL) { 
        p->id = id; 
        p->next = NULL; 
    } 
    else {
        fprintf(flog, "HDmalloc() failed: Out of Memory\n");
        return 0;
    }

    if(dn_list == NULL){
        dn_list = p;
    }
    else{
        tail = dn_list;
        while(tail->next != NULL){
            ++count;
            tail = tail->next;
        }
        tail->next = p;
    }
    return count;
}

/*!
  \fn set_dn2id_list_id(char* name, unsigned int id)
  \brief  Associate \a name dimension name with \a id XML ID and store them
  into the global linked list.
  
  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note made error message consistent.

*/
void 
set_dn2id_list_id(char* name, unsigned int id)
{

    dn2id_list_ptr p = NULL;
    dn2id_list_ptr tail = NULL;

    p = (dn2id_list_ptr) HDmalloc(sizeof(dn2id_list_elmt)); 
    HDmemset(p->elmt.name, 0, H4_MAX_NC_NAME);

    if (p != NULL) { 
        strncpy(p->elmt.name, name, strlen(name));
        p->elmt.id = id; 
        p->elmt.written = 0;
        p->next = NULL; 
    } 
    else {
        fprintf(flog, "HDmalloc() failed: Out of Memory\n");
    }

    if(dn2id_list == NULL){
        dn2id_list = p;
    }
    else{
        tail = dn2id_list;
        while(tail->next != NULL){
            tail = tail->next;
        }
        tail->next = p;
    }

}

/*!
  \fn set_dn2id_list_written(char* name)
  \brief  Remeber that \a name dimension name was written to MAP file already.

*/
unsigned int 
set_dn2id_list_written(char* name)
{
    dn2id_list_ptr p = dn2id_list;

    while(p != NULL){

        if(!strncmp(name, p->elmt.name, strlen(name))){
            p->elmt.written = 1;
            return p->elmt.written;
        }
        p = p->next;
    }
    return 0;


}


/*!

  \fn write_array_attrs(FILE *ofptr, int32 sd_id, int32 nattrs, intn indent)
  \brief  Write all SD attributes.

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)
  \date February 7, 2012
  \note added error handling for memory allocation failure.

  \date June 13, 2011
  \note 
  fixed dropping MSB in nt_type.

  \date April 13, 2011
  \note 
  attr_name is escaped with get_string_xml().

  \date March 15, 2011
  \note 
  added error handlers.

 */
unsigned int ID_AA = 0;  /*!< Unique SDS attribute ID.  */

intn 
write_array_attrs(FILE *ofptr, int32 sdsid, int32 nattrs, intn indent)
{

    int32 attr_index, attr_count, attr_nt, attr_buf_size;
    int32 attr_nt_no_endian;

    char* name_xml = NULL;
    char* attr_nt_desc = NULL;

    char  attr_name[H4_MAX_NC_NAME];

    VOIDP attr_buf = NULL;
    intn  status = SUCCEED;     /* status from a called routine */
    
    /* For each file attribute, print its info and values. */
    for (attr_index = 0; attr_index < nattrs; attr_index++)
        {
            int32 offset = -1;
            int32 length = 0;

            /* Get the attribute's name, number type, and number of values. */
            status = SDattrinfo(sdsid, attr_index, attr_name, &attr_nt, 
                                &attr_count);
            if( status == FAIL ){
                FAIL_ERROR("SDattrinfo() failed.");
            }
      
            /* get number type description of the attribute */
            attr_nt_desc = HDgetNTdesc(attr_nt);
            attr_buf_size = DFKNTsize(attr_nt) * attr_count;
            if (attr_nt_desc == NULL || attr_buf_size <= 0)
                continue;

            attr_nt_no_endian = (attr_nt & 0xff);
            if (attr_nt_no_endian == DFNT_CHAR || 
                attr_nt_no_endian == DFNT_UCHAR)
                attr_buf = (char *) HDmalloc(attr_buf_size+1);
            else
                attr_buf = (VOIDP)HDmalloc(attr_buf_size);

            if(attr_buf == NULL){
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }
            status = SDreadattr(sdsid, attr_index, attr_buf);
            if( status == FAIL ) {
                HDfree (attr_buf);
                FAIL_ERROR("SDreadattr() failed.");
            }

            status = SDgetattdatainfo(sdsid, attr_index, &offset, &length);
            
            if(status == DFE_NOVGREP){
                /* Retrieve the old attribute data info. */
                status = SDgetoldattdatainfo(0, sdsid, attr_name, 
                                             &offset, &length);
                if(status == FAIL){
                    HDfree(attr_nt_desc);
                    HDfree(attr_buf);
                    FAIL_ERROR("SDgetoldattdatainfo() failed.");
                }
                /* Recalculate offset and length based on the name
                 of the attributes.*/
                get_adjusted_attr_datainfo(attr_name, &offset, &length); 
                
            }

            if(status == FAIL){
                HDfree(attr_nt_desc);
                HDfree(attr_buf);
                FAIL_ERROR("SDgetattdatainfo() failed.");
            }

            start_elm(ofptr, TAG_AATTR, indent);
            name_xml = get_string_xml(attr_name, strlen(attr_name));
            if(name_xml != NULL) {
                fprintf(ofptr, "name=\"%s\" id=\"ID_AA%d\">", name_xml, 
                        ++ID_AA);
                HDfree(name_xml);
            }
            else{
                fprintf(flog, "get_string_xml() returned NULL.\n");
                status = FAIL;                
            }

            write_array_attribute(ofptr, attr_nt, attr_count, attr_buf, 
                                  attr_buf_size, offset, length, indent+1);
            end_elm(ofptr, TAG_AATTR, indent);

            HDfree(attr_buf);
            if(attr_nt_desc != NULL)
                HDfree (attr_nt_desc);
        }  /* for each file attribute */
    return(status);
}   /* end of wirte_array_attrs */


/*!

  \fn write_array_attribute(FILE *ofptr, int32 attr_nt, int32 attr_count,
                           VOIDP attr_buf, int32 attr_buf_size, 
                           int32 offset, int32 length, intn indent)

  \brief Write array attribute data.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date March 15, 2011
  \note changed return type to void.

  \date October 12, 2010
  \note created
*/
void
write_array_attribute(FILE *ofptr, int32 attr_nt, int32 attr_count,
                      VOIDP attr_buf, int32 attr_buf_size,
                      int32 offset, int32 length, intn indent)
{
    if (is_string(attr_nt) == 1){
        ((char *)attr_buf)[attr_buf_size] = '\0';
        /* Write non-numeric datum element. */
        write_map_datum_char(ofptr, attr_nt, indent);
        /* Write attributeData element. */
        write_attr_data(ofptr, offset, length, indent);
        /* Write stringValue element. */
        start_elm_0(ofptr, TAG_SVALUE, indent);
        write_attr_value(ofptr, attr_nt, attr_count, attr_buf, 
                         indent);
        end_elm_0(ofptr, TAG_SVALUE);
    }
    else{
        write_map_datum(ofptr, attr_nt, indent);
        /* Write attributeData element. */
        write_attr_data(ofptr, offset, length, indent);
        /* Write numericValue element. */
        start_elm_0(ofptr, TAG_NVALUE, indent);
        write_attr_value(ofptr, attr_nt, attr_count, attr_buf, 
                         indent);
        end_elm_0(ofptr, TAG_NVALUE);
    }
}

/*!
  \fn write_array_byte_stream(FILE *ofptr, SD_mapping_info_t *map_info, 
  intn indent)
  \brief Write array data(SDS) byte stream information.

  If lengths and offsets are back to back, they'll be merged into one.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date October 13, 2010

*/
intn write_array_byte_stream(FILE *ofptr, SD_mapping_info_t *map_info, 
                            intn indent)
{
    write_byte_streams(ofptr, map_info->nblocks, map_info->offsets, 
                       map_info->lengths, indent);
    return 1;
}

/*!
  \fn write_array_chunks(FILE *ofptr, SD_mapping_info_t *map_info, int32 rank,
  int32* dimsizes, HDF_CHUNK_DEF* cdef, intn indent)
  \brief Write array data(SDS) chunk information.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date October 13, 2010

*/
intn 
write_array_chunks(FILE *ofptr, SD_mapping_info_t *map_info, int32 rank,
                   int32* dimsizes, HDF_CHUNK_DEF* cdef, intn indent)
{
    int k;
    intn status = FAIL;
    /* Write <h4:chunks> element. */
    start_elm_0(ofptr, TAG_CHUNK, indent);
    /* Write <h4:chunkDimensionSizes> element. */
    start_elm_0(ofptr, TAG_CSPACE, indent+1);

    for (k=0; k < rank; k++){
        if(k != rank - 1){
            fprintf(ofptr, "%d ", (int)cdef->chunk_lengths[k]);
        }
        else{
            fprintf(ofptr, "%d", (int)cdef->chunk_lengths[k]);

        }

    }
    end_elm_0(ofptr, TAG_CSPACE);

    status = write_array_chunks_byte_stream(ofptr, map_info, rank, dimsizes, 
                                            cdef, indent+1);
    end_elm(ofptr, TAG_CHUNK, indent);
    return status;
}


/*!

  \fn write_array_chunks_byte_stream(FILE *ofptr, SD_mapping_info_t *map_info, 
                               int32 rank, int32* dimsizes, 
                               HDF_CHUNK_DEF* cdef,intn indent)

  \brief Write chunked array data(SDS) byte stream information.

  It writes chunk array information.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Jul 19, 2016
  \note added uuid function call.

  \date Oct 11, 2013
  \note made error message consistent.

  \date April 27, 2011
  \note	removed the use of read_fill_value() function.	

  \date April 26, 2011
  \note updated error message for read_fill_value() failure.

  \date October 20, 2010
  \note created.
*/
intn 
write_array_chunks_byte_stream(FILE *ofptr, SD_mapping_info_t *map_info, 
                               int32 rank, int32* dimsizes, 
                               HDF_CHUNK_DEF* cdef,intn indent)
{
    int i = 0;
    int j = 0;
    int k = 0;
    int chunk_size = 1;


    int32* strides = NULL;
    int* steps = NULL;

    strides = (int32 *) HDmalloc((int)rank * sizeof(int32));
    if(strides == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }

    steps = (int *) HDmalloc((int)rank * sizeof(int));
    if(steps == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }

    /* Initialize steps. */
    for(i = 0; i < (int)rank; i++){
        steps[i] = (int)(dimsizes[i] / cdef->chunk_lengths[i]);
        chunk_size = chunk_size * steps[i];
        strides[i] = 0;
    }


    for (j=0; j < chunk_size; j++) {

        int scale = 1;

        if(read_chunk(map_info->id, map_info, strides) == FAIL){
            fprintf(flog, "read_chunk() failed at chunk - ");
            for(i = 0; i < (int)rank; i++){
                fprintf(flog, " %ld", (long) strides[i]);
            }
            fprintf(flog, "\n");
            HDfree(strides);
            HDfree(steps);
            return FAIL;
        }

        if(map_info->nblocks > 1){ /* Add h4:byteStreamSet element. */
            start_elm(ofptr, TAG_BSTMSET, indent);

            write_chunk_position_in_array(ofptr, (int)rank, 
                                          cdef->chunk_lengths, 
                                          strides, 
                                          0);  /* Do not close the tag. */
            for (k=0; k < map_info->nblocks; k++) {
                start_elm(ofptr, TAG_BSTREAM, indent+1);
                if (uuid == 1) {
                    write_uuid(ofptr, (int)map_info->offsets[k],
                               (int)map_info->lengths[k]);
                }
                fprintf(ofptr, "offset=\"%d\" nBytes=\"%d\"/>", 
                        (int)map_info->offsets[k], (int)map_info->lengths[k]);
            }
            end_elm(ofptr, TAG_BSTMSET, indent);
        }
        else{                   
            if(map_info->nblocks == 1) { /* There's only one block stream. */
                start_elm(ofptr, TAG_BSTREAM, indent);
                if (uuid == 1) {
                    write_uuid(ofptr, (int)map_info->offsets[k],
                               (int)map_info->lengths[k]);
                }
                
                fprintf(ofptr, "offset=\"%d\" nBytes=\"%d\" ", 
                        (int)map_info->offsets[0], (int)map_info->lengths[0]);
                write_chunk_position_in_array(ofptr, (int)rank, 
                                              cdef->chunk_lengths, 
                                              strides, 
                                              1); /* Close the tag. */
            }
            else {
                /* 0 means all fill values in the chunk. 
                   See MISR_ELIPSOID case. */
                /* Thus, we'll skip generating offset / length. */
                start_elm(ofptr, TAG_FVALUE, indent);        
                fprintf(ofptr, "value=\"");
                write_attr_value(ofptr, map_info->data_type, 1, 
                                 map_info->fill_value, 0);                  
                fprintf(ofptr, "\" ");
                write_chunk_position_in_array(ofptr, (int)rank, 
                                              cdef->chunk_lengths, 
                                              strides, 
                                              1); /* Close the tag. */
            }
        }

        /* Increase strides for each dimension. */
        /* The fastest varying dimension is rank-1. */
        for(i = rank-1; i >= 0 ; i--){ 
            if((j+1) % scale == 0){
                strides[i] = ++strides[i] % steps[i];
            }
            scale = scale * steps[i];
        }
    } 

    HDfree(strides);
    HDfree(steps);
    return SUCCEED;
}

/*!
  \fn write_array_data(FILE *ofptr, SD_mapping_info_t *map_info, int32 rank,
  int32* dimsizes,  HDF_CHUNK_DEF* cdef, int32 chunk_flag, intn indent)
  \brief Write array data(SDS) information.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date October 19, 2011
  \note	added the rank > 0 condition for fastestVaryingDimensionIndex.

  \date April 27, 2011
  \note removed the use of read_fill_value() function.


  \date April 26, 2011
  \note removed the line for saving sdsid in map_info.

  \date October 13, 2010
  \note created.

*/
intn 
write_array_data(FILE *ofptr, SD_mapping_info_t *map_info, int32 rank, 
                 int32* dimsizes, HDF_CHUNK_DEF* cdef, int32 chunk_flag, 
                 intn indent)
{
    intn  ret_value = SUCCEED;

    /* Handle it differently if chunking is used. */
    if(chunk_flag){
        if(rank > 0){
            start_elm(ofptr, TAG_ARRDATA, indent+1);
            fprintf(ofptr, "fastestVaryingDimensionIndex=\"%ld\"", (long) rank-1);
        }
        else{
            start_elm_0(ofptr, TAG_ARRDATA, indent+1);
        }

        /* If compression is used, add compression information. */
        if(read_compression(map_info) == FAIL){
            fprintf(flog, "read_compression() failed.");
            fprintf(flog, ":sdsid=%ld\n", (long)map_info->id);
            return FAIL;
        }

        if(HDstrlen(map_info->comp_info) > 0)
            fprintf(ofptr, " %s", map_info->comp_info);
        if(rank > 0)
            fprintf(ofptr, ">");

        ret_value = write_array_chunks(ofptr, map_info, rank, dimsizes, 
                                       cdef, indent+2);
        end_elm(ofptr, TAG_ARRDATA, indent+1);

    }
    else{
        ret_value = SDmapping(map_info->id, map_info);
        if(ret_value == FAIL){
            FAIL_ERROR("SDmapping() failed.");
        }
     
        if(map_info->nblocks > 0){
            if(rank > 0){
                start_elm(ofptr, TAG_ARRDATA, indent+1);
                fprintf(ofptr, "fastestVaryingDimensionIndex=\"%ld\"",
                        (long) rank-1);
            }
            else{
                start_elm_0(ofptr, TAG_ARRDATA, indent+1);
            }

            /* If compression is used, add compression information. */
            if(read_compression(map_info) == FAIL){
                fprintf(flog, "read_compression() failed.");
                fprintf(flog, ":sdsid=%ld\n", (long)map_info->id);
                return FAIL;
            }

            if(HDstrlen(map_info->comp_info) > 0){
                fprintf(ofptr, " %s", map_info->comp_info);
            }
            else{

            }
            if(rank > 0){
                fprintf(ofptr, ">");
            }

            ret_value = write_array_byte_stream(ofptr, map_info, indent+2);
            end_elm(ofptr, TAG_ARRDATA, indent+1);
        }
        else{
            if(map_info->nblocks == 0){
                /* It must have fillvalue. */
                if(rank > 0){
                    start_elm(ofptr, TAG_ARRDATA, indent+1);
                    fprintf(ofptr, "fastestVaryingDimensionIndex=\"%ld\">", 
                            (long) rank-1);
                }
                else{
                    start_elm_0(ofptr, TAG_ARRDATA, indent+1);
                }
                /* Write fill value.*/
                start_elm(ofptr, TAG_FVALUE, indent+2);        
                fprintf(ofptr, "value=\"");
                write_attr_value(ofptr, map_info->data_type, 1, 
                                 map_info->fill_value, 0);  
                fprintf(ofptr, "\"/>");
                end_elm(ofptr, TAG_ARRDATA, indent+1);
            }

        }
    }

    return ret_value;
}

/*!
  \fn void write_attr_data_meta(FILE *ofptr, int32 offset, int32 length, 
  char* name, int indent)
  \brief Write attributeData element for merged metadata.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date July 21, 2016
  \note added uuid function call.

  \date September 24, 2010
  \note created.

*/
void 
write_attr_data_meta(FILE *ofptr, int32 offset, int32 length, char* name,
                     int indent)
{


    start_elm(ofptr, TAG_BSTREAM, indent+1);
    fprintf(ofptr, "originalAttribute=\"%s\" ", name); 
    if(offset == -1){
        fprintf(ofptr, "offset=\"N/A\" nBytes=\"%ld\"/>", (long) length); 
    }
    else {
        if(uuid == 1) {
            write_uuid(ofptr, offset, length);
        }
        fprintf(ofptr, "offset=\"%ld\" nBytes=\"%ld\"/>", 
                (long) offset, (long) length);
    }


}


/*!
  \fn write_chunk_position_in_array(FILE* ofptr, int rank, int32* lengths, 
      int32* strides, int tag_close)
  \brief Write chunk position in array.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date October 13, 2010
 */

void 
write_chunk_position_in_array(FILE* ofptr, int rank, int32* lengths, 
                              int32* strides, int tag_close)
{
    int i=0;
    fprintf(ofptr, "chunkPositionInArray=\"[");
    for(i = 0; i < (int)rank; i++){
        int32 index = lengths[i] * strides[i];
        if(i != rank - 1){
            fprintf(ofptr, "%ld,", (long) index);
        }
        else{
            fprintf(ofptr, "%ld", (long) index);
        }
    }
    if(tag_close)
        fprintf(ofptr, "]\"/>"); 
    else
        fprintf(ofptr, "]\">"); 

}

/*!
  \fn write_dimension(FILE *ofptr, int32 sdsid, char* obj_name, 
  int32 data_type, int32 rank, int32* dimsizes, int32 num_attrs,
  SD_mapping_info_t* map_info, intn indent)

  \brief Write dimension data.

  Write dimension data for coordinate variables that have scale information.
  If there are no scale information, this function will not be called at all.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date March 15, 2011
  \note 
  cleaned up error messages.
  added more error handlers.

  \date September 24, 2010
  \note created.

  \see write_dimension_no_data()
*/
intn 
write_dimension(FILE *ofptr, int32 sdsid, char* obj_name, int32 data_type,
                int32 rank, int32* dimsizes, int32 num_attrs,
                SD_mapping_info_t* map_info, intn indent)
{

    unsigned int id = 0;
    intn emptySDS = 0;
    intn status = SUCCEED;
    
    start_elm(ofptr, TAG_DIM, indent);

    /* See if obj_name already exists in the Dimension name to ID list. */
    id = get_dn2id_list_id(obj_name);
    if(id == 0){
        id = ++ID_D;
        set_dn2id_list_id(obj_name, id);
    }

    /* Rename fakeDim to "". The decision was made during telecon. */
    if(strncmp(obj_name, "fakeDim", 7)){
        get_escape_xml(obj_name);
        fprintf(ofptr, "name=\"%s\" id=\"ID_D%d\">", obj_name, id);
    }
    else {
        fprintf(ofptr, "name=\"\" id=\"ID_D%d\">", id);
    }

    /* Write dimension attribute. */
    if(num_attrs > 0 ) {
        status = write_dimension_attrs(ofptr, sdsid, num_attrs, indent+1);
        if(status == FAIL){
            fprintf(flog, "write_dimension_attrs() failed.");
            fprintf(flog, ":name=%s, nattrs=%ld\n", obj_name, (long)num_attrs);
            return FAIL;
        }

    }
    /* Check if SDS has no data. */
    if(SDcheckempty(sdsid, &emptySDS) == FAIL){
        FAIL_ERROR("SDcheckempty() failed.");
    }
    if(emptySDS != 1){
        /* Write datum if it has data. */
        write_map_datum(ofptr, data_type, indent+1); 
        /* Write dimension data. */
        write_dimension_data(ofptr, map_info, indent+1);
        /* Write verification values. */
        status =
            write_verify_array_values(ofptr, sdsid, obj_name, rank,
                                      dimsizes, data_type, indent+1);
        if(status == FAIL){
            fprintf(flog, "write_verify_array_values() failed.");
            fprintf(flog, ":name=%s, rank=%ld\n", obj_name, (long)rank);
            return FAIL;
        }
        
        
    }
    end_elm(ofptr, TAG_DIM, indent);

    set_dn2id_list_written(obj_name);
    return status;
}


/*!
  \fn write_dimension_attrs(FILE *ofptr, int32 sdsid, int32 nattrs, 
  intn indent)

  \brief  Write all dimension attributes.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date February 7, 2012
  \note filtered reserved XML character	from the attribute name.

  \date June 13, 2011
  \note fixed dropping MSB in nt_type.

  \date March 15, 2011
  \note 
  cleaned up error messages.	
  added more error handlers.	

  \date August 6, 2010
  \note created.

  \see write_array_attrs()
 */
intn 
write_dimension_attrs(FILE *ofptr, int32 sdsid, int32 nattrs, intn indent)
{
    static unsigned int ID_DA = 0;

    int32 attr_index = 0;
    int32 attr_count = 0;
    int32 attr_nt = 0;
    int32 attr_nt_no_endian = 0;
    int32 attr_buf_size = 0;

    char  attr_name[H4_MAX_NC_NAME];
    char* attr_nt_desc = NULL; 
    char* name_xml = NULL;

    VOIDP attr_buf = NULL;

    intn  status = SUCCEED;   /* status from a called routine */

    /* For each file attribute, print its info and values. */
    for (attr_index = 0; attr_index < nattrs; attr_index++)
        {
            int32 offset = -1;
            int32 length = 0;

            /* Get the current attr's name, number type, and number of 
               values */
            status = SDattrinfo(sdsid, attr_index, attr_name, &attr_nt, 
                                &attr_count);
            if( status == FAIL ){
                FAIL_ERROR("SDattrinfo() failed.");
            }
      
            /* get number type description of the attribute */
            attr_nt_desc = HDgetNTdesc(attr_nt); 
            attr_buf_size = DFKNTsize(attr_nt) * attr_count;
            if (attr_nt_desc == NULL || attr_buf_size <= 0)
                continue;

            attr_nt_no_endian = (attr_nt & 0xff);
            if (attr_nt_no_endian == DFNT_CHAR || 
                attr_nt_no_endian == DFNT_UCHAR){
                attr_buf = (char *) HDmalloc(attr_buf_size+1);
            }
            else{
                attr_buf = (VOIDP)HDmalloc(attr_buf_size);
            }

            if(attr_buf == NULL){
                HDfree (attr_nt_desc);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            status = SDreadattr(sdsid, attr_index, attr_buf);
            if( status == FAIL ) {
                HDfree (attr_buf);
                HDfree (attr_nt_desc);
                FAIL_ERROR("SDreadattr() failed.");
            }

            start_elm(ofptr, TAG_DATTR, indent);
            name_xml =  get_string_xml(attr_name, strlen(attr_name));
            if(name_xml != NULL){
                fprintf(ofptr, "name=\"%s\" id=\"ID_DA%d\">", name_xml, 
                        ++ID_DA);
                HDfree(name_xml);
            }
            else{
                fprintf(flog, "get_string_xml() returned NULL.\n");
                status = FAIL;
            }


            status = SDgetattdatainfo(sdsid, attr_index, &offset, &length);

            if(status == DFE_NOVGREP){ 
                /* It must be old dimension attribute. */
                /* 0 is passed since Dimension variable is 1-D. */
                int32 sdsdim_id = SDgetdimid(sdsid, 0);	
                int32 sds_id = get_did2sid_list(sdsdim_id);
                status = SDgetoldattdatainfo(sdsdim_id, sds_id, attr_name, 
                                             &offset, &length);
            }


            if(status == FAIL){
                HDfree(attr_nt_desc);
                HDfree(attr_buf);
                FAIL_ERROR("SDgetoldattdatainfo() failed");
            }

            write_array_attribute(ofptr, attr_nt, attr_count, attr_buf, 
                                  attr_buf_size, offset, length,  indent+1);
            end_elm(ofptr, TAG_DATTR, indent);
            HDfree(attr_nt_desc);
            HDfree(attr_buf);
        }  /* for each file attribute */
    return status;
}   /* end of write_dimension_attrs */

/*!
  \fn write_dimension_data(FILE *ofptr, SD_mapping_info_t *map_info, 
  intn indent)

  \brief  Write dimension data info.

  \see write_array_byte_stream()

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Feb 21, 2012
  \note merged offset and length information if consecutive.

  \date July 6, 2011
  \note added byteStreamSet element if number of block > 1.

  \date August 6, 2010
  \note created.

 */
intn 
write_dimension_data(FILE *ofptr, SD_mapping_info_t *map_info, intn indent)
{
    start_elm_0(ofptr,TAG_DDATA, indent);
    write_byte_streams(ofptr, map_info->nblocks, map_info->offsets, 
                       map_info->lengths, indent+1);
    end_elm(ofptr, TAG_DDATA, indent);
    return 1;

}


/*!
  \fn write_dimension_ref(FILE *ofptr, int32 sds_id, 
  int32 rank, intn indent)

  \brief  Write dimension scale references.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date August 9, 2010
 */
intn 
write_dimension_ref(FILE *ofptr, int32 sds_id, int32 rank,
                    intn indent)
{
    char   sdsdim_name[H4_MAX_NC_NAME]; /* SDS dimensional name */

    int32  sdsdim_id = -1; /* SDS dimensional scale id */
    int32  sdsdim_type = 0; /* SDS dimensional scale data type */
    int32  sdsdim_nattrs = 0; /* number of SDS dimensional scale attributes */
    int32  sdsdim_size = 0; /* the size of SDS dimensional scale */
    int32  stat = 0;   /* the "return" status of HDF4 APIs */

    int j=0;
    int id = 0;

    for (j=0; j < rank; j++) {

        sdsdim_id = SDgetdimid(sds_id, j);	

        if(sdsdim_id == FAIL) {
            fprintf(flog, 
                    "cannot obtain SDS dimension for sds_id = %ld rank = %d", 
                    (long) sds_id, j);
            return FAIL;
        }

        /* Save the {sds_id, dim_id} relation for old attribute handling. */
        set_did2sid_list(sdsdim_id, sds_id);
    
        /* Get the information of this dimensional scale. */
        stat = SDdiminfo(sdsdim_id, sdsdim_name, &sdsdim_size,
                         &sdsdim_type, &sdsdim_nattrs);  
        if (stat == FAIL) {						      
            fprintf(flog,
                    "cannot obtain SDS dimension information");
            return FAIL;
        }

        /* Check if sdsdim_type is not 0. It's not processed yet. */ 
        if(sdsdim_type != 0){
            /* Check if the dimension name is seen already or not. */
            id = get_dn2id_list_id(sdsdim_name);
            if(id == 0){
                id = ++ID_D;
                set_dn2id_list_id(sdsdim_name, id);
            }
            start_elm(ofptr, TAG_DREF, indent);
            /* Rename fakeDim to "". */
            if(strncmp(sdsdim_name, "fakeDim", 7)){
                get_escape_xml(sdsdim_name);
                fprintf(ofptr, 
                        "name=\"%s\" dimensionIndex=\"%d\" ref=\"ID_D%d\"/>", 
                        sdsdim_name,  j, id);
            }
            else{
                fprintf(ofptr, 
                        "name=\"\" dimensionIndex=\"%d\" ref=\"ID_D%d\"/>", 
                         j, id);
            }

        } /* if(sdsdim_type != 0) */
        else{ 
            /* 
               sdsdim_type == 0 means no data is involved and 
               it's already processed by set_dimension_no_data(). 
             */
            id = get_dn2id_list_id(sdsdim_name);
            if(id > 0){
                start_elm(ofptr, TAG_DREF, indent);
                if(strncmp(sdsdim_name, "fakeDim", 7)){
                    get_escape_xml(sdsdim_name);
                    fprintf(ofptr, 
                            "name=\"%s\" dimensionIndex=\"%d\" ref=\"ID_D%d\"/>", 
                            sdsdim_name,  j, id);
                }
                else {
                    fprintf(ofptr, 
                            "name=\"\" dimensionIndex=\"%d\" ref=\"ID_D%d\"/>", 
                            j, id);

                }
            }
        } /* if(sdsdim_type == 0) */

    } /*  for (j=0; j < rank; j++) */
    return 1;
}

/*!
  \fn write_file_attrs(FILE *ofptr, int32 sdsid, int32 nattrs, intn indent)
  \brief Write file attributes like StructMetadata and coreMetadata.

  \return FAIL if any HDF4 calls fail.
  \return SUCCEED otherwise

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note made error message consistent.

  \date February 7, 2012
  \note filtered reserved XML character	from the attribute name.

  \date June 13, 2011
  \note fixed dropping MSB in nt_type.

  \date March 10, 2011
  \note added error handlings.


  \date July 30, 2010
  \note created.


  
*/
intn 
write_file_attrs(FILE *ofptr, int32 sdsid, int32 nattrs, intn indent)
{

    int32 attr_index = 0;
    int32 attr_count = 0;
    int32 attr_nt = 0;
    int32 attr_nt_no_endian = 0;
    int32 attr_buf_size = 0;

    char  attr_name[H4_MAX_NC_NAME];
    char *attr_nt_desc = NULL;
    char *name_xml = NULL;

    VOIDP attr_buf = NULL;
    intn  status = SUCCEED;           /* status from a called routine */

    /* For each file attribute, print its info and values. */
    for (attr_index = 0; attr_index < nattrs; attr_index++)
        {
            int32 offset = -1;
            int32 length = 0;

            /* Get the attribute's name, number type, and number of values. */
            status = SDattrinfo(sdsid, attr_index, attr_name, &attr_nt, 
                                &attr_count);
            if( status == FAIL ){
                FAIL_ERROR("SDattrinfo() failed.");
            }
            /* Get number type description of the attribute */
            attr_nt_desc = HDgetNTdesc(attr_nt);
            attr_buf_size = DFKNTsize(attr_nt) * attr_count;
            if (attr_nt_desc == NULL || attr_buf_size <= 0)
                continue;
                    
            attr_nt_no_endian = (attr_nt & 0xff);

            
            if (attr_nt_no_endian == DFNT_CHAR || 
                attr_nt_no_endian == DFNT_UCHAR)
                attr_buf = (char *) HDmalloc(attr_buf_size+1);
            else
                attr_buf = (VOIDP) HDmalloc(attr_buf_size);

            if(attr_buf == NULL){
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            status = SDreadattr(sdsid, attr_index, attr_buf);
            if(status == FAIL) {
                HDfree(attr_nt_desc);
                HDfree(attr_buf);                
                FAIL_ERROR("SDreadattr() failed.");
            }


            status = SDgetattdatainfo(sdsid, attr_index, &offset, &length);
            if(status == FAIL){
                HDfree(attr_nt_desc);
                HDfree(attr_buf);
                FAIL_ERROR("SDgetattdatainfo() failed.");
                return FAIL;
            }

            if(is_string(attr_nt) == 1 && merge == 1){
                status = set_mattr(attr_name, attr_nt, attr_buf, attr_buf_size,
                                   offset, length, nattrs);
                if(status == FAIL) {
                    fprintf(flog, "set_mattr() failed:attr_name=%s.\n", 
                            attr_name);
                    HDfree(attr_nt_desc);
                    HDfree(attr_buf);
                    return FAIL;
                }
            }
            else {
                start_elm(ofptr, TAG_FATTR, indent);
                name_xml =  get_string_xml(attr_name, strlen(attr_name));
                if(name_xml != NULL){                
                    fprintf(ofptr, 
                            "name=\"%s\" origin=\"File Attribute: Arrays\"", 
                            name_xml);
                    HDfree(name_xml);                    
                }
                else{
                    fprintf(flog, "get_string_xml() returned NULL.\n");
                    status = FAIL;
                }
                fprintf(ofptr, 
                        " id=\"ID_FA%d\">", 
                        ++ID_FA);            

                write_array_attribute(ofptr, attr_nt, attr_count, attr_buf, 
                                      attr_buf_size, offset, length, indent+1);

                end_elm(ofptr, TAG_FATTR, indent);
            }
            HDfree(attr_nt_desc);
            HDfree(attr_buf);
        }  /* for each file attribute */
    return SUCCEED;
}   /* end of write_file_attrs */


/*!
  \fn write_map_dimension_no_data(FILE *ofptr)

  \brief Write dimension with no data.

  Write dimension data for named dimensions. They may have attributes.

  \return FAIL
  \return SUCCEED
  
  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date May 3, 2011
  \note increased the indentation.

  \date March 15, 2011
  \note 
  cleaned up error handlers.

  \date March 10, 2011
  \note replaced return 1 with exit().

  \date August 17, 2010
  \note created.
  \see write_map_dimension()

*/
intn 
write_map_dimension_no_data(FILE *ofptr)
{

    char   sdsdim_name[H4_MAX_NC_NAME]; /* SDS dimensional name */

    int32  sdsdim_id = -1; /* SDS dimensional scale id */
    int32  sdsdim_type = 0; /* SDS dimensional scale data type */
    int32  sdsdim_nattrs; /* number of SDS dimensional scale attributes */
    int32  sdsdim_size; /* the size of SDS dimensional scale */
    int32  stat;   /* the "return" status of HDF4 APIs */

    int id = 0;

    dn_list_ptr p = dn_list;

    while(p != NULL){
        sdsdim_id = p->id;
        /* Get the information of this dimensional scale. */
        stat = SDdiminfo(sdsdim_id, sdsdim_name, &sdsdim_size,
                         &sdsdim_type, &sdsdim_nattrs);  


        if (stat == FAIL) {						      
            FAIL_ERROR("SDdiminfo() failed.");
        }
        id = get_dn2id_list_id(sdsdim_name);
        /* Check if the dimension name is already written or not. */
        if(get_dn2id_list_written(sdsdim_name) == 0){
            /* Remember that this dimension name is handled. */
            set_dn2id_list_written(sdsdim_name);

            start_elm(ofptr, TAG_DIM, 2);

            /* Rename fakeDim to "". */
            if(strncmp(sdsdim_name, "fakeDim", 7)){
                get_escape_xml(sdsdim_name);
                fprintf(ofptr, "name=\"%s\" id=\"ID_D%d\"", 
                        sdsdim_name, id);
            }
            else{
                fprintf(ofptr, "name=\"\" id=\"ID_D%d\"", 
                        id);
            }

            /* Write dimension attributes. */
            if(sdsdim_nattrs > 0){
                fprintf(ofptr, ">"); 
                write_dimension_attrs(ofptr, sdsdim_id, sdsdim_nattrs, 
                                      2);
                end_elm(ofptr, TAG_DIM, 2);
            }
            else{
                /* Close the tag if dimension has no attributes. */
                fprintf(ofptr, "/>"); 

            }
        } /* if(get_dn2id_list_written(sdsdim_name) == 0) */
        p = p->next;
    } /* while( p!= NULL) */
    return SUCCEED;
}

/*!

  \fn write_map_dimensions(FILE *ofptr)
  \brief Write all SDS coordinate variables

  Write all SDS coordinate variables collected in the global coordinate 
  variable list using dimension element. The indentation is 1 because they 
  will be written at the end of the map.

  \return FAIL
  \return SUCCEED


  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date May 3, 2011
  \note increased the indentation.

  \date March 15, 2011
  \note 
  changed return type.
  added more error handlers.
  removed local num_errs variable.

  \date March 10, 2011
  \note added num_errs for return value and changed return type from intn to 
  int.

  \date September 29, 2010
  \note created.
 */
intn write_map_dimensions(FILE *ofptr)
{

    char  obj_name[H4_MAX_NC_NAME]; /* name of the object */

    int32 rank = 0;             /* number of dimensions */
    int32 dimsizes[H4_MAX_VAR_DIMS]; /* dimension sizes */
    int32 data_type = 0;        /* data type */
    int32 num_attrs  = 0;       /* number of global attributes */

    intn status = SUCCEED;      /* status flag */

    SD_mapping_info_t sd_map_info;
    cv_list_ptr p = cv_list;

    HDmemset(obj_name, 0, H4_MAX_NC_NAME);

    while(p != NULL){
        status = SDgetinfo(p->id, obj_name, &rank, dimsizes, &data_type, 
                           &num_attrs);
        if(status == FAIL){
            FAIL_ERROR("SDgetinfo() failed.");
        }
        HDmemset(&sd_map_info, 0, sizeof(sd_map_info));

        status = SDmapping(p->id, &sd_map_info);
        if(status == FAIL){
            fprintf(flog, "SDmapping() failed.");
            fprintf(flog, ":name=%s\n", obj_name);
            SDfree_mapping_info(&sd_map_info);
            return status;
        }

        status = write_dimension(ofptr, p->id, obj_name, data_type, rank, 
                                 dimsizes, num_attrs, &sd_map_info, 2);
        if(status == FAIL){
            fprintf(flog, "write_dimension() failed.");
            fprintf(flog, ":name=%s nattrs=%ld\n", obj_name, (long)num_attrs);
            SDfree_mapping_info(&sd_map_info);
            return FAIL;
        }
        SDfree_mapping_info(&sd_map_info);
        p = p->next;
    }    
    return status;
}



/*!

  \fn write_map_lone_sds(FILE *ofptr, char *infile, int32 sd_id, int32 an_id,
                         ref_count_t *sd_visited)
  \brief  Write mapping information for all lone SDS.

  \return FAIL
  \return SUCCEED


  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 11, 2013
  \note added ref for error message.

  \date May 3, 2011
  \note increased the indentation.

  \date April 27, 2011  
  \note filtered arrays that are mapped under groups.

  \date March 15, 2011  
  \note corrected error handler for write_map_sds() call.

  \date March 11, 2011  
  \note 
  changed return type to either FAIL or SUCCEED.
  cleaned up error messages.
  removed num_errs variable.
 */
intn 
write_map_lone_sds(FILE *ofptr, char *infile, int32 sd_id, int32 an_id,
                   ref_count_t *sd_visited)
{
    int i = 0;
    int32 num_sds = 0;          /* number of SDS in file       */
    int32 num_gattr = 0;        /* number of global attributes */
    int32 sdsid = 1;            /* dataset id                  */
    int32 ref = -1;             /* reference number */
    intn  status = SUCCEED;     /* status flag                 */


    status = SDfileinfo(sd_id, &num_sds, &num_gattr);
    if(status == FAIL)
        FAIL_ERROR("SDfileinfo() failed.");

    for (i=0; i < num_sds; i++) {
        sdsid = SDselect(sd_id, i);
        if(sdsid == FAIL)
            FAIL_ERROR("SDselect() failed.");

        ref = SDidtoref(sdsid);
        /* Get the reference number. */
        if(ref == FAIL){
            FAIL_ERROR("SDidtoref() failed.");
        }

        /* Filter out arrays that are visited already. */
        if(ref != ref_count(sd_visited, ref)){

            SD_mapping_info_t map_info;

            /* Reset mapping file information for SDS dataset */
            HDmemset(&map_info, 0, sizeof(map_info));

            status = write_map_sds(ofptr, sdsid, an_id, "/", &map_info, 2, 
                                   sd_visited);
            if(status == FAIL){
                fprintf(flog, "write_map_sds() failed:");
                fprintf(flog, "sds_id=%ld, ref=%ld, an_id=%ld.\n",
                        (long)sdsid, (long)ref, (long)an_id);
                SDfree_mapping_info(&map_info);
                status = SDendaccess(sdsid);
                if(status == FAIL)
                    FAIL_ERROR("SDendaccess() failed.");        
                return FAIL;
            }
            SDfree_mapping_info(&map_info);
        }
        status = SDendaccess(sdsid);
        if(status == FAIL)
            FAIL_ERROR("SDendaccess() failed.");        
    }

    return(status);
}


/*! 
  \fn write_map_sds(FILE *ofptr, int32 sdsid, int32 an_id, const char *path, 
                  SD_mapping_info_t *map_info, int indent,
                  ref_count_t *sd_visited)
  \brief  Write dataset information to mapped file.
  
  \return SUCCEED
  \return FAIL


  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date Aug 2, 2016
  \note removed ISO C90 warning.

  \date Oct 11, 2013
  \note added ref information for error message.


  \date March 29, 2012
  \note relaxed condition that fillValue element should
  appear whenever user-defined fill value is defined. fillValue element
  will no longer appear if rank > 0 and one of dimensions is unlimited
  (i.e. 0). This check is done by SDreaddata() return call.

  \date October 24, 2011
  \note 
  handled rank 0 with user defined value case.
  handled rank 0 with no user defined value case.

  \date October 19, 2011
  \note 
  changed the condition if(map_info->is_empty != 1 && rank != 0) to
  if(map_info->is_empty == 1 && rank != 0) because SDcheckempty() 
  is updated by Binh-Minh on October 9, 2011.
  does not return failure on SDreaddata() call for rank=1 and 
  size=unlimited case.

  \date October 7, 2011
  \note	"dataDimensionSizes" will appear only if rank > 0.

  \date October 6, 2011
  \note 
  initialized rank, ref, data_type, num_attrs to -1.
  added condition for checking rank != 0 before SDreaddata() for getting
  fill value. 
  changed nDimensions = 1 to 0 if (rank == 0).
  added conditionfor checking rank > 0 for writing verificaiton values.


  \date April 28, 2011
  \note 
  fixed return value for empty SDS case with no fill values.


  \date April 27, 2011
  \note 
  allowed an array to appear twice under different groups.

  \date April 26, 2011
  \note 
  removed the use of local variable for checking empty SDS.

  \date March 15, 2011
  \note 
  added more error handlers.

  \date March 11, 2011
  \note 
  moved checking compression information from write_array_data(). 
  changed return type to SUCCEED and FAIL.
  cleaned up error handling.
  removed num_errs variable.

*/
intn 
write_map_sds(FILE *ofptr, int32 sdsid, int32 an_id, const char *path, 
              SD_mapping_info_t *map_info, int indent,
              ref_count_t *sd_visited)
{

    static unsigned int ID_A = 0;

    char obj_name[H4_MAX_NC_NAME]; /* name of the object */
    
    int32 rank = -1;                 /* number of dimensions */
    int32 ref = -1;                  /* reference number  */
    int32 dimsizes[H4_MAX_VAR_DIMS]; /* dimension sizes */
    int32 data_type = -1;            /* data type */
    int32 num_attrs = -1;            /* number of global attributes */
    int32 chunk_flag = -1;        /* chunking flag */
    int32 numtype = 0;
    int32 eltsz = 0;

    int32* alloc_dimsizes = NULL; /* allocated dimension sizes */
    int32* picks = NULL;
    int32* stride = NULL;
    int32* edge = NULL;
    
    intn status = SUCCEED; /* status flag */
    
    int j = 0;
    int rank_temp = 0;
    
    unsigned int nDimensions = 0;

    HDF_CHUNK_DEF cdef;    
   
    ref = SDidtoref(sdsid);

    /* Get the reference number. */
    if(ref == FAIL){
        FAIL_ERROR("SDidtoref() failed.");
    }

    /* Save the reference to avoid generating the same array element
       in write_map_lone_sds().  */
    ref_count(sd_visited, ref);

    /* Save the SDS id. */
    map_info->id = sdsid;

    /* Check if SDS has no data. */
    status = SDcheckempty(sdsid, &(map_info->is_empty));
    if(status == FAIL){
        FAIL_ERROR("SDcheckempty() failed.");
    }

    status = SDgetinfo(sdsid, obj_name, &rank, dimsizes, &data_type, 
                       &num_attrs);
    if(status == FAIL){
        FAIL_ERROR("SDgetinfo() failed.");
    }

    /* Save the data type. */
    map_info->data_type = data_type;

    status = SDgetchunkinfo(sdsid, &cdef, &chunk_flag);
    if(status == FAIL){
        FAIL_ERROR("SDgetchunkinfo() failed.");
    }

    /* Handle named dimensions first if they exist. */
    status = set_dimension_no_data(sdsid, rank);
    if(status == FAIL){
        fprintf(flog, 
                "set_dimension_no_data() failed:sdsid=%ld rank=%ld.\n",
                (long)sdsid, (long)rank);
        return FAIL;
    }    

    /* Handle dimension variables differently using Dimension tag. */
    if(SDiscoordvar(sdsid)){ 
        /* Save the SDS id and process it later. */
        set_cv_list(sdsid);
        return SUCCEED;
    }



    /* Check if this object uses unsupported compression mechanism. */
    if(read_compression(map_info) == FAIL){
        fprintf(flog, "read_compression() failed.");
        fprintf(flog, ":path=%s, name=%s\n", path, obj_name);
        return FAIL;
    }

    if(HDstrlen(map_info->comp_info) > 0 && 
       strncmp(map_info->comp_info, "UNMAPPABLE:", 11) == 0){
        
        ++num_errs;
        fprintf(ferr, "%s:name=", map_info->comp_info);
        if(HDstrlen(path) == 1){
            fprintf(ferr, "%s%s\n", path, obj_name);
        } 
        else{
            fprintf(ferr, "%s/%s\n", path, obj_name);
        }
        return SUCCEED;
    }


    get_escape_xml(obj_name);   /* Clean XML reserved characters. */
    start_elm(ofptr, TAG_DSET, indent);

    nDimensions = (unsigned int)rank;


    fprintf(ofptr, "name=\"%s\" path=\"%s\" nDimensions=\"%d\" id=\"ID_A%d\">",
            obj_name, path, nDimensions, ++ID_A);

    /* Write annoation attributes. */
    status = write_array_attrs_an(ofptr, sdsid, an_id, indent+1);
    if(status == FAIL){
        fprintf(flog, 
                "write_array_attrs_an() failed:sdsid=%ld, ref=%ld, an_id=%ld.\n",
                (long)sdsid, (long)ref, (long)an_id);
        return FAIL;        
    }

    /* Write SDS attributes. */
    if (num_attrs > 0 ) {
        status = write_array_attrs(ofptr, sdsid, num_attrs, indent+1);
        if(status == FAIL){
            fprintf(flog, 
                    "write_array_attrs() failed:sdsid=%ld, nattrs=%ld.\n",
                    (long)sdsid, (long)num_attrs);
            return FAIL;                    
        }
    }

    /* Write SDS dimension sizes if rank > 0. */
    if (rank > 0) {
        start_elm_0(ofptr, TAG_DSPACE, indent+1);

        for (j=0; j < rank; j++){
            if(j != rank - 1){
                fprintf(ofptr, "%d ", (int)dimsizes[j]);
            }
            else{
                fprintf(ofptr, "%d", (int)dimsizes[j]);
            }
        }
        end_elm_0(ofptr, TAG_DSPACE);
    }


    /* Write the allocated dimension sizes if different. */
    if(chunk_flag){
        int flag = 0;
        /* Check if sizes can be evenly divided by chunk sizes. */
        for (j=0; j < rank; j++){
            int remainder = (int)dimsizes[j]%(int)cdef.chunk_lengths[j];
            if(remainder != 0){
                flag = 1;
                break;
            }
                
        }
        /* If not, get the closest multiple using the quotient + 1. */
        if(flag){
            alloc_dimsizes = HDmalloc(rank * sizeof(int32));
            if(alloc_dimsizes == NULL){
                FAIL_ERROR("HDmalloc(): Out of Memory.");
            }
            start_elm_0(ofptr, TAG_ASPACE, indent+1);
            for (j=0; j < rank; j++){
                int remainder = (int)dimsizes[j]%(int)cdef.chunk_lengths[j];
                if(remainder != 0){
                    int quotient = (int)dimsizes[j]/(int)cdef.chunk_lengths[j];
                    ++quotient;
                    alloc_dimsizes[j] =
                        (int32) quotient * cdef.chunk_lengths[j];
                }
                else{
                    alloc_dimsizes[j] = dimsizes[j];
                }
                fprintf(ofptr, "%d", (int) alloc_dimsizes[j] );                
                if(j != rank - 1){
                    fprintf(ofptr, " ");
                }
                
            }
            end_elm_0(ofptr, TAG_ASPACE);
            
        }
    }
    
    /* Write the dimension scale reference information. */
    write_dimension_ref(ofptr, sdsid, rank, indent+1);

    write_map_datum(ofptr, data_type, indent+1); 

    /* Read fill value if it exists. */
    numtype = data_type & DFNT_MASK;
    eltsz = DFKNTsize(numtype | DFNT_NATIVE);
    map_info->fill_value = (VOIDP) HDmalloc(eltsz);
    if(map_info->fill_value == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }
    HDmemset(map_info->fill_value, 0, eltsz);

    status = SDgetfillvalue(sdsid, map_info->fill_value);

    /* Read the fill value directly from dataset. 
       For rank 0 case, either a user or the HDF4 library supplied value 
       will be recorded as fill value.  

       The following is for old HDF4 data such as 200301h18ea-gdm.hdf. 
       The fill value in this case is the default value that HDF4 
       library provides, not the value from file.

       Such HDF4 library behavior originates from netcdf compatibility. 
    */
    if(map_info->is_empty == 1 || rank == 0){
        rank_temp = 0;
        if(rank == 0){
            /* Handle case for rank 0 yet user wrote something 
               accidentally which happens only on Ruth's 
               EmptyArrays3.hdf example case. */
            rank_temp = 1;  
        }
        else{
            rank_temp = rank;
        }
            
        picks = HDmalloc(rank_temp * sizeof(int32));
        stride = HDmalloc(rank_temp * sizeof(int32));
        edge = HDmalloc(rank_temp * sizeof(int32));
        for(j=0; j < rank_temp; j++){
            /* Pick the first point since all values are same. */
            picks[j] = 0;       
            stride[j] = 1;    
            edge[j] = 1;      
        }

        if(rank == 0){
            /* Without NULL for stride, SDreadata() causes seg. fault. */
            status = SDreaddata(sdsid, picks, NULL, edge, 
                                map_info->fill_value);
        }
        else{
            status = SDreaddata(sdsid, picks, NULL, edge, 
                                map_info->fill_value);
        }
        if(status == FAIL){
            HDfree(map_info->fill_value);
            /* 
               This is the case where rank > 0 and its size is unlimited 
               without any fill value. 
               See  MYD14.A2007273.0255.005.2007275112449.hdf test case.
            */
            map_info->fill_value = NULL;
        }
        HDfree(picks);
        HDfree(stride);
        HDfree(edge);
    }

    if(map_info->is_empty != 1 || map_info->fill_value != NULL) {
        /* Write offset/size information of data blocks. */
        if(alloc_dimsizes != NULL){
            status = write_array_data(ofptr, map_info, rank, alloc_dimsizes, 
                                      &cdef,chunk_flag, indent);
        }
        else{
            status = write_array_data(ofptr, map_info, rank, dimsizes, &cdef, 
                                      chunk_flag, indent);        
        }

        if(status == FAIL){
            HDfree(alloc_dimsizes);
            fprintf(flog, "write_array_data() failed on SDS dataset %s.\n",
                    obj_name);
            return FAIL;
        }
    }
    else {
        /* Reset status. This is empty SDS case with no fill value. */
        status = SUCCEED;
    }

    if(map_info->is_empty != 1) {
        /* Read and write verification values. */
        if(rank > 0){
            status = 
                write_verify_array_values(ofptr, sdsid, obj_name, rank, 
                                          dimsizes, data_type, indent+1);
        }
        else {                  
            /* There can be only one user value, which is recorded as
               map_info->fill_value in the previous step.
               See EmptyArrays3.hdf case. */
            start_elm(ofptr, "!--", indent+1);
            fprintf(ofptr, "value(s) for verification");
            indentation(ofptr, indent+2);
            fprintf(ofptr, "%s=", obj_name);                
            write_attr_value(ofptr, data_type, 1, map_info->fill_value,  
                             indent+1);           
            indentation(ofptr, indent+1);            
            fprintf(ofptr, "-->");
        }
        if(status == FAIL){
            fprintf(flog, "write_verify_array_values() failed.");
            fprintf(flog, ":name=%s, rank=%ld\n", obj_name, (long)rank);
            return FAIL;
        }
    }
    end_elm(ofptr, TAG_DSET, indent);

    if(alloc_dimsizes != NULL)
        HDfree(alloc_dimsizes);
    
    if(map_info->fill_value != NULL)
        HDfree(map_info->fill_value);
    return status;
}

/*!
  \fn write_mattrs(FILE *ofptr, intn indent)

  \brief Write the (merged) file attributes.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date Aug 3, 2016
  \note fixed the bug of skipping metadata for TRMM products.

  \date Aug 2, 2016
  \note removed ISO C90 warning.

  \date July 27, 2016
  \note checked attribute content before calling ODL parser.

  \date July 26, 2016
  \note added ODL parser.


  \date October 5, 2010
  \see set_mattrs()

*/
intn 
write_mattrs(FILE *ofptr, intn indent)
{
    int i = 0;

    char buffer[32];
    char new_name[16];

    char* str_trimmed = NULL;

    mattr_list_ptr tail = list_mattr;
    void *buf = NULL;
    
    while(tail != NULL){

        mattr* meta = &tail->m;

        if(meta == NULL){
            return FAIL;
        }

        if(meta->count > 0){
            start_elm(ofptr, TAG_FATTR, indent);

            if(meta->count == 1){
                fprintf(ofptr, "name=\"%s\"", meta->names[0]); 
                fprintf(ofptr, " origin=\"File Attribute: Arrays\""); 
            }
            else {
                strncpy(buffer, meta->names[0], strlen(meta->names[0]));
                buffer[strlen(meta->names[0])] = '\0';
                fprintf(ofptr,"name=\"%s\"", strtok(buffer,"."));
                fprintf(ofptr, " origin=\"File Attribute: Arrays\""); 
                fprintf(ofptr, " combinesMultipleAttributes=\"true\"");
            
            }
            fprintf(ofptr, " id=\"ID_FA%d\">", ++ID_FA);

            /* Write non-numeric datum element. */
            write_map_datum_char(ofptr, meta->type, indent+1);

            /* Write attribute data. */
            if(meta->count == 1){
                write_attr_data(ofptr, meta->offset[0], meta->length[0],
                                indent+1);

            }
            else{               
                /* Write the original attribute names. */
                start_elm_0(ofptr, TAG_ATTDATA, indent+1);

                start_elm_0(ofptr, TAG_BSTMSET, indent+2);
                for(i=0; i < meta->count; i++){
                    write_attr_data_meta(ofptr, meta->offset[i],
                                         meta->length[i], meta->names[i],
                                         indent+2);
                }
                end_elm(ofptr, TAG_BSTMSET, indent+2);
                end_elm(ofptr, TAG_ATTDATA, indent+1);
            }
            /* Trim if trim option is enabled. */
            str_trimmed = 
                get_trimmed_xml(meta->str, &meta->trimmed,
                                meta->total);

            if(meta->trimmed > 0){
                start_elm(ofptr, TAG_SVALUE, indent+1);
                fprintf(ofptr, "nNullsTrimmedFromEnd=\"%d\">", meta->trimmed);
            }
            else{
                start_elm_0(ofptr, TAG_SVALUE, indent+1);
            }
            if(str_trimmed != NULL) {
                fprintf(ofptr, "%s", str_trimmed);
            }
            end_elm_0(ofptr, TAG_SVALUE);
        
            end_elm(ofptr, TAG_FATTR, indent);

            /* Parse ODL. */
            if(parse_odl == 1){
                /* Get a consistent metadta name. */

                if(get_consistent_metadata_name(meta->names[0],
                                                new_name) == SUCCEED)
                    {
                        write_group(ofptr, new_name, "N/A", "", indent);
                        buf = odl_string(str_trimmed);
                        odlparse(ofptr);
                        end_elm(ofptr, TAG_GRP, indent);    
                    }
            }
            
            if(str_trimmed != NULL) {
                HDfree(str_trimmed);
            }
        
        }
        tail = tail->next;
    } /* while() */
    return SUCCEED;
}



/*!
  \fn write_verify_array_values(FILE *ofptr, int32 sds_id, char* name, 
  int32 rank, int32* dimsizes, int32 data_type, intn indent) 

  \brief  Write sample verification values in a comment block.

  \return FAIL
  \return SUCCEED

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date Aug 2, 2016
  \note removed ISO C90 warning.

  \date March 15, 2011
  \note 
  cleaned up error messages.
  fixed memory leaks.

  \date August 24, 2010
  
 */
intn 
write_verify_array_values(FILE *ofptr, int32 sds_id, char* name, 
                          int32 rank, int32* dimsizes, int32 data_type, 
                          intn indent)
{
    VOIDP fill_value = NULL;
    VOIDP data_value = NULL;

    int i = 0;
    int j = 0;
    int k = 0;
    int flag = 0;    
    int last = 0;
    int has_fill_value = 0;
    int count = 0;
    int count_corners = 0;
    int total_corners = 1 << (int)rank;
    int is_tried_already = 0;

    int32 eltsz = 0;
    int32 numtype = 0;
    int32 random = 0;
    
    int32* picks = NULL;
    int32* stride = NULL;
    int32* edge = NULL;

    intn status = SUCCEED;
    
    v_vals v_vals_trials[MAX_RAND_TRY];
    v_vals v_vals_picks[MAX_RAND_SAMPLES];
    v_vals* v_vals_corners = NULL;

    /* Generate random indexes. */
    /* Using sds_id will make indexes more random across different datasets. */
    unsigned int iseed = (unsigned int) sds_id;
    /* If you want a true random seed, use the following: */
    /* unsigned int iseed = (unsigned int)time(NULL)+(unsigned int) sds_id; */

    v_vals_corners = (v_vals*) HDmalloc( total_corners * sizeof(v_vals));
    if(v_vals_corners == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }
    /* Initialize arrays. */
    for(i=0; i < MAX_RAND_TRY; i++){
        v_vals_trials[i].picks = NULL;
        v_vals_trials[i].val = NULL;
    }

    for(i=0; i < MAX_RAND_SAMPLES; i++){
        v_vals_picks[i].picks = NULL;
        v_vals_picks[i].val = NULL;
    }

    numtype = data_type & DFNT_MASK;
    eltsz = DFKNTsize(numtype | DFNT_NATIVE);
    fill_value = (VOIDP) HDmalloc(eltsz);
    if(fill_value == NULL){
        HDfree(v_vals_corners);
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }

    /* Determine the fill value. */
    status = SDgetfillvalue(sds_id, fill_value);
    if(status != FAIL){
        has_fill_value = 1;
    }

    start_elm(ofptr, "!--", indent);
    fprintf(ofptr, "value(s) for verification");
    
    /* Generate corner point values. */
    for(i=0; i < total_corners ; i++){
        
        picks = HDmalloc(rank * sizeof(int32));
        if(picks == NULL){
            HDfree(fill_value);
            HDfree(v_vals_corners);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        
        stride = HDmalloc(rank * sizeof(int32));
        if(stride == NULL){
            HDfree(picks);
            HDfree(fill_value);
            HDfree(v_vals_corners);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        edge = HDmalloc(rank * sizeof(int32));
        if(edge == NULL){
            HDfree(stride);
            HDfree(picks);
            HDfree(fill_value);
            HDfree(v_vals_corners);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        
        is_tried_already = 0;
        
        for(j=0; j < rank; j++){
            if((i >> j) & 1){
                picks[j] = dimsizes[j]-1;
            }
            else{
                picks[j] = 0;
            }
            stride[j] = 1;
            edge[j] = 1;
        }
        
        /* Skip the indices that are already checked. */
        /* This can happen when one of dimsize is only 1. */
        for(k=0; k < count_corners; k++){

            if(v_vals_corners[k].picks != NULL){
                flag = 0;
                for(j=0; j < rank; j++){
                    if(v_vals_corners[k].picks[j] != picks[j]){
                        flag = 1;
                    }
                }
                if(flag == 0){      /* All three points are same. */
                    is_tried_already = 1;
                }
            }
            else {
                break;
            }

        }
        
        /* Read and save the value if it's not read already. */
        if(is_tried_already == 0){
            
            data_value = NULL;
            
            /* Allocate memory. */
            v_vals_corners[count_corners].picks =
                HDmalloc(rank * sizeof(int32));
            if(v_vals_corners[count_corners].picks == NULL){
                HDfree(edge);
                HDfree(stride);
                HDfree(picks);
                HDfree(fill_value);
                HDfree(v_vals_corners);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }
            v_vals_corners[count_corners].val = (VOIDP) HDmalloc(eltsz);
            if(v_vals_corners[count_corners].val == NULL){
                HDfree(v_vals_corners[count_corners].picks);
                HDfree(edge);
                HDfree(stride);
                HDfree(picks);
                HDfree(fill_value);
                HDfree(v_vals_corners);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            
            for(j=0; j < rank; j++){
                v_vals_corners[count_corners].picks[j] = picks[j];
            }

            
            data_value = (VOIDP) HDmalloc(eltsz);
            if(data_value == NULL){
                HDfree(v_vals_corners[count_corners].val);
                HDfree(v_vals_corners[count_corners].picks);
                HDfree(edge);
                HDfree(stride);
                HDfree(picks);
                HDfree(fill_value);
                HDfree(v_vals_corners);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            HDmemset(data_value, 0, eltsz);
            status = SDreaddata(sds_id, picks, stride, edge, data_value);
            if(status == FAIL){
                fprintf(flog, "SDreaddata() failed:name=%s\n", name);
                HDfree(data_value);
                HDfree(v_vals_corners[count_corners].val);
                HDfree(v_vals_corners[count_corners].picks);
                HDfree(edge);
                HDfree(stride);
                HDfree(picks);
                HDfree(fill_value);
                HDfree(v_vals_corners);
                return FAIL;
            }
            memcpy(v_vals_corners[count_corners].val, data_value, eltsz);
            
            ++count_corners;
            
            if(data_value != NULL)
                HDfree(data_value);
            
        }
        HDfree(edge);
        HDfree(stride);
        HDfree(picks);
    }

    /* Using second as a seed is not a strong PNG but it's OK for 
       verification purpose. */
    srand(iseed);
    /* Repeat MAX_RAND_TRY (=100 by default; configurable) times. */
    for(i=0; i < MAX_RAND_TRY && count < MAX_RAND_SAMPLES; i++){

        picks = NULL;
        stride = NULL;
        edge = NULL;
        is_tried_already = 0;

        picks = HDmalloc(rank * sizeof(int32));
        if(picks == NULL){
            HDfree(fill_value);
            HDfree(v_vals_corners);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        
        stride = HDmalloc(rank * sizeof(int32));
        if(stride == NULL){
            HDfree(picks);
            HDfree(fill_value);
            HDfree(v_vals_corners);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        edge = HDmalloc(rank * sizeof(int32));
        if(edge == NULL){
            HDfree(stride);
            HDfree(picks);
            HDfree(fill_value);
            HDfree(v_vals_corners);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        /* Pick indices based on random number. */
        for(j=0; j < rank; j++){
            random = (int32) rand();
            picks[j] = random % dimsizes[j];
            stride[j] = 1;
            edge[j] = 1;
        }

        /* Skip the corner values that are already checked. */
        for(k=0; k < count_corners; k++){

            if(v_vals_corners[k].picks != NULL){
                flag = 0;
                for(j=0; j < rank; j++){
                    if(v_vals_corners[k].picks[j] != picks[j]){
                        flag = 1;
                    }
                }
                if(flag == 0){      /* All three points are same. */
                    is_tried_already = 1;
                }
            }
            else {
                break;
            }

        }
        


        /* Skip the indices that are already checked. */
        for(k=0; k < last; k++){

            if(v_vals_trials[k].picks != NULL){
                flag = 0;
                for(j=0; j < rank; j++){
                    if(v_vals_trials[k].picks[j] != picks[j]){
                        flag = 1;
                    }
                }
                if(flag == 0){      /* All three points are same. */
                    is_tried_already = 1;
                }
            }
            else {
                break;
            }

        }

        if(is_tried_already == 0){
            data_value = NULL;
            /* Allocate memory. */
            v_vals_trials[last].picks = HDmalloc(rank * sizeof(int32));
            if(v_vals_trials[last].picks == NULL){
                HDfree(edge);
                HDfree(stride);
                HDfree(picks);
                HDfree(fill_value);
                HDfree(v_vals_corners);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }
            v_vals_trials[last].val = (VOIDP) HDmalloc(eltsz);
            if(v_vals_trials[last].val == NULL){
                HDfree(v_vals_trials[last].picks);
                HDfree(edge);
                HDfree(stride);
                HDfree(picks);
                HDfree(fill_value);
                HDfree(v_vals_corners);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
                
            }
            for(j=0; j < rank; j++){
                v_vals_trials[last].picks[j] = picks[j];
            }
            data_value = (VOIDP) HDmalloc(eltsz);
            if(data_value == NULL){
                HDfree(v_vals_trials[last].val);
                HDfree(v_vals_trials[last].picks);
                HDfree(edge);
                HDfree(stride);
                HDfree(picks);
                HDfree(fill_value);
                HDfree(v_vals_corners);
                FAIL_ERROR("HDmalloc() failed: Out of Memory");
            }

            HDmemset(data_value, 0, eltsz);
            status = SDreaddata(sds_id, picks, stride, edge, data_value);
            if(status == FAIL){
                fprintf(flog, "SDreaddata() failed:name=%s\n.", name);
                HDfree(data_value);
                HDfree(v_vals_trials[last].val);
                HDfree(v_vals_trials[last].picks);
                HDfree(edge);
                HDfree(stride);
                HDfree(picks);
                HDfree(fill_value);
                HDfree(v_vals_corners);
                return FAIL;
            }

            memcpy(v_vals_trials[last].val, data_value, eltsz);
            
            /* Search MAX_RAND_SAMPLES values that are not fill values
               excluding corners. */
            if(has_fill_value  == 1 &&
               is_diff(data_value, fill_value, data_type)){
                /* Save the index and the value. */
                v_vals_picks[count].picks = HDmalloc(rank * sizeof(int32));
                if(v_vals_picks[count].picks == NULL){
                    HDfree(data_value);
                    HDfree(v_vals_trials[last].val);
                    HDfree(v_vals_trials[last].picks);
                    HDfree(edge);
                    HDfree(stride);
                    HDfree(picks);
                    HDfree(fill_value);
                    HDfree(v_vals_corners);
                    FAIL_ERROR("HDmalloc() failed: Out of Memory");
                }
                v_vals_picks[count].val = (VOIDP) HDmalloc(eltsz);
                if(v_vals_picks[count].val == NULL){
                    HDfree(v_vals_picks[count].picks);
                    HDfree(data_value);
                    HDfree(v_vals_trials[last].val);
                    HDfree(v_vals_trials[last].picks);
                    HDfree(edge);
                    HDfree(stride);
                    HDfree(picks);
                    HDfree(fill_value);
                    HDfree(v_vals_corners);
                    FAIL_ERROR("HDmalloc() failed: Out of Memory");
                }


                for(j=0; j < rank; j++){
                    v_vals_picks[count].picks[j] = picks[j];
                    memcpy(v_vals_picks[count].val, data_value, eltsz);
                }
                ++count;
            }
            ++last;
            if(data_value != NULL)
                HDfree(data_value);
            
        }
        HDfree(picks);
        HDfree(stride);
        HDfree(edge);
    }  /*  for(i=0; i < MAX_RAND_TRY && count < MAX_RAND_SAMPLES; i++) */

    /* Write corner values first. */
    for(i=0; i < count_corners; i++){
        indentation(ofptr, indent+1);

        if(strncmp(name, "fakeDim", 7)){
            fprintf(ofptr, "%s[", name);    
        }
        else{         /* If fakeDim, skip writing name. */
            fprintf(ofptr, "[");
        }

        for(j=0; j < rank; j++){
            if(j == rank-1){
                fprintf(ofptr, "%ld", (long) v_vals_corners[i].picks[j]);    
            }
            else {
                fprintf(ofptr, "%ld,", (long) v_vals_corners[i].picks[j]);    
            }
        }
        fprintf(ofptr, "]=");    
        write_attr_value(ofptr, data_type, 1, v_vals_corners[i].val,
                         indent);
                        
    }
    /* Write random points.  */
    if(count == 0){
        /* If everything is fill value, just write MAX_RAND_SAMPLES random
           values. */        
#ifdef DEBUG        
        fprintf(ofptr, "\nAll values are fill values for %s\n", name);
#endif        
        for(i=0; i < last && i < MAX_RAND_SAMPLES ; i++){
            indentation(ofptr, indent+1);

            if(strncmp(name, "fakeDim", 7)){
                fprintf(ofptr, "%s[", name);    
            }
            else{
                fprintf(ofptr, "[");    
            }

            for(j=0; j < rank; j++){
                if(j == rank-1){
                    fprintf(ofptr, "%ld", (long)v_vals_trials[i].picks[j]);    
                }
                else {
                    fprintf(ofptr, "%ld,",(long)v_vals_trials[i].picks[j]);    
                }
            }
            fprintf(ofptr, "]=");    
            
            write_attr_value(ofptr, data_type, 1, v_vals_trials[i].val,
                             indent);
        }
        
    }
    else {
        /* Write MAX_RAND_SAMPLES values that are not fill values. */
        for(i=0; i < count; i++){
            indentation(ofptr, indent+1);
            fprintf(ofptr, "%s[", name);    
            for(j=0; j < rank; j++){
                if(j == rank-1){
                    fprintf(ofptr, "%ld", (long)v_vals_picks[i].picks[j]);    
                }
                else {
                    fprintf(ofptr, "%ld,",(long)v_vals_picks[i].picks[j]);    
                }
            }
            fprintf(ofptr, "]=");    
            write_attr_value(ofptr, data_type, 1, v_vals_picks[i].val,
                             indent);
                        
        }
    }

    indentation(ofptr, indent);
    fprintf(ofptr, "-->");
    
    
    HDfree(fill_value);

    /* Free arrays. */
    for(i=0; i < count_corners; i++){
        HDfree(v_vals_corners[i].picks);
        HDfree(v_vals_corners[i].val);
    }

    
    for(i=0; i < last; i++){
        HDfree(v_vals_trials[i].picks);
        HDfree(v_vals_trials[i].val);
    }

    for(i=0; i < count; i++){
        HDfree(v_vals_picks[i].picks);
        HDfree(v_vals_picks[i].val);
    }
    if(v_vals_corners != NULL)
        HDfree(v_vals_corners);
    return status;
}
