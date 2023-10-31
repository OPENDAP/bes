/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2011-2016  The HDF Group                                    *
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
  \file xml_pal.c
  \brief Functions to write palette information.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note major updates for palette handling. see ChangeLog.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \note created.
  \date February 11, 2011

*/

#include "hmap.h"

unsigned int ID_PA = 0;                 /*!< Unique Palette attribute ID.  */

extern PAL_DD_info_t   *pal_DDs;        /*!< Defined in h4_pal.c */
extern PAL_DE_info_t   *pal_DEs;        /*!< Defined in h4_pal.c */

/* Constant for all Palettes implemented in HDF4 */
#define N_PAL_ENTRIES     256   /*!< Number of Palette entries. */
#define N_PAL_COMPONENTS  3     /*!< Number of Palette components. */

/*!

  \fn write_map_pals( FILE* ofptr, char* h4filename, 
                      int32 file_id, int32 gr_id, int32 an_id,
                      ref_count_t *palIP8_visited, ref_count_t *palLUT_visited )
  \brief  Write palettes to map.

  \return SUCEED
  \return FAIL

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note 
  signifigant changes to support 2 palettes with same ref and different offsets

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date Jul 21, 2016
  \note added uuid function call.

  \date June 13, 2011
  \note 
  added increased indentation level to 2.

  \date March 14, 2011
  \note 
  added more error handling. 
  added unmappable objects handling.

  \date March 11, 2011
  \note 
  cleaned up error handling. 
  removed the use of num_errs.

  \date February 11, 2011
  \note  created.

 */
intn 
write_map_pals( FILE* ofptr, char* h4filename, 
                int32 file_id, int32 gr_id, int32 an_id,
                ref_count_t *palIP8_visited, ref_count_t *palLUT_visited )
{
    intn result = SUCCEED;

    int nPals;                          /* number of palettes */
    uint palID;                         /* ID of element currently being written */

    int i;                              /* indexing variables */
    PAL_DD_info_t *curDD;               /* shorthand */
    PAL_DE_info_t *curDE;               /* shorthand */

    int indent = 2;

    /* 
     * Read palette DD & DE info from file
     * Number of DEs is returned 
     */
    nPals = read_pals( h4filename, file_id, gr_id );
    if (nPals == FAIL){
        fprintf( flog, "read_pals() failed: filename=%s\n", h4filename);
        return FAIL;
    }

    /* 
     * Write one palette element to the map file for each DE in the file.
     * Note:
     *  - There may be multiple palette DDs that point to any given DE.
     *     - The annotations for all of these DDs get associated with the
     *       one palette element in the map.
     *  - Although the HDF4 documentation (and some code) indicate that there 
     *    are lots of options for Palettes, in fact only a basic set was ever
     *    implemented.  Hence, many of the attribute values are hard-coded rather
     *    than being read from the file.
     *  - It is possible for there to be an IP8 DD and an LUT DD with the same ref 
     *    that point to different DEs. 
     */

    for ( palID = 1; palID <= nPals; palID++ ) {

        /* Start the Palette map element */
        start_elm(ofptr, TAG_PAL, indent);

        /* Attributes other than palette ID are fixed at the basic values */
        fprintf( ofptr, 
            "nEntries=\"%d\" nComponentsPerEntry=\"%d\" id=\"ID_P%u\">",
            N_PAL_ENTRIES, 
            N_PAL_COMPONENTS,
            palID ); 

        /* Look through palette DDs for any that refer to this palette element. */
        i = 0;                              /* index into PAL_DD_info array */

        while ( pal_DDs[i].tag != 0 ) {     /* until we reach empty entry */

            if ( pal_DDs[i].elementID == palID ) {     /* found a DD for this element */

                curDD = &pal_DDs[i];                   /* shorthand to DD entry */

                /* Save the ref into the appropriate visited list. */
                if ( curDD->tag == 201 ) {
                    if (curDD->ref == ref_count(palIP8_visited, curDD->ref)) {
                        fprintf(flog, 
                            "ERROR:Palette has been already visited:tag=201,ref=%u\n",
                            curDD->ref);                
                        free_pal_arrays();
                        return FAIL;
                    }  /* else all is fine */
                } else if ( curDD->tag == 301 ) {
                    if (curDD->ref == ref_count(palLUT_visited, curDD->ref)) {
                        fprintf(flog, 
                            "ERROR:Palette has been already visited:tag=301,ref=%u\n",
                            curDD->ref);                
                        free_pal_arrays();
                        return FAIL;
                    }  /* else all is fine */
                } else {                              /* not 201 or 301 */
                    fprintf(flog, 
                        "ERROR:Unexpected tag for palette:tag=%u,ref=%u\n",
                        curDD->tag, curDD->ref );
                    free_pal_arrays();
                    return FAIL;
                }

                /* Write any annotations associated with this DD to the palette element */
                result = write_pal_attrs_an(ofptr, an_id, curDD->tag, curDD->ref, indent+1);
                if (result == FAIL) {
                    fprintf(flog, 
                        "ERROR:write_pal_attrs_an failed for palette:tag=%u,ref=%u\n",
                        curDD->tag, curDD->ref );
                    free_pal_arrays();
                    return FAIL;
                }
            }

            i++;                    /* move on to next DD */
        }

        /* Write datatype information - it must be "unit8" */
        indentation(ofptr, indent+1);        
        fprintf(ofptr, "<h4:datum dataType=\"uint8\"/>");

        /* Write paletteData element.*/
        start_elm_0(ofptr, TAG_PDATA, indent+1);

        /* Shortcut into pal_DEs array for this element */
        curDE = &pal_DEs[palID-1];

        /* Write offset/size information of data block. */
        start_elm(ofptr, TAG_BSTREAM, indent+2);
        if (uuid == 1) {
            write_uuid(ofptr, curDE->offset, N_PAL_VALUES);
        }
        fprintf(ofptr, "offset=\"%u\" nBytes=\"%d\"/>", 
                curDE->offset, N_PAL_VALUES);    

        end_elm(ofptr, TAG_PDATA, indent+1);

        /* Write values for verification */
        start_elm(ofptr, "!--", indent+1);
        fprintf(ofptr, "value(s) for verification; csv format");
        for (i=0; i < N_PAL_VALUES; i=i+3) {
            if (i == 0 || i == 33 || i== 387 || i == 765){
            indentation(ofptr, indent+2);
            fprintf(ofptr, "entry[%d]=\"%d,%d,%d\"", i/3, 
                        curDE->data[i],
                        curDE->data[i+1],
                        curDE->data[i+2]);
            }
        }
        indentation(ofptr, indent+1);
        fprintf(ofptr, "-->");
        end_elm(ofptr, TAG_PAL, indent);
    }

    return result;
}

/*!

  \fn palRef_to_palMapID( uint16 ref )
  \brief  Return Palette Element ID for h4 palette ref

  \return Palette Element Map ID
  \return 0 if no match (an error)

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 18, 2012
  \note created as part of extended palette handing

 */

uint
palRef_to_palMapID( uint16 ref )
{
    uint retID = 0;         /* typically IDs are uint but we won't roll over */
    uint curID;             /* shorthand */
    int i;

    i = 0;
    while ( pal_DDs[i].tag != 0 ) {                         /* until we reach empty entry */
        curID = pal_DDs[i].elementID;

        if ( pal_DDs[i].ref == ref ) {                      /* found a match */
            if ( retID == 0 ) {                             /* first match */
                retID = curID;
            } else {                                        /* not first match - look further */
                if ( retID == curID ) {                     /* both point to same palette map element */
                    /* all is well */
                } else {                                    /* if palette data are equal, doesn't matter which we reference */
                    if ( memcmp( pal_DEs[retID-1].data, pal_DEs[curID-1].data, (size_t)N_PAL_VALUES ) == 0 ) {
                        /*  all is well */
                    } else {                                /* palette data not equal; needs manual inspection to figure out */
                        fprintf(flog, "ERROR: multiple palettes with ref=%u and unequal palette data.\n", ref );
                        return( 0 );
                    }
                }
            }
        }
        i++;
    }

    return( retID );
}
