/*
 * FILENAME: ndarray.h
 *
 *  Structure and function declarations for ndarray.c
 *
 * MACROS:
 *
 * NDARR_RESET_INDICES(ARRAY_INDEX_PTR) 
 *		resets the indices to the origion.
 * NDARR_SET_PADDING(NDARR_SOURCE, char)
 *		sets the padding character.
 * NDARR_GET_SEPARATION(ARRAY_INDEX_PTR, long)
 *		determines what length of separation is necessary after the indexed element.
 * NDARR_GET_MAP_SEPARATION(ARRAY_MAPPING_PTR, long)
 *		determines what length of separation is necessary after the mapping increment
 *		block.
 *
 * CAVEAT:
 * No claims are made as to the suitability of the accompanying
 * source code for any purpose.  Although this source code has been
 * used by the NOAA, no warranty, expressed or implied, is made by
 * NOAA or the United States Government as to the accuracy and
 * functioning of this source code, nor shall the fact of distribution
 * constitute any such endorsement, and no responsibility is assumed
 * by NOAA in connection therewith.  The source code contained
 * within was developed by an agency of the U.S. Government.
 * NOAA's National Geophysical Data Center has no objection to the
 * use of this source code for any purpose since it is not subject to
 * copyright protection in the U.S.  If this source code is incorporated
 * into other software, a statement identifying this source code may be
 * required under 17 U.S.C. 403 to appear with any copyright notice.
 *
 */

#ifndef NDARRAY_H__
#define NDARRAY_H__

typedef unsigned short NDARR_SOURCE;

/***************************
 * Structure declarations: *
 ***************************/
typedef struct array_descriptor_struct{
	char **dim_name;     /* names of each dimension */
	long *start_index;   /* array of dimension starting indices */
	long *end_index;     /* array of dimension ending indices */
	long *granularity;   /* array of dimension granularity */
	long *grouping;      /* array of dimension grouping */
	long *separation;    /* array of dimension speration (by bytes) */
	char *index_dir;     /* array of index directions (down or up) */
	long *dim_size;      /* An array of the size of each dimension */
	long *coeffecient;   /* A number necessary for calculating offsets */
	void *extra_info;    /* An extra pointer *RESERVED!* */
	void *extra_index;   /* A pointer to the groupmap index *RESERVED!* */
	long total_elements; /* the number of elements in the array */
	long num_groups;     /* The number of groups in the array */
	long group_size;   /* The size of an individual group */
	long contig_size;  /* The size, in bytes, of the array contiguous (no separation) */
	long total_size;   /* The size, in bytes, of the whole array */
	long element_size; /* The size of an individual element */
	int num_dim;         /* number of dimensions */
	char type;           /* The type of the array *RESERVED!* */

#ifdef ND_FP 
	FILE *fp;
#endif
} ARRAY_DESCRIPTOR, *ARRAY_DESCRIPTOR_PTR;

typedef struct array_index_struct{
	ARRAY_DESCRIPTOR_PTR descriptor; /* ptr to array descriptor */
	long *index; /* The indices of the array */
} ARRAY_INDEX, *ARRAY_INDEX_PTR;

typedef struct array_mapping_struct{
	ARRAY_DESCRIPTOR_PTR super_array; /* superset of the sub-array */
	ARRAY_DESCRIPTOR_PTR sub_array; /* subset of the super-array */
	int *dim_mapping; /* dimension mapping from sub-array to super-array */
	long *index_mapping; /* index mapping from sub-array to super-array */
	long *gran_mapping; /* Granularity mapping from sub to super array */
	long *gran_div_mapping; /* Granularity mapping from super to sub array */
	long *cacheing; /* For use with ndarr_reorient_to_cache */
	char *index_dir; /* Direction of mapping from sub-array to super-array */
	ARRAY_INDEX_PTR aindex; /* Extra index pointer (for use in calculations)*/
	ARRAY_INDEX_PTR subaindex; /* The current sub-array indices */
	long increment_block; /* size of elements readable at one time */
	long superastart; /* The first offset in superarray for subarray */
	long superaend; /* The last offset in superarray for subarray */
	long subsep; /* the maximum padding length in the sub array */
	int necessary;  /* 0 if mapping is not necessary, !=0 if mapping necessary */
	int dimincrement; /* The lowest unreoriented dimension */
	char fcreated; /* Reserved for use in ndarr_reorient */
} ARRAY_MAPPING, *ARRAY_MAPPING_PTR;

/**************************
 * Function declarations: *
 **************************/
ARRAY_DESCRIPTOR_PTR ndarr_create(int numdim);
ARRAY_MAPPING_PTR    ndarr_create_mapping(ARRAY_DESCRIPTOR_PTR subarray,
							ARRAY_DESCRIPTOR_PTR superarray);
ARRAY_INDEX_PTR      ndarr_create_indices(ARRAY_DESCRIPTOR_PTR arrdesc);
ARRAY_INDEX_PTR      ndarr_increment_indices(ARRAY_INDEX_PTR aindex);
ARRAY_INDEX_PTR      ndarr_increment_mapping(ARRAY_MAPPING_PTR amap);
ARRAY_INDEX_PTR      ndarr_convert_indices(ARRAY_INDEX_PTR aindex,
							unsigned char direction);
unsigned long        ndarr_get_offset(ARRAY_INDEX_PTR aindex);
unsigned long        ndarr_get_mapped_offset(ARRAY_MAPPING_PTR amap);
void                 ndarr_free_descriptor(ARRAY_DESCRIPTOR_PTR arrdesc);
void                 ndarr_free_indices(ARRAY_INDEX_PTR aindex);
void                 ndarr_free_mapping(ARRAY_MAPPING_PTR amap);
void                *ndarr_get_group(ARRAY_INDEX_PTR aindex);
void                *ndarr_get_next_group(ARRAY_DESCRIPTOR_PTR arrdesc, char mode);
int                  ndarr_do_calculations(ARRAY_DESCRIPTOR_PTR arrd);
int                  ndarr_create_brkn_desc(ARRAY_DESCRIPTOR_PTR adesc, int map_type, void *mapping);
int                  ndarr_set(ARRAY_DESCRIPTOR_PTR arrd, ...);
long                 ndarr_reorient(ARRAY_MAPPING_PTR amap,
							NDARR_SOURCE sourceid,  void *source,  long source_size,
							NDARR_SOURCE destid,    void *dest,    long dest_size,
							int *array_complete);

/*************************
 * Preprocessor defines: *
 *************************/

/* ndarr_set arguments: */
#define NDARR_DIM_NUMBER      (int)1
#define NDARR_DIM_NAME        (int)2
#define NDARR_DIM_START_INDEX (int)3
#define NDARR_DIM_END_INDEX   (int)4
#define NDARR_DIM_GRANULARITY (int)5
#define NDARR_DIM_GROUPING    (int)6
#define NDARR_DIM_SEPARATION  (int)7
#define NDARR_ELEMENT_SIZE    (int)10
#define NDARR_FILE_GROUPING   (int)20
#define NDARR_BUFFER_GROUPING (int)21
#define NDARR_MAP_IN_BUFFER   (int)22
#define NDARR_MAP_IN_FILE     (int)23
#define NDARR_END_ARGS        (int)0

/* ndarr_create_from_str tokens: */
#define NDARR_SB_KEY0 "sb"
#define NDARR_SB_KEY1 "separation"
#define NDARR_GB_KEY0 "gb"
#define NDARR_GB_KEY1 "grouping"

/* ndarr_convert_indices arguments */
#define NDARR_USER_TO_REAL 'r'
#define NDARR_REAL_TO_USER 'u'

/* Array types */
#define NDARRT_CONTIGUOUS    0
#define NDARRT_BROKEN        1
#define NDARRT_GROUPMAP_FILE 2
#define NDARRT_GROUPMAP_BUFF 3

/* ndarr_get_next_group arguments */
#define NDARR_GINITIAL 0
#define NDARR_GNEXT    1

/* Define array storage bitfields */
#define NDARRS_FILE    (NDARR_SOURCE)0x8000
#define NDARRS_APPEND  (NDARR_SOURCE)0x4000
#define NDARRS_UPDATE  (NDARR_SOURCE)0x2000
#define NDARRS_CREATE  (NDARR_SOURCE)0x1000
#define NDARRS_BUFFER  (NDARR_SOURCE)0x0800
#define NDARRS_PADDING (NDARR_SOURCE)0x00FF

/* If we are under freeform, use freeform's err functions */
/******************
 * Define macros: *
 ******************/

/*
 * NAME:	NDARR_RESET_INDICES
 *
 * PURPOSE:	To reset an ARRAY_INDEX struct so that its indices are all 0 (the origion)
 *
 * USAGE:   void NDARR_RESET_INDICES(ARRAY_INDEX_PTR aindex)
 *
 * RETURNS: void
 *
 * DESCRIPTION: This is implmented as a macro, due to its extremely small size.
 *
 * SYSTEM DEPENDENT FUNCTIONS: none
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS: Implemented as a macro
 *
 * KEYWORDS: array macro
 */
#define NDARR_RESET_INDICES(nda_i_p) {                                           \
	int nda_d_s;                                                                 \
	for(nda_d_s = 0; nda_d_s < nda_i_p->descriptor->num_dim; nda_d_s++)          \
		nda_i_p->index[nda_d_s] = 0;}                                            \

/*
 * NAME:	NDARR_SET_PADDING
 *
 * PURPOSE:	To set the padding portion of an NDARR_SOURCE type variable.
 *
 * USAGE:   void NDARR_SET_PADDING(NDARR_SOURCE svar, char paddingchar)
 *
 * RETURNS: void
 *
 * DESCRIPTION: This is implmented as a macro, due to its extremely small size.
 *			Sets the padding portion of the NDARR_SOURCE variable to paddingchar.
 *
 * SYSTEM DEPENDENT FUNCTIONS: none
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS: Implemented as a macro
 *
 * KEYWORDS: array macro
 */
#define NDARR_SET_PADDING(nda_s_v, pad_c_v) {                                    \
	nda_s_v &= ~NDARRS_PADDING;                                                  \
	nda_s_v |= (NDARRS_PADDING & (pad_c_v * 257));}                              \

/*
 * NAME:	NDARR_GET_SEPARATION
 *
 * PURPOSE:	To determine the length (in bytes) of the separation to follow the 
 *			element with the given index.
 *
 * USAGE:   void NDARR_GET_SEPARATION(ARRAY_INDEX_PTR aindex, long separation)
 *
 * RETURNS: void, but the separation length (in bytes) is stored in the long
 *			variable argument.
 *
 * DESCRIPTION: This is implmented as a macro, due to its relatively small size,
 *				and possible speed gains from inline code (this macro gets called
 *				frequently inside of big loops)
 *
 *				Determines the number of bytes to come after the element named
 *				in the ARRAY_INDEX_PTR as separation.
 *
 * SYSTEM DEPENDENT FUNCTIONS: none
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS: Implemented as a macro
 *
 * KEYWORDS: array macro
 */
#define NDARR_GET_SEPARATION(nda_i_p, nda_l_v) { \
	int nda_t_i; \
	nda_l_v = nda_i_p->descriptor->separation[nda_i_p->descriptor->num_dim - 1]; \
	for(nda_t_i = nda_i_p->descriptor->num_dim - 2; nda_t_i >= 0; nda_t_i--) { \
		if(nda_i_p->descriptor->grouping[nda_t_i + 1]) { \
			if(!((nda_i_p->index[nda_t_i + 1] + 1) % \
			     nda_i_p->descriptor->grouping[nda_t_i + 1])) { \
				nda_l_v += nda_i_p->descriptor->separation[nda_t_i]; \
                        } \
			else { \
				break; \
                        } \
                } \
		else { \
			if(((nda_i_p->index[nda_t_i + 1] + 1) == \
			    nda_i_p->descriptor->dim_size[nda_t_i + 1])) { \
				nda_l_v += nda_i_p->descriptor->separation[nda_t_i]; \
			} \
			else { \
				break; \
                        } \
                } \
	}\
} \


/*
 * NAME:	NDARR_GET_MAP_SEPARATION
 *
 * PURPOSE:	To determine the length (in bytes) of the separation to follow the 
 *			array mapping increment starting with amap->subaindex.
 *
 * USAGE:   void NDARR_GET_MAP_SEPARATION(ARRAY_MAPPING_PTR amap, long separation)
 *
 * RETURNS: void, but the separation length (in bytes) is stored in the long
 *			variable argument.
 *
 * DESCRIPTION: This is implmented as a macro, due to its relatively small size,
 *				and possible speed gains from inline code (this macro gets called
 *				frequently inside of big loops)
 *
 *				Determines the number of bytes to come after the amap->increment_block
 *				whose starting index is specified by amap->subaindex.
 *
 * SYSTEM DEPENDENT FUNCTIONS: none
 *
 * AUTHOR:      Kevin Frender (kbf@ngdc.noaa.gov)
 *
 * COMMENTS: Implemented as a macro
 *
 * KEYWORDS: array macro
 */
#define NDARR_GET_MAP_SEPARATION(nda_m_p, nda_l_v) {                             \
	int nda_t_i;                                                                 \
	ARRAY_INDEX_PTR nda_i_p = nda_m_p->subaindex;                                \
	nda_l_v = nda_i_p->descriptor->separation[nda_m_p->dimincrement];            \
	for(nda_t_i = nda_m_p->dimincrement - 1; nda_t_i >= 0; nda_t_i--)            \
	{                                                                            \
		if(nda_i_p->descriptor->grouping[nda_t_i + 1])                           \
		{                                                                        \
			if(!((nda_i_p->index[nda_t_i + 1] + 1) %                             \
					nda_i_p->descriptor->grouping[nda_t_i + 1]))                 \
				nda_l_v += nda_i_p->descriptor->separation[nda_t_i];             \
			else                                                                 \
				break;                                                           \
		}                                                                        \
		else                                                                     \
		{                                                                        \
			if(((nda_i_p->index[nda_t_i + 1] + 1) ==                             \
					nda_i_p->descriptor->dim_size[nda_t_i + 1]))                 \
				nda_l_v += nda_i_p->descriptor->separation[nda_t_i];             \
			else                                                                 \
				break;                                                           \
		}                                                                        \
	}                                                                            \
}

/* End of ndarray.h */

#include "freeform.h"


#endif
