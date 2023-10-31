/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010  The HDF Group.                                      *
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

  \file xml_sds.h
  \brief Have the structures about HDF4 SDS objects that will be mapped to XML
         elements.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 5, 2010


 */

#ifndef _XML_SDS_H_
#define _XML_SDS_H_
#define MAX_RAND_TRY 100        /*!< Maximum number of random trials */
#define MAX_RAND_SAMPLES 5      /*!< Number of  samples for verification */


/*!
  \struct cv_list
  \brief Keep track of SDS ids of coordinate variable data.
*/
/*!
  \typedef cv_list_elmt
  \brief cv_list element.
*/
/*!
  \typedef cv_list_ptr
  \brief cv_list pointer.
*/

typedef struct cv_list {
    int32 id;                   /*!< SDS id of coordinate variable.*/
    struct cv_list *next;       /*!< pointer to next element */
} cv_list_elmt, *cv_list_ptr; 


/*!
  \struct did2sid
  \brief Associate dimension id with SDS id.

  This is required for retrieving old dimension attribute offset and length
  using SDgetoldattdatainfo() call.

 */
typedef struct {
    int32 did;                  /*!< Dimension id  */
    int32 sid;                  /*!< SDS id */
} did2sid;

/*!
  \struct did2sid_list
  \brief Keep track of dimension name and XML id attribute value pair.
 */
/*!
  \typedef did2sid_list_elmt
  \brief did2sid_list element.
*/
/*!
  \typedef did2sid_list_ptr
  \brief did2sid_list pointer.
*/
typedef struct did2sid_list {
    did2sid elmt;                 /*!< dimension name to XML ID structure */
    struct did2sid_list *next;    /*!< pointer to the next element   */
} did2sid_list_elmt, *did2sid_list_ptr;



/*!
  \struct dn2id
  \brief Associate dimension name with XML ID attribute value.
 */
typedef struct {
    char name[H4_MAX_NC_NAME];  /*!< dimension name */
    unsigned int id;            /*!< XML ID attribute value */
    unsigned int written; /*!< flag to indicate whether it's written or not.  */
} dn2id;



/*!
  \struct dn2id_list
  \brief Keep track of dimension name and XML id attribute value pair.
 */
/*!
  \typedef dn2id_list_elmt
  \brief dn2id_list element.
*/
/*!
  \typedef dn2id_list_ptr
  \brief dn2id_list pointer.
*/
typedef struct dn2id_list {
    dn2id elmt;                 /*!< dimension name to XML ID structure */
    struct dn2id_list *next;    /*!< pointer to the next element   */
} dn2id_list_elmt, *dn2id_list_ptr;


/*!
  \struct dn_list
  \brief Keep track of dimension ids of named dimensions.
 */
/*!
  \typedef dn_list_elmt
  \brief dn_list element.
*/
/*!
  \typedef dn_list_ptr
  \brief dn_list pointer.
*/
typedef struct dn_list {
    int32 id;                   /*!< SDS dimension id of dimension name */
    struct dn_list *next;       /*!< pointer to the next element */
} dn_list_elmt, *dn_list_ptr;


/*!
  \struct v_vals
  \brief Hold verification value and its indexes.
 */
typedef struct {
    int32* picks;   /*!< indices of random picks */
    VOIDP  val; /*!< pointer to data value */
} v_vals;

#ifdef __cplusplus
extern "C" {
#endif

void 
free_cv_list();

void 
free_did2sid_list();

void 
free_dn_list();

void 
free_dn2id_list();

intn 
get_adjusted_attr_datainfo(char* name, int32 *offset, int32 *length); 


int32 
get_did2sid_list(int32 did);

unsigned int 
get_dn2id_list_id(char* name);

unsigned int 
get_dn2id_list_written(char* name);

char* 
get_trimmed_xml(char *s, int* trimmed, int count);

int
is_diff(VOIDP data, VOIDP fill, int32 data_type);


unsigned int 
set_cv_list(int32 id);

void
set_did2sid_list(int32 did, int32 sid);

unsigned int 
set_dn_list(int32 id);

void 
set_dn2id_list_id(char* name, unsigned int id);

unsigned int 
set_dn2id_list_written(char* name);

    
intn 
write_array_attrs(FILE *ofptr, int32 sd_id, int32 nattrs, intn indent);

void
write_array_attribute(FILE *ofptr, int32 attr_nt, int32 attr_count, 
                      VOIDP attr_buf, int32 attr_buf_size,
                      int32 offset, int32 length, intn indent);

intn 
write_array_byte_stream(FILE *ofptr, SD_mapping_info_t *map_info, intn indent);


intn 
write_array_chunks(FILE *ofptr, SD_mapping_info_t *map_info, int32 rank,
                   int32* dimsizes, HDF_CHUNK_DEF* cdef, intn indent);

intn 
write_array_chunks_byte_stream(FILE *ofptr, SD_mapping_info_t *map_info, 
                               int32 rank, int32* dimsizes, 
                               HDF_CHUNK_DEF* cdef,intn indent);

intn 
write_array_data(FILE *ofptr, SD_mapping_info_t *map_info, int32 rank,
                 int32* dimsizes, HDF_CHUNK_DEF* cdef, int32 chunk_flag, 
                 intn indent);

void 
write_attr_data_meta(FILE *ofptr, int32 offset, int32 length, char* name,
                     int indent);

void 
write_chunk_position_in_array(FILE* ofptr, int rank, int32* lengths, 
                              int32* strides, int tag_close);

intn 
write_dimension(FILE *ofptr, int32 sdsid, char* obj_name, int32 data_type,
                int32 rank, int32* dimsizes, int32 num_attrs,
                SD_mapping_info_t* map_info, intn indent);

intn 
write_dimension_attrs(FILE *ofptr, int32 sd_id, int32 nattrs, intn indent);

intn 
write_dimension_data(FILE *ofptr, SD_mapping_info_t *map_info, intn indent);

intn
write_dimension_ref(FILE *ofptr, int32 sds_id, int32 rank, intn indent);

intn 
write_file_attrs(FILE *ofptr, int32 sd_id, int32 nattrs, intn indent);

intn 
write_map_dimension_no_data(FILE *ofptr);

intn
write_map_dimensions(FILE *ofptr);

int 
write_map_lone_sds(FILE *ofptr, char *infile, int32 sd_id, int32 an_id,
                   ref_count_t *sd_visited);

intn 
write_map_sds(FILE *ofptr, int32 sdsid, int32 an_id, const char *path,
              SD_mapping_info_t *map_info, int indent,
              ref_count_t *sd_visited);

intn
write_mattrs(FILE *ofptr, intn indent);


intn 
write_verify_array_values(FILE *ofptr, int32 sds_id, char* name, int32 rank, 
                          int32* dimsizes, int32 data_type, intn indent);




#ifdef __cplusplus
}

#endif

#endif /* _XML_SDS_H_ */
