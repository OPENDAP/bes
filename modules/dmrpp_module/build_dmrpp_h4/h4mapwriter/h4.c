/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2013  The HDF Group.                                   *
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
 \file h4.c
 \brief Help reading HDF4 files.

 This file has utility functions that are commonly used in reading different 
 HDF4 objects.

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
 \date October 11, 2013
 \note made error message consistent in set_mattr().

 \date  October 9, 2013
 \note updated check_tag_expected() and read_all_refs().


 \author Ruth Aydt (aydt@hdfgroup.org)
 \date  July 17, 2012
 \note  updated to support 2 palette visited lists.

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
 \date  October 4, 2010
 \note created.
*/

#include "hmap.h"

mattr_list_ptr list_mattr = NULL; /*!< Merged attributes list.  */

/*!
  \fn check_tag_expected(uint16 tag, 
                   int DIL_annHandled_seen, 
                   int DIL_annOther_seen,
                   int DIA_annHandled_seen,
                   int DIA_annOther_seen,
                   int ID_unexpected_cnt)
  \brief Check if \a tag is expected in the file.

  \return 0 if \a tag isn't expected. 
  \return 1 otherwise.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 9, 2013
  \note reduced the number of function arguments by mering tag 201 and 700.
  moved 200-306 from "planned" to "supported".

  \date May 5, 2011
  \note removed checking Rasters under Vgroup.

  \date March 16, 2011
  \note errors are redirected so that they can be added into the final map.

  \date October 4, 2010
  \note added.
*/

int
check_tag_expected(uint16 tag, 
                   int DIL_annHandled_seen, 
                   int DIL_annOther_seen,
                   int DIA_annHandled_seen, 
                   int DIA_annOther_seen,
                   int ID_unexpected_cnt)

{
    int result = 1;


    switch( tag ) {

    case 20:        /* LINKED */
    case 40:        /* COMPRESSED */
    case 61:        /* CHUNK */
    case 100:       /* FID - file identifier */
    case 101:       /* FD - file descriptor */
    case 106:       /* NT - number type */
    case 200:       /* ID8 - image dimension-8 (obsolete)  */
    case 201:       /* IP8 - image palette-8 (obsolete) */
    case 202:       /* RI8 - raster image-8 (obsolete) */
    case 203:       /* CI8 - compressed image 8 (obsolete) */
    case 301:       /* LUT - lookup table (palette) */
    case 302:       /* RI - raster image */
    case 306:       /* RIG - raster image group */
    case 701:       /* SDD - Scientific data dimension */
    case 702:       /* SD - Scientific Data */
    case 703:       /* SDS - scale */
    case 704:       /* SDL - label */
    case 705:       /* SDU - units*/
    case 706:       /* SDF - format */
    case 707:       /* SDM - min-max */
    case 720:       /* NDG - numeric data group */
    case 731:       /* CAL - calibration*/
    case 1962:      /* VH - vdata description */
    case 1963:      /* VS - vdata */

    case 16424:     /* Special COMPRESSED (40) */
    case 16445:     /* Special CHUNK (61) */
    case 17086:     /* Special SD (702) */
    case 18347:     /* Special VS (1963) */
        break;

    case 11:        /* RLE - run length encoding */
    case 12:        /* IMCOMP - compressed data */
    case 13:        /* JPEG - 24-bit jpeg compression info */
    case 14:        /* GRAYJPEG - 8-bit jpeg compression info */
    case 15:        /* JPEG5 */
    case 16:        /* GREYJPEG5 */
    case 50:        /* VLINKED */
    case 51:        /* VLINKED_DATA */
    case 60:        /* CHUNKED */
    case 102:       /* TID - tag identifier */
    case 103:       /* TD - tag description */
    case 107:       /* MT - machine type */

    case 204:       /* II8 - IMCOMP image 8 (obsolete) */

    case 304:       /* NRI */
    case 307:       /* LD - LUT dimension*/
    case 308:       /* MD - Matte channel dimension */
    case 309:       /* MA - Matte channel */
    case 310:       /* CCN - color correction */
    case 311:       /* CFM - color format */
    case 312:       /* AR - aspect ratio */
    case 400:       /* DRAW */
    case 401:       /* RUN */
    case 500:       /* XYP - x-y position */
    case 501:       /* MTO */
    case 602:       /* T14 - tektronix 4014 */
    case 603:       /* T105 - tekgronix 4105 */
    case 709:       /* SDT - transpose */
    case 781:       /* SDRAG */
        fprintf(ferr, "UNMAPPABLE:" );
        result = 0;
        break;

    case 303:       /* CI - compressed raster image */
        fprintf(ferr, "PARTIAL+:");  /* Map 8 bit RLE only but not others */
        /* Make sure to investigate if seen; other DDs should catch but just 
           in case.  */
        result = 0;    
        break;

    case 30:        /* VERSION - WILL NOT MAP. */
        break;

    case 104:       /* DIL - data identifier label */
        if (DIL_annOther_seen) {
            fprintf(ferr, "UNMAPPABLE:" );
            result = 0;
        }
        break;

    case 105:       /* DIA - data identifier annotation */
        if (DIA_annOther_seen) {
            fprintf(ferr, "UNMAPPABLE:" );
            result = 0;
        }
        break;

    case 300:       /* ID - image dimension */
        if (ID_unexpected_cnt > 0 ) {
            fprintf(ferr, "UNMAPPABLE:" );
            result = 0;
        }
        break;

    case 708:  /* SDC - coordinates  << probably handled but not yet seen >> */
    case 732:  /* FV - fill value << probably handled but not yet seen >> */
        fprintf(ferr, "UNCONFIRMED+:" );
        result = 0;
        break;

    case 710:       /* SDLNK */
        fprintf(ferr, "COMPATIBILITY:" );
        result = 0;
        break;

    case 700:       /* SDG - scientific data group (obsolete) */
        fprintf(ferr, "OBSOLETE:" );
        result = 0;
        break;

    default:
        fprintf(ferr, "UNMAPPABLE:" );
        result = 0;
    }

    if(result == 0){
        fprintf(ferr, "Object with the following tag can't be mapped.");
        fprintf(ferr, ":tag=%d\n", tag);
    }

    return result;
}

/*!
  \fn check_nt_unexpected(int32 file_id, uint16 ref, int32 offset, 
      int32 length)
  \brief Check if \a file_id has a number type that's not expected in the file.

  \return 1 if a NT that isn't expected. 

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date May 13, 2011
  \note 
  added newline character at the end of reference number printing.
  replaced "class:=" with "class=".

  \date March 17, 2011
  \note replaced NUMBER_TYPE with DATATYPE because type can be character.

  \date March 16, 2011
  \note errors are redirected so that they can be added into the final map.
*/
int
check_nt_unexpected(int32 file_id, uint16 ref, int32 offset, int32 length)
{
    int   unexpected = 0;
    uint8 ntstring[4];
    uint8 version = 0;
    uint8 type = 0;
    uint8 width = 0;
    uint8 class = 0; 

    
    Hgetelement(file_id, DFTAG_NT, ref, ntstring );
    version = ntstring[0];
    type = ntstring[1];
    width = ntstring[2];
    class = ntstring[3];

    if ( version != 1 ) {
        fprintf(ferr, "DATATYPE:unexpected version:version=%u", version );
        unexpected = 1;
    }

    if ( (type == 3) || (type == 4) ) {  /* UCHAR8 or CHAR8 */
        if ( width != 8 ) {
            fprintf(ferr, "DATATYPE:unexpected width for (U)CHAR8");
            fprintf(ferr, ":type=%u, width=%u", type, width );
            unexpected = 1;
        }
        if ( (class != 0)  && (class != 1) ) {
            fprintf(ferr, "DATATYPE:unexpected class for (U)CHAR8");
            fprintf(ferr, "-  neither BYTE nor ASCII");
            fprintf(ferr, ":type=%u, class=%u", type, class);
            unexpected = 1;
        }
    } else if ( (type == 20) || (type == 21) ) {   /* INT8 or UINT8 */
        if ( width != 8 ) {
            fprintf(ferr, "DATATYPE:unexpected width for (U)INT8");
            fprintf(ferr, ":type=%u, width=%u", type, width);
            unexpected = 1;
        }
        if ( (class != 0) && (class !=1) ) { 
            /* allow class==0 since only 1 byte */
            if ( class == 4 ) {
                fprintf(ferr, 
                        "DATATYPE:little-endian for (U)INT8");
                fprintf(ferr,":type=%u, class=%u", type, class);
            } else {
                fprintf(ferr,"DATATYPE:unexpected class for (U)INT8");
                fprintf(ferr, " -  not MBO/BE or IBO/LE");
                fprintf(ferr, ":type=%u, class=%u", type, class);
            }
            unexpected = 1;
        }
    } else if ( (type == 22) || (type == 23) ) {   /* INT16 or UINT16 */
        if ( width != 16 ) {
            fprintf(ferr, "DATATYPE:unexpected width for (U)INT16");
            fprintf(ferr, ":type=%u, width=%u", type, width );
            unexpected = 1;
        }
        if ( class != 1 ) {
            if ( class == 4 ) {
                fprintf(ferr, 
                        "DATATYPE:little-endian for (U)INT16");
                fprintf(ferr,":type=%u, class=%u", type, class);

            } else {
                fprintf(ferr,"DATATYPE:unexpected class for (U)INT16");
                fprintf(ferr, " -  not MBO/BE or IBO/LE");
                fprintf(ferr, ":type=%u, class=%u", type, class);
            }
            unexpected = 1;
        }
    } else if ( (type == 24) || (type == 25) ) {   /* INT32 or UINT32 */
        if ( width != 32 ) {
            fprintf(ferr, "DATATYPE:unexpected width for (U)INT32");
            fprintf(ferr, ":type=%u, width=%u", type, width );
            unexpected = 1;
        }
        if ( class != 1 ) {
            if ( class == 4 ) {
                fprintf(ferr, 
                        "DATATYPE:little-endian for (U)INT32");
                fprintf(ferr,":type=%u, class=%u", type, class);

            } else {
                fprintf(ferr,"DATATYPE:unexpected class for (U)INT32");
                fprintf(ferr, " -  not MBO/BE or IBO/LE");
                fprintf(ferr, ":type=%u, class=%u", type, class);
            }
            unexpected = 1;
        }
    } else if (type == 5) {   /* FLOAT32 */
        if ( width != 32 ) {
            fprintf(ferr, "DATATYPE:unexpected width for FLOAT32");
            fprintf(ferr, ":type=%u, width=%u", type, width );
            unexpected = 1;
        }
        if ( class != 1 ) {
            if ( class == 4 ) {
                fprintf(ferr, 
                        "DATATYPE:little-endian IEEE for FLOAT32");
                fprintf(ferr,":type=%u, class=%u", type, class);

            } else {
                fprintf(ferr,"DATATYPE:unexpected class for FLOAT32");
                fprintf(ferr, " -  not IEEE");
                fprintf(ferr, ":type=%u, class=%u", type, class);
            }
            unexpected = 1;
        }
    } else if (type == 6) {   /* FLOAT64 */
        if ( width != 64 ) {
            fprintf(ferr, "DATATYPE:unexpected width for FLOAT64");
            fprintf(ferr, ":type=%u, width=%u", type, width );
            unexpected = 1;
        }
        if ( class != 1 ) {
            if ( class == 4 ) {
                fprintf(ferr, 
                        "DATATYPE:little-endian IEEE for FLOAT64");
                fprintf(ferr,":type=%u, class=%u", type, class);

            } else {
                fprintf(ferr,"DATATYPE:unexpected class for FLOAT32");
                fprintf(ferr, " -  not IEEE");
                fprintf(ferr, ":type=%u, class=%u", type, class);
            }
            unexpected = 1;
        }
    } else {
        fprintf(ferr, "DATATYPE:unexpected type:type=%u", type );
        unexpected = 1;
    }
    if(unexpected == 1){
        fprintf(ferr, ", ref=%d\n", ref);
    }
    return unexpected;
        
}


/*!
 
 \fn getTwoByteValue(int swapBytes,  uint8* from, void* to)

 \brief Copy two bytes from memory at 'from' into memory at 'to', after 
 swapping them if needed

 */
void
getTwoByteValue(int swapBytes,  uint8* from, void* to )
{
    if (swapBytes) {
        swap_2bytes( from );               
    }

    memcpy( to, from, 2 );
    
    return;
}

/*! 
 \fn getFourByteValue(int swapBytes,  uint8* from, void* to )
 \brief Copy four bytes from memory at 'from' into memory at 'to', after 
 swapping them if needed.
 */
void
getFourByteValue(int swapBytes,  uint8* from, void* to )
{
    if (swapBytes) {
        swap_4bytes( from );               
    }

    memcpy( to, from, 4 );
    
    return;
}


/*!
  \fn is_little_endian()
  \brief  Checks if the machine architecture is little-endian.

  \return 1 if it is little-endian.
  \return 0 otherwise.
*/
int 
is_little_endian()
{
    unsigned short  ushortv;
    unsigned char*  ucharvp;
    unsigned char   ucharv;
    int             isLE;

    ushortv = 1;            /* put 1 in LSB */

    ucharvp = (unsigned char*) &ushortv;
    ucharv = *ucharvp;

    if ( (short)ucharv == ushortv ) {
        isLE = 1;
    } else {
        isLE = 0;
    }
    return isLE;

}

/*!
  \fn is_string(int32 type)
  \brief  Checks if \a type is character.

  \return 1 if type is (unsigned) character.
  \return 0 otherwise.

*/
int
is_string(int32 type)
{
    
    /* The HDF4 attribute must be character type. */
    if((type == DFNT_CHAR) || (type == DFNT_UCHAR)){
        return 1;
    }
    else{
        return 0;
    }
}

/*!
  \fn has_mattr(char* name)
  \brief  Checks if \a name is in the merged attribute list.

  \return 1 if exists.
  \return 0 otherwise.

 \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
 \date  October 5, 2010


*/
mattr_list_ptr 
has_mattr(char* name)
{

    mattr_list_ptr p = list_mattr;

    while(p != NULL){
        if((strlen(name) == strlen(p->m.name)) && 
           !strncmp(name, p->m.name, strlen(name)))
            {
                return p;
            }
        p = p->next;
    }
    return p;
}

/*!

  \fn free_mattr_list()
  \brief  Free the global list_mattr.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date  October 5, 2010

 */
void 
free_mattr_list()
{
    int i = 0;
    mattr_list_ptr p = list_mattr;

    while(p != NULL){
        mattr_list_ptr temp = p->next;
        if(p->m.name != NULL){
            HDfree(p->m.name);
        }
        if(p->m.names != NULL){
            for(i=0; i < p->m.count; i++){
                if(p->m.names[i] != NULL)
                    HDfree(p->m.names[i]);
            }
            HDfree(p->m.names);
        }
        if(p->m.length != NULL){
            HDfree(p->m.length);
        }
        if(p->m.offset != NULL){
            HDfree(p->m.offset);
        }
        if(p->m.str != NULL){
            HDfree(p->m.str);
        }
        HDfree(p);
        p = temp;
    } /* while(p != NULL) */
}

/*!
  \fn is_ignored(int tag, int* list, int count)

  \brief Check if \a tag is in the \a list whose size is \a count.

  \return 1 if exists
  \return 0 otherwise

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date  March 3, 2011

*/
int 
is_ignored(int tag, int* list, int count)
{
    int i = 0;
    for (i = 0; i < count; i++) {
        if (list[i] == tag)
            return 1;
    }
    return 0;
}


/*!
  \fn is_ref_visited(ref_count_t* count, int32 ref)

  \brief Check if \a ref is in the \a count list.

  \return 1 if exists
  \return 0 otherwise

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date  October 28, 2010

*/
int 
is_ref_visited(ref_count_t* count, int32 ref)
{
    int i = 0;
    if(count == NULL){
        return 0;
    }
    for (i=0; i < count->size; i++) {
        if (count->refs[i] == ref)
            return 1;
    }
    return 0;
}

/*!
  \fn read_all_refs(int32 file_id, FILE* filep, ref_count_t *obj_missed, 
  ref_count_t *sd_visited, ref_count_t *vs_visited, ref_count_t *vg_visited, 
  ref_count_t *ris_visited, ref_count_t *palIP8_visited, 
  ref_count_t *palLUT_visited, int* ignore_tags, int ignore_cnt)

  \brief  Read all references and count the missing objects.

  \return number of missing objects.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Oct 9, 2013
  \note updated the code to match the current behavior of map writer.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note
  broke pal_visited into palIP8_visited and palLUT_visited

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date June 30, 2011
  \note 
  added extra check for external file.

  \date May 9, 2011
  \note 
  removed code for checking palette dimension DD related to GR image.

  \date March 16, 2011
  \note 
  errors are redirected so that they can be added into the final map.
  reset not_fully_mapped variable at the beginning of while() loop.

  \date  March 11, 2011
  \note replaced malloc() with HDmalloc() and calloc() with HDcalloc().

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date  Feb 10, 2011

*/
int 
read_all_refs(int32 file_id, FILE* filep, 
              ref_count_t *obj_missed, 
              ref_count_t *sd_visited, 
              ref_count_t *vs_visited, 
              ref_count_t *vg_visited, 
              ref_count_t *ris_visited,
              ref_count_t *palIP8_visited,
              ref_count_t *palLUT_visited,
              int* ignore_tags, int ignore_cnt)
{

    /* flag if any DIA annot dd with tag 201 | 700 | 1965 */
    int DIA_annHandled_seen = 0; 
    /* flag if any DIA annot dd with tag != 201 | 700 | 1965 */
    int DIA_annOther_seen = 0;
    /* flag if any DIL annot dd with tag 201 | 700 | 1965 */
    int DIL_annHandled_seen = 0;
    /* flag if any DIL annot dd with tag != 201 | 700 | 1965 */
    int DIL_annOther_seen = 0;
    int ID_unexpected_cnt = 0;/* Count of ID objects with nComponents != 1 */
    int buf_offset = 0;       /* offset into buffer for value of interest */
    int not_fully_mapped = 0; /* flag if not yet fully mapped */
    int swapBytes = 0;        /* if on LE system, we need to swap */

    int16   nComponents = 0; /* number of components in Image (expect 1) */
    int16   var_int16 = 0;
    int32   find_offset = 0;  /* offset of current dd data element */
    int32   find_length = 0;  /* length of current dd data element */
    int32   var_int32 = 0;

    intn    status = SUCCEED; /* status from h4 calls */

    uint8*  buffer = NULL;    /* buffer to read data element */

    uint16  annotated_tag;      /* tag of dd that was annotated */
    uint16  annotated_ref;      /* ref of dd that was annotated */
    uint16  find_tag = 0;       /* tag for current dd */
    uint16  find_ref = 0;       /* ref for current dd */
    uint16  special_type;       /* special tag type */
    uint16  var_uint16;

    int32   external_length;    /* length of data in external file */
    int32   external_offset;    /* offset of data in external file */
    int32   external_filename_length;   /* length of external filename */

    /* Determine if we'll need to swap bytes. */
    swapBytes = is_little_endian();

    do {
        status = Hfind(file_id, DFTAG_WILDCARD, DFREF_WILDCARD, 
                       &find_tag, &find_ref, &find_offset, &find_length, 
                       DF_FORWARD );
        if ( status == SUCCEED ) {
            not_fully_mapped = 0;

            /* Check if number type is supported. */
            if(find_tag == DFTAG_NT){
                if (check_nt_unexpected(file_id, find_ref, find_offset, 
                                         find_length) == 1) {
                    not_fully_mapped = 1;
                }
            }
            /* For Data Annotations - what is annotated?; 
               Not all are handled.          */ 
            /* Variable not_fully_mapped is set in summary code 
               based on flags  set here */
            if ( (find_tag == DFTAG_DIL) || (find_tag == DFTAG_DIA) ) {
                buffer = HDcalloc(find_length+1, 1 );   
                if(buffer == NULL){
                    FAIL_ERROR("HDcalloc() failed: Out of Memory");
                }
                read_bytes(filep, find_offset, find_length, buffer);
                if (swapBytes) {
                    swap_2bytes(buffer);
                    swap_2bytes(buffer+2);
                }

                memcpy(&annotated_tag, buffer, 2 );
                memcpy(&annotated_ref, buffer+2, 2 );

                if ( find_tag == DFTAG_DIL ) {
                    if ( ( annotated_tag == 201 ) ||        /* mapped */
                         ( annotated_tag == 700 ) || 
                         ( annotated_tag == 1965 ) ) { 
                        DIL_annHandled_seen = 1;
                    }
                    else{
                        DIL_annOther_seen = 1;       /* not mapped */
                        fprintf(ferr, "ANNOTATION:" );
                        fprintf(ferr, "Object has Data Label.");
                        fprintf(ferr, ":tag=%u, ref=%u, value=%s\n",
                                annotated_tag, annotated_ref, buffer+4 );
                        not_fully_mapped = 1;
                    } 

                } /* if(find_tag == DFTAG_DIL) */

                if ( find_tag == DFTAG_DIA ) {
                    if ( ( annotated_tag == 201 ) ||        /* mapped */
                         ( annotated_tag == 700 ) || 
                         ( annotated_tag == 1965 ) ) { 
                        DIA_annHandled_seen = 1;
                    } else {                                /* not mapped */
                        DIA_annOther_seen = 1;       /* not mapped */
                        fprintf(ferr, "ANNOTATION:" );
                        fprintf(ferr, "Object has Data Description.");
                        fprintf(ferr, ":tag=%u, ref=%u, value=%s\n",
                                annotated_tag, annotated_ref, buffer+4 );
                        not_fully_mapped = 1;

                    }
                } /* if(find_tag == DFTAG_DIA) */
                HDfree(buffer);
            } /* Check if tag is already supported. */

            /* We map only 8-bit Raster Images (with RLE compression). */

            /* We don't map IMCOMP compressed image. */
            if ( find_tag == DFTAG_II8 ) {
                fprintf(ferr, "RASTER:" );
                fprintf(ferr, "Unmapped HDF4 object with tag %u ref %u; IMCOMP image compression.\n",
                        find_tag, find_ref );
                not_fully_mapped = 1;
            }

            /* We don't map JPEG compressed image. */
            if ( find_tag == DFTAG_JPEG ) {
                fprintf(ferr, "RASTER:" );
                fprintf(ferr, "Unmapped HDF4 object with tag %u ref %u; JPEG image compression.\n",
                            find_tag, find_ref );
                not_fully_mapped = 1;
            }

            /* We don't map GREYJPEG compressed image. */
            if ( find_tag == DFTAG_GREYJPEG ) {
                fprintf(ferr, "RASTER:" );
                fprintf(ferr, "Unmapped HDF4 object with tag %u ref %u; GREYJPEG image compression.\n",
                            find_tag, find_ref );
                not_fully_mapped = 1;
            }


            /* We don't map JPEG5 compression image. */
            if ( find_tag == DFTAG_JPEG5 ) {
                fprintf(ferr, "RASTER:" );
                fprintf(ferr, "JPEG5 image compression for 24 bit data.");
                fprintf(ferr, ":tag=%u, ref=%u\n", find_tag, find_ref );
                not_fully_mapped = 1;
            }

            /* We don't map images with number of components != 1. */
            if ( find_tag == DFTAG_ID ) {
                buffer = HDcalloc( find_length+1, 1 );                   
                if(buffer == NULL){
                    FAIL_ERROR("HDcalloc() failed: Out of Memory");
                }
                read_bytes(filep, find_offset, find_length, buffer);

                buf_offset = 0; /* xdim */
                getFourByteValue(swapBytes,buffer+buf_offset, (void*) &var_int32 );
                buf_offset += 4; /* ydim */
                getFourByteValue(swapBytes, buffer+buf_offset, (void*) &var_int32 );
                buf_offset +=4; /* NT tag */
                getTwoByteValue(swapBytes, buffer+buf_offset, (void*) &var_uint16 );
                buf_offset +=2; /* NT ref */
                getTwoByteValue(swapBytes, buffer+buf_offset, (void*) &var_uint16 );
                buf_offset +=2; /* nComponents */
                getTwoByteValue(swapBytes, buffer+buf_offset, (void*) &nComponents );
                buf_offset +=2; /* interlace */
                getTwoByteValue(swapBytes, buffer+buf_offset, (void*) &var_int16 );
                buf_offset +=2; /* compression tag */
                getTwoByteValue(swapBytes, buffer+buf_offset, (void*) &var_uint16 );
                buf_offset +=2; /* compression ref */
                getTwoByteValue(swapBytes, buffer+buf_offset, (void*) &var_uint16 );

                if ( nComponents != 1 ) {
                    fprintf(ferr, "RASTER:" );
                    fprintf(ferr, "Not yet supporting > 1 component" );
                    fprintf(ferr, ":tag=%u, ref=%u, component=%d\n",
                            find_tag, find_ref, nComponents );

                    not_fully_mapped = 1;
                    ID_unexpected_cnt++;
                }
                HDfree(buffer);
            }


            /* For Special Storage - Report if data is in External File */
            if (SPECIALTAG(find_tag)) {
                buffer = HDcalloc( find_length+1, 1 );   
                if(buffer == NULL){
                    FAIL_ERROR("HDcalloc() failed: Out of Memory");
                }

                read_bytes(filep, find_offset, find_length, buffer);
                if (swapBytes) {
                    swap_2bytes(buffer);       /* What special type? */
                }
                memcpy(&special_type, buffer, 2);

                if (special_type == SPECIAL_EXT) {

                    if (swapBytes) {
                        swap_4bytes(buffer+2); /* length in external file */
                        swap_4bytes(buffer+6); /* offset in external file */
                        swap_4bytes(buffer+10);/* length of ext. filename */
                    }
                    memcpy(&external_length, buffer+2, 4);
                    memcpy(&external_offset, buffer+6, 4);
                    memcpy(&external_filename_length, buffer+10, 4);

                    fprintf(ferr, "EXT_FILE:") ;
                    fprintf(ferr, "Not yet supporting external files.");
                    fprintf(ferr, ":filename=%s, offset=%ld, length=%ld\n",
                            buffer+14, 
                            (long) external_offset, 
                            (long) external_length );
                    not_fully_mapped = 1;
                    has_extf = 1;
                }
                HDfree(buffer);
            }

            /* Search if the ref is visited. */
            /* Note: Since tag/ref combination isn't checked, it is
             * possible to get a match on a ref from another tag.
             * For example, match a raster ref for a palette IP8 DD.
             * So, not a perfect check. */
            if(!(
                 (find_ref == 1) || /* tag 30 - version number */
                 is_ref_visited(sd_visited, find_ref) || 
                 is_ref_visited(vs_visited, find_ref) ||
                 is_ref_visited(vg_visited, find_ref) ||
                 is_ref_visited(ris_visited, find_ref) ||
                 is_ref_visited(palIP8_visited, find_ref) ||
                 is_ref_visited(palLUT_visited, find_ref) ||
                 is_ignored(find_tag, ignore_tags, ignore_cnt) ||
                 check_tag_expected(find_tag, 
                                    DIL_annHandled_seen, 
                                    DIL_annOther_seen, 
                                    DIA_annHandled_seen, 
                                    DIA_annOther_seen,
                                    ID_unexpected_cnt)
                 )
               )
                {
                    ++num_errs;
                    ref_count(obj_missed, find_ref);
                }
            else if(not_fully_mapped == 1){
                    ++num_errs;
                    ref_count(obj_missed, find_ref);
            }


        } /* if(status == SUCCEED) */

        
    } while( status == SUCCEED );

    if(obj_missed != NULL)
        return obj_missed->size;
    else
        return 0;
}

/*!
  \fn read_bytes(FILE* filep, int offset, int length, uint8* buffer)

  \brief  Read all references and count the missing objects.

  \return number of missing objects.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \author Ruth Aydt (aydt@hdfgroup.org)
  \date  January 13, 2011

*/
void 
read_bytes(FILE* filep, int offset, int length, uint8* buffer)
{

    int retval = 0;
    /* Seek to desired offset. */
    retval = fseek(filep, offset, SEEK_SET);
    if (retval != 0) {
        fprintf(flog, "Error in seeking to offset %d\n", offset);
        exit(1);
    }

    /* Read the bytes */
    retval = fread(buffer, 1, length, filep);
    if (retval < length) {
        fprintf(flog, 
                "Error reading bytes from file. %d read and %d expected\n", 
                retval, length);
        exit(1);
    }

    /* Put a null in the next byte just in case bytes end in string that isn't 
       NULL terminated */
    buffer[length] = 0;
}


/*!
  \fn set_mattr(char* attr_name, int32 attr_type, VOIDP attr_buf,
          int32 attr_buf_size, int32 offset, int32 length, int32 nattr)
  \brief  Sets merged attribute list.

  \return SUCCESS if mattr structure is properly initialized.
  \return FAIL otherwise.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date October 11, 2013
  \note  made error message consistent.

  \date February 3, 2012
  \note	rephrased the warning message for meatdatas that are not in sequence.

  \date October 5, 2010
  \note created.
*/
unsigned int 
set_mattr(char* attr_name, int32 attr_type, VOIDP attr_buf,
          int32 attr_buf_size, int32 offset, int32 length, int32 nattr)
{
    char* buffer = NULL;
    char* head = NULL;
    char* suffix  = NULL;  

    int n = 0;
    int i = 0;
    int warn = 0;

    mattr_list_ptr mattr_ptr = NULL;
    mattr_list_ptr p = NULL;

    /* Tokenize attribute name first using "." as delimiter. */
    buffer = (char*) HDmalloc(sizeof(char) * strlen(attr_name)+1);
    if(buffer == NULL){
        FAIL_ERROR("HDmalloc() failed: Out of Memory");
    }
    else {
        strncpy(buffer, attr_name, strlen(attr_name));
        buffer[strlen(attr_name)] = '\0';
        head =  strtok(buffer, ".");
        suffix = strtok(NULL, "");
        if(suffix != NULL){
            n = atoi(suffix);
        }
    }

    /* Check if it's in the merged attribute list. */
     mattr_ptr = has_mattr(head);

    if(mattr_ptr == NULL){      /* Create a new mattr element. */
        /* Throw a warning if sequence doesn't start with 0. */
        if(n != 0 && warn == 1){
            fprintf(flog, 
                    "%s file attribute does not begin with .0 suffix.\n", 
                    attr_name);
        }



        p = (mattr_list_ptr) HDmalloc(sizeof(mattr_list_elmt));
        if(p == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        p->m.name = (char*) HDmalloc(sizeof(char) * strlen(head) + 1);
        if(p->m.name == NULL){
            HDfree(p);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        else {
            strncpy(p->m.name, head, strlen(head));
            p->m.name[strlen(head)] = '\0';
        }

        /* There will be at most fattr.0, fattr.1, ..., fattr.nattr 
           original names. */
        p->m.names = (char**) HDmalloc(sizeof(char*) * nattr);
        if(p->m.names == NULL){
            HDfree(p->m.name);
            HDfree(p);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        else{
            memset(p->m.names, 0, nattr);
            p->m.names[0] = (char*) HDmalloc(sizeof(char) * 
                                             strlen(attr_name)+1);
            strncpy(p->m.names[0], attr_name, strlen(attr_name));
            p->m.names[0][strlen(attr_name)] = '\0';
        }

        p->m.length = (int*)HDmalloc(sizeof(int) * nattr);
        if(p->m.length == NULL){
            HDfree(p->m.name);
            HDfree(p->m.names[0]);
            HDfree(p->m.names);
            HDfree(p);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        p->m.length[0] = length;

        p->m.offset = (int*)HDmalloc(sizeof(int) * nattr);
        if(p->m.offset == NULL){
            HDfree(p->m.name);
            HDfree(p->m.names[0]);
            HDfree(p->m.names);
            HDfree(p->m.length);
            HDfree(p);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        p->m.offset[0] = offset;

        p->m.str = HDmalloc(attr_buf_size);
        if(p->m.str == NULL){
            HDfree(p->m.name);
            HDfree(p->m.names[0]);
            HDfree(p->m.names);
            HDfree(p->m.length);
            HDfree(p->m.offset);
            HDfree(p);
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        memcpy(p->m.str, attr_buf, attr_buf_size);
        /* Initialize the rest of members properly. */
        p->m.count = 1;
        p->m.start = n; 
        p->m.trimmed = 0;
        p->m.type = attr_type;
        p->m.total = attr_buf_size;
        p->next = NULL;

        /* Append it to the list. */
        if(list_mattr == NULL){
            list_mattr = p;
        }
        else{
            mattr_list_ptr tail = list_mattr;
            while(tail->next != NULL){
                tail = tail->next;
            }
            tail->next = p;
        }
    }
    else {

        /* Throw a warning if sequence is out of order. */
        if((mattr_ptr->m.count + mattr_ptr->m.start) != n){
            fprintf(flog, 
                    "Expected %s.%d but found %s as next File Attribute in the sequence.\n", 
                    mattr_ptr->m.name, 
                    mattr_ptr->m.count + mattr_ptr->m.start,
                    attr_name);

        }

        /* Merge it with existing one. */
        i = mattr_ptr->m.count;

        /* Remember the original attribute name.  */
        mattr_ptr->m.names[i] = (char*) HDmalloc(sizeof(char) * 
                                                 strlen(attr_name)+1);
        if(mattr_ptr->m.names[i] == NULL){
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        strncpy(mattr_ptr->m.names[i], attr_name, strlen(attr_name));
        mattr_ptr->m.names[i][strlen(attr_name)] = '\0';

        /* Set the offset and length */
        mattr_ptr->m.offset[i] = offset;
        mattr_ptr->m.length[i] = length;

        /* Merge the attribute content. */
        mattr_ptr->m.str = (char*) 
            HDrealloc((VOIDP)mattr_ptr->m.str, 
                      mattr_ptr->m.total+attr_buf_size);
        if(mattr_ptr->m.str == NULL){
            HDfree(mattr_ptr->m.names[i]);
            FAIL_ERROR("HDrealloc() failed: Out of Memory");
        }

        memcpy(mattr_ptr->m.str+mattr_ptr->m.total, attr_buf, 
               attr_buf_size); 
        mattr_ptr->m.total += attr_buf_size;
        ++mattr_ptr->m.count;

    }
    if(buffer != NULL){
        HDfree(buffer);
    }
    return SUCCEED;
}

/*! 
  \fn swap_2bytes(uint8 *bytes) 
  \brief Swap the first 2 bytes in \a bytes array.
*/
void 
swap_2bytes(uint8 *bytes) 
{
    uint8 tmp = bytes[0];
    bytes[0] = bytes[1];
    bytes[1] = tmp;
}

/*! 
  \fn swap_4bytes(uint8 *bytes) 
  \brief Swap the 4 bytes in \a bytes array.
*/
void 
swap_4bytes( uint8 *bytes ) 
{
    uint8 tmp = bytes[0];
    bytes[0] = bytes[3];
    bytes[3] = tmp;
    tmp = bytes[1];
    bytes[1] = bytes[2];
    bytes[2] = tmp;
}
