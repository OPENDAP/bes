/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2011  The HDF Group.                                   *
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

  \file xml_vg.h
  \brief Have routines for writing Vgroup information.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date March 11, 2011
  \note Documented in Doxygen.

 */

#ifndef _XML_VG_H_
#define _XML_VG_H_

#ifdef __cplusplus
extern "C" {
#endif
intn 
write_group(FILE *ofptr, char* name, const char* path, char* class, 
            intn indent);

intn 
write_group_attrs(FILE *ofptr, int32 id, int32 nattrs, intn indent);

intn 
write_vgroup_noroot(FILE *ofptr, int32 file_id, int32 sd_id, int32 gr_id,
                    int32 an_id, int indent, 
                    ref_count_t *sd_visited, 
                    ref_count_t *vs_visited, 
                    ref_count_t *vg_visited, 
                    ref_count_t *ris_visited);

#ifdef __cplusplus
}

#endif

#endif /* _XML_SDS_H_ */
