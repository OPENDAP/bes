/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2011  The HDF Group                                         *
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
  \file h4_pal.c
  \brief Read information about palettes.

  \author Ruth Aydt
  \date July 17, 2012
  \note Rewritten to handle IP8 and LUT DDs with same ref and different offsets for DEs.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \author Ruth Aydt (aydt@hdfgroup.org)
  \date February 16, 2011
  \note Created.

*/
#include "hmap.h"


/*
 * These will be allocated and populated here.  They are kept around so the information
 * can be written to the map, and so that Rasters elements can reference their Palette
 * elements in the map file.
 */
PAL_DD_info_t   *pal_DDs = NULL;        /*!< shared with xml_pal.c */
PAL_DE_info_t   *pal_DEs = NULL;        /*!< shared with xml_pal.c*/

/*!

  \fn read_pals(char *h4filename, int32 file_id, int32 gr_id )
  \brief  Read palette info from file and assign element ids.  pal_DDs and pal_DEs alloced and filled.

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note Replaced read_pal; This handles IP8 and LUT DDs with same ref and different offsets. 

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date March 11, 2011
  \note Replaced stderr with flog. Cleaned up error messages.

  \return number of palette DEs in file
  \return FAIL

 */

int 
read_pals( char *h4filename, int32 file_id, int32 gr_id )
{
    hdf_ddinfo_t *ddInfo = NULL;       /* Array of [tag, ref, offset, length] values for each Palette DD */
    int nPalDDs, nPalDEs;              /* Number of palette DDs and DEs in the file */
    int d, e;                          /* Loop variables for palette descriptors & palette elements */
    int status;                        /* Return value from some hdf4 calls */

    /* Get the number of Palette DEs in the file */
    nPalDEs = DFPnpals( h4filename );
    if ( nPalDEs == FAIL ) {
        fprintf(flog, "ERROR: DFPnpals() failed.\n");
        return FAIL;
    } else if ( nPalDEs == 0 ) {        /* No palette data elements in the file */
        return( 0 );                    
    };

    /* Get the number of IP8 and LUT DDs in the file */
    nPalDDs = GRgetpalinfo( gr_id, 0, NULL );
    if ( nPalDDs == FAIL ) {
        fprintf(flog, "ERROR: GRgetpalinfo() failed.\n");
        return FAIL;
    } else if ( nPalDDs == 0 ) {        /* Unexpected since we had palette Data Elements */
        fprintf(flog, "ERROR: File has palette Data Descriptors but no palette Data Elements.\n");
        return FAIL;
    };

    /* Space for "generic" info from palette DDs */
    ddInfo = calloc( (size_t) nPalDDs, sizeof(hdf_ddinfo_t) );
    if ( ddInfo == NULL ) {
        fprintf(flog, "ERROR: Unable to allocate space for ddInfo in read_pals().\n"); 
        return FAIL; 
    }

    /* Space for information from Palette DDs, and for Map IDs.                    *
     * One extra entry that is used to mark end of list.                           *
     * Since calloc is used, we know the fields will all be initialized to 0.      */
    pal_DDs = calloc( (size_t) nPalDDs+1, sizeof( PAL_DD_info_t ) );
    if ( pal_DDs == NULL ) {
        fprintf(flog, "ERROR: Unable to allocate space for pal_DDs in read_pals().\n"); 
        free( ddInfo );
        return FAIL; 
    }

    /* Space for offsets and values of palette DEs */
    pal_DEs = calloc( (size_t) nPalDEs, sizeof( PAL_DE_info_t ) );
    if ( pal_DEs == NULL ) {
        fprintf(flog, "ERROR: Unable to allocate space for pal_DEs in read_pals().\n"); 
        free( pal_DDs );
        pal_DDs = NULL;
        free( ddInfo );
        return FAIL; 
    }

    /* Get generic information for Palette DDs from file */
    status = GRgetpalinfo( gr_id, nPalDDs, ddInfo );
    if ( (status == FAIL) || (status != nPalDDs) ) {
        fprintf(flog, "ERROR: GRgetpalinfo() failed or got different number of DDs than expected.\n");
        free_pal_arrays();
        free( ddInfo );
        return FAIL;
    } 

    /* 
     * For each Palette DD:
     *  Check for expected palette length.
     *  Move needed information from the generic ddInfo list to the permanent pal_DDs array.
     *  Update pal_DEs array when new offsets are seen.
     *  Assign ID to pal_DDs entry based on position of palette offset/values in pal_DEs array.
     */

    for ( d = 0; d < nPalDDs; d++ ) {

        /* Make sure we have expected length */
        if ( ddInfo[d].length != N_PAL_VALUES ) {
            fprintf( flog, "ERROR: Palette DD at index %d had DE of length %d.\n", d, (int)ddInfo[d].length );
            free_pal_arrays();
            free( ddInfo );
            return FAIL;
        }

        /* Move select generic DD information to palette DD info array */
        pal_DDs[d].tag = ddInfo[d].tag;
        pal_DDs[d].ref = ddInfo[d].ref;

        /* Add offset and palette values to DE info array if not already there, using index in array to set element ID */
        for ( e = 0; e < nPalDEs; e++  ) {
            if ( pal_DEs[e].offset == ddInfo[d].offset ) {    /* Palette for this offset already mapped */
                break;
            } else if ( pal_DEs[e].offset == 0 ) {            /* First time for this offset; add it and read values */
                pal_DEs[e].offset = ddInfo[d].offset;

                /* Get the palette values from the DE */
                status = Hgetelement( file_id, pal_DDs[d].tag, pal_DDs[d].ref, pal_DEs[e].data );
                if ( status != N_PAL_VALUES ) {
                    fprintf( flog, "Unexpected: Hgetelement for palette DD at index %d returned %d bytes.\n", d, status );
                    free_pal_arrays();
                    free( ddInfo );
                    return FAIL;
                }
                break;
            }
        }
        pal_DDs[d].elementID = e+1;                           /* Element IDs start at 1, not 0, so add 1 to index */
    }

    /* Free ddInfo as we don't need it anymore */
    free( ddInfo );

    /* Return the number of palette Data Elements in the file */
    return nPalDEs;
}

/*!

  \fn void free_pal_arrays( void )
  \brief Free the long-term memory that was allocated in read_pals

  \author Ruth Aydt (aydt@hdfgroup.org)
  \date July 17, 2012
  \note Created.

*/
void
free_pal_arrays( void )
{
    free( pal_DDs );
    pal_DDs = NULL;
    free( pal_DEs );
    pal_DEs = NULL;
}
