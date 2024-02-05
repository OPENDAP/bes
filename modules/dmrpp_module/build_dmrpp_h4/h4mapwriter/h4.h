/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2014  The HDF Group                                    *
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

  \file h4.h
  \brief Have the structures about HDF4 objects that will be mapped to XML
         elements.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 7, 2014
  \note updated comments to remove doxygen warnings.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 9, 2013
  \note reduced the number of function arguments by mering tag 201 and 700.


  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note updates related to palette handling
  deleted N_PAL_ENTRIES, N_PAL_COMPONENTS, PAL_info_t

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date May 5, 2011
  \note added the check_tag_expected() function header.

  \date  April 28, 2011
  \note removed read_fill_value().

  \date October 13, 2010
  \note documented in Doxygen style.

  \author Binh-Minh Ribler (bmribler@hdfgroup.org)
  \author Peter Cao (xcao@hdfgroup.org)

 */

// Added jhrg 12/6/23
#define uint unsigned int

#define COMP_INFO 512 /*!< Max buffer size for compression information.  */
#define N_PAL_VALUES      768   /*!< Number of Palette values. */

/*!
  \struct PAL_DD_info_t
  \brief Hold needed info for palette DDs

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note created.

 */
typedef struct {
    uint16  tag;                /*!< tag of palette */
    uint16  ref;                /*!< ref of palette  */
    uint    elementID;          /*!< ID of palette element in map file */
} PAL_DD_info_t;                

/*!

  \struct PAL_DE_info_t
  \brief Hold needed info for palette DEs

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note created.

 */
typedef struct {
    uint    offset;             /*!< offset of palette data in h4 file */
    uint8   data[N_PAL_VALUES]; /*!< palette data; each value 1 byte long */
} PAL_DE_info_t;

/*!

  \struct RIS_info_t
  \brief Hold number of images and their tag/refs.

 */
typedef struct {
    int32   nimages;            /*!< number of raster images */
    int32   *tags;              /*!< tags of raster images */
    int32   *refs;              /*!< refs of raster images */
} RIS_info_t;

/*! 

  \struct RIS_mapping_info_t
  \brief Hold mapping information for Raster Images

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note
  replaced PAL_info_t *pal with uint pal_ref.

  \author Hyo-Kyung Joe Lee (hyoklee@hdfgroup.org)

  \date March 18, 2010
  \note 
  added is_r8.
  changed type of is_mappable to intn.

  \date March 17, 2010
  \note 
  added num_attrs.

  \date March 14, 2010
  \note 
  added is_mappable.


*/
typedef struct {
    char       comp_info[COMP_INFO]; /*!< compression information */
    char       name[H4_MAX_NC_NAME]; /*!< name of raster iamge  */
    intn       is_mappable;          /*!< whether it can be mapped or not  */
    intn       is_r8;                /*!< whether it is RIS8 or GR  */
    int32      ncomp;                /*!< number of components */
    int32      interlace;            /*!< data ordering:chunky, planar, etc. */
    int32      dimsizes[2];          /*!< dimension sizes of the image */
    int32      nt;                   /*!< number type of data */
    int32      nblocks;              /*!< number of data blocks */
    int32      *offsets;             /*!< offsets of data blocks */
    int32      *lengths;             /*!< lengths (in bytes) of data blocks */
    int32      num_attrs;            /*!< number of attributes  */
    intn       npals;                /*!< number of palettes (0 or 1) */
    uint16     pal_ref;              /*!< ref for palette */
    
} RIS_mapping_info_t;

/*!

  \struct SD_mapping_info_t
  \brief Hold mapping information for SDS objects.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)  

  \date April 27, 2011
  \note added data_type member.

  \date April 26, 2011
  \note added is_empty and fill_value.

*/
typedef struct {
    char comp_info[COMP_INFO];  /*!< compression information */
    int32 nblocks;              /*!< number of data blocks in dataset */
    int32 *offsets;             /*!< offsets of data blocks */
    int32 *lengths;             /*!< lengths (in bytes) of data blocks */
    int32 id;                   /*!< SDS id  */
    int32 data_type;            /*!< data type */
    intn is_empty;              /*!< flag for checking empty  */
    VOIDP fill_value;           /*!< fill value  */

} SD_mapping_info_t;

/*! 

  \struct VS_mapping_info_t
  \brief Hold mapping information for VData 

*/
typedef struct {
    char **fnames;              /*!< Vdata field names  */
    char name[VSNAMELENMAX+1];  /*!< Vdata name  */
    char class[VSNAMELENMAX+1]; /*!< Vdata class  */    

    int ncols;                  /*!< number of columns */
    int is_old_attr;            /*!< flag for old attribute.  */

    int16 sorder;               /*!< storage order */
    int32 *ftypes;              /*!< Vdata field types  */

    int32 nrows;                /*!< number of rows */
    int32 nblocks;              /*!< number of data blocks in the dataset */
    int32 *offsets;             /*!< offsets of data blocks */
    int32 *lengths;             /*!< lengths (in bytes) of data blocks */

    int32 *forders;             /*!< Vdata field orders  */

} VS_mapping_info_t;

/*!

  \struct ref_count_t
  \brief  Hold the references of the visited objects.

*/
typedef struct {
    int32 size;                 /*!< the current size of refs array */
    int32 max_size;             /*!< maximum size of the refs array */
    int32 *refs;                /*!< array that holds the ref values */
} ref_count_t;


/*!
  \struct mattr
  \brief Keep track of merged attribute.

 */
typedef struct mattr {
    char *name;                 /*!< merged name */
    char *str;                  /*!< pointer to merged metadata value */
    char **names;               /*!< individual original names  */
    int count;                  /*!< number of attributes merged so far */
    int trimmed;                /*!< number of null characters trimmed */
    int *length;                /*!< offsets */
    int *offset;                /*!< lengths */
    int  start;                 /*!< starting suffix  */
    int32 type;                 /*!< attribute type */
    int32 total;                /*!< number of bytes in str so far */
} mattr;                        /*!<  merged attribute type */


/*!
  \struct mattr_list
  \brief Keep track of all merged attributes.

  This list is mainly used for SDS file attributes.

 */
/*!
  \typedef mattr_list_elmt
  \brief mattr_list element.
*/
/*!
  \typedef mattr_list_ptr
  \brief mattr_list pointer.
*/
typedef struct mattr_list {
    mattr m;                    /*!< merged attribute element  */
    struct mattr_list *next;    /*!< pointer to the next element */
} mattr_list_elmt, *mattr_list_ptr;



#ifdef __cplusplus
extern "C" {
#endif

/* functions in h4.c  */

/*! 
  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note 
  signature changed for read_all_refs()
*/
  
  

int 
check_nt_unexpected(int32, uint16, int32, int32 );

int
check_tag_expected(uint16 tag, 
                   int DIL_annHandled_seen, 
                   int DIL_annOther_seen,
                   int DIA_annHandled_seen, 
                   int DIA_annOther_seen,
                   int ID_unexpected_cnt);

int 
is_little_endian();

int 
is_string(int32 type);

mattr_list_ptr 
has_mattr(char *name);

void 
free_mattr_list();

int 
read_all_refs(int32 file_id, FILE* filep,
              ref_count_t *obj_missed, ref_count_t *sd_visited, 
              ref_count_t *vs_visited, ref_count_t *vg_visited, 
              ref_count_t *ris_visited, 
              ref_count_t *palIP8_visited, 
              ref_count_t *palLUT_visited,
              int* ignore_tags, int ignore_cnt);
void 
read_bytes(FILE* filep, int offset, int length, uint8* buffer);

unsigned int 
set_mattr(char *attr_name, int32 attr_type, VOIDP attr_buf,
          int32 attr_buf_size, int32 offset, int32 length, int32 nattr);

void 
swap_2bytes(uint8 *bytes);

void 
swap_4bytes(uint8 *bytes);


/* fucntions in h4_sds.c */
intn 
SDfree_mapping_info (SD_mapping_info_t  *map_info);

intn 
SDmapping (int32  sdsid, SD_mapping_info_t  *map_info);


intn
read_chunk(int32 sdsid, SD_mapping_info_t *map_info, int32* index);

intn
read_compression(SD_mapping_info_t *map_info);

intn
set_dimension_no_data(int32 sdsid, int32 rank);

/* functions in h4_vdata.c */
 intn 
VSfree_mapping_info(VS_mapping_info_t  *map_info);

intn 
VSmapping(int32  vdata_id, VS_mapping_info_t  *map_info);

/* functions in h4_ris.c */
intn 
RISfree_mapping_info(RIS_mapping_info_t  *map_info);

intn 
RISget_refs(int32 file_id, RIS_info_t *ris_info);

intn 
RISfree_ris_info(RIS_info_t  *ris_info);

intn 
RISmapping(int32 file_id, int32 ri_id, RIS_mapping_info_t *map_info);

/* functions in h4_pal.c */
/*! 

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note 
  read_pals() replaced read_pal()
  added free_pal_arrays()

*/
int
read_pals(char *h4filename, int32 file_id, int32 gr_id);

void
free_pal_arrays( void );



#ifdef __cplusplus


#endif
