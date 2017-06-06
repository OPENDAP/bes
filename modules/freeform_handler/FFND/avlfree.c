/*
 * FILENAME: avlfree.c
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
 */

#include <freeform.h>		/* AVLFREE.C */
#include <avltree.h>

static void fa(HEADER *root)
{
	/* Delete the entire tree pointed to by root. Note that unlike
	 * tfree(), this routine is passed a pointer to a HEADER rather
	 * than to the memory just below the header.
	 */

	if( root )
	{
		fa( root->left  );
		fa( root->right );
		memFree( root, "In fa avlfree.c" );
	 
	}
}

/*----------------------------------------------------------------------*/

void freeall(HEADER **root)
{
	fa( *root );
	*root = NULL;
}
