/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2010-2016  The HDF Group.                                   *
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

  \mainpage

  \section Introduction
  
  The h4mapwriter is an application that maps an HDF4 file into an XML file. 
  The XML map file can be used to retrieve the values inside of the 
  corresponding HDF4 file using the generic file read calls using offset and
  length.
  

 */

/*!

  \file hmap.h
  \brief Have structures and routines for creating HDF4 mapping files.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)

  \date June 16, 2011
  \note 
  added config.h.


  \date March 17, 2011
  \note 
  removed GOTO_ERROR definition.


  \date March 11, 2011
  \note 
  added extern num_err.
  added extern ferr.

  \author Peter Cao <xcao@hdfgroup.org>
  \date  October 12, 2007

*/

#ifndef HMAP_H
#define HMAP_H


#ifndef DATAINFO_MASTER
#define DATAINFO_MASTER       /*!< Use special HDF4 MAP writer API. */
#endif



#include <stdio.h>
#include <ctype.h>              /* For Solaris. */
#include <hdf.h>
#include <mfhdf.h>
#include <local_nc.h>
#include <unistd.h>
#include "h4.h"
#include "xml.h"
#include "config.h"

/*! 
  \def FAIL_ERROR(message)
  \brief Return FAIL after printing \a message.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \note added.
  \date  March 10, 2011
*/
#define FAIL_ERROR(message) {\
          fprintf(flog, "%s:%d:%s\n", __FILE__,  __LINE__, message); \
          return FAIL;}

/*! 
  \def CHECK_ERROR(status, val, message)
  \brief Check the return status and write error message.

  \author Hyo-Kyung (Joe) Lee (hyoklee@hdfgroup.org)
  \date March 17, 2011
  \note added exit_errs++; line.

  \date March 15, 2011
  \note removed num_errs++; line.

  \date  March 10, 2011
  \note format changed to include file name.
*/
#define CHECK_ERROR(status, val, message) {if(status == val) { \
            fprintf(flog, "%s:%d:%s\n", __FILE__,  __LINE__, message); \
            ++exit_errs;                                               \
             goto done;}}


extern FILE* flog;
extern FILE* ferr;
extern int num_errs;
extern int has_extf;
extern int uuid;
#endif /* HMAP_H */


