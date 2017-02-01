/*
 * FILENAME: avltree.h
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

#ifndef AVLTREE_H__
#define AVLTREE_H__

typedef int	*TREE;			/* Dummy typedef for a tree. */

typedef struct	_leaf
{
	struct  _leaf	*left     ;
	struct  _leaf	*right	  ;
	unsigned	size : 14 ;
	unsigned	bal  :  2 ;
}
HEADER;
			/* Possible values of bal field. Can be	*/
			/* any three consecutive numbers but	*/
			/* L < B < R must hold.			*/
#define L	 0	/* 	Left  subtree is larger		*/
#define	B	 1	/* 	Balanced subtree		*/
#define R	 2	/* 	Right subtree is larger		*/

HEADER *insert ( HEADER**, HEADER*, int(*)(void *, void *) );

void	tprint  ( TREE* , void(*)(), FILE* );

HEADER *talloc ( int );
void	freeall ( HEADER** );

#endif /* AVLTREE_H__ */
