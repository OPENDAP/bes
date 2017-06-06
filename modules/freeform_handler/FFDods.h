
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ff_handler a FreeForm API handler for the OPeNDAP
// DAP2 data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.


// (c) COPYRIGHT URI/MIT 1997-98
// Please read the full copyright statement in the file COPYRIGHT.  
//
// Authors: reza (Reza Nekovei)

//
// DODS C++ header file for FreeFrom library. 
// 
// ReZa 6/16/97

#ifndef _ffdods_h
#define _ffdods_h 1


/*
 * FF_STD_ARGS
 */

typedef struct struct_ff_std_args FF_STD_ARGS, *FF_STD_ARGS_PTR, **FF_STD_ARGS_HANDLE;

struct struct_ff_std_args
{
#ifdef FF_CHK_ADDR
  void *check_address;
#endif
  char *input_file;           /* string: name of input file */
  char *input_buffer;         /* buffer containing input data */
  char *input_format_file;    /* string: name of input format file */
  char *input_format_title;   /* string: input format title */
  char *input_format_buffer;  /* string: input format description */
  char *output_file;          /* string: name of output file */
  char *output_buffer;        /* buffer to contain output data */
  char *output_format_file;   /* string: name of output format file */ 
  char *output_format_title;  /* string: output forma title */
  char *output_format_buffer; /* string: output format description */
  char *var_file;             /* string: name of variable file */
  char *query_file;           /* string: name of query file */
  unsigned long cache_size;	/* integer: default data cacheing size */
  long records_to_read;		/* integer: if negative, file tail */

  char *error_log;            /* string: file name to send error messages */
  BOOLEAN error_prompt;

  /* Checkvar specific option flags */
  int     cv_precision;
  double  cv_missing_data;
  int     cv_maxbins;
  BOOLEAN cv_maxmin_only;

  struct struct_std_args_user
  {
    unsigned int set_cv_precision : 1;
    unsigned int set_cv_missing_data : 1;
    unsigned int is_stdin_redirected : 1;
  } user;
};

/*
 * FORMAT
 */

typedef struct struct_ff_format FORMAT, *FORMAT_PTR, **FORMAT_HANDLE;
struct struct_ff_format
{
#ifdef FF_CHK_ADDR
  void *check_address;		/* assert(format->check_address == format); */
#endif
  VARIABLE_LIST  variables;	/* the DLL of variable structures */
  char *name;			/* title if given in a format file or menu
				   section */
  char *locus;			/* path-name or menu section title of format */
  FF_TYPES_t type;              /* format type, see FFF_LOOKUP format_types */
  unsigned int num_vars;	/* number of variables in format */
  FF_NDX_t length;		/* record length in bytes */
};

#define FORMAT_NAME_INIT "Format Was Untitled"
#define FORMAT_LOCUS_INIT "Unknown Origin"
#define FORMAT_LOCUS_GV_EDIT "GeoVu Format Editor"

/*
 * FF_BUFSIZE
 */

typedef struct struct_ff_bufsize FF_BUFSIZE, *FF_BUFSIZE_PTR, **FF_BUFSIZE_HANDLE;
struct struct_ff_bufsize
{
#ifdef FF_CHK_ADDR
        void *check_address;
#endif
        char HUGE *buffer;
        unsigned short usage;
        FF_BSS_t  bytes_used;
        FF_BSS_t  total_bytes;
};

#define DBASK_FORMAT_SUMMARY                3


/* Define BIN structures */
#define FREEFORM_H__
#include <eval_eqn.h>  


typedef struct struct_databin
{
#ifdef FF_CHK_ADDR
        void *check_address;
#endif
        char *title;

        EQUATION_INFO_PTR eqn_info;

        FF_ARRAY_CONDUIT_LIST array_conduit_list;

        NAME_TABLE_LIST table_list;
} DATA_BIN, *DATA_BIN_PTR, **DATA_BIN_HANDLE;

extern "C" FF_STD_ARGS_PTR ff_create_std_args(void);
extern "C" int db_init(struct_ff_std_args *, struct_databin **, int (*)(int));
extern "C" int db_ask(struct_databin *, int,...);

// $Log: FFDods.h,v $
// Revision 1.5  2000/10/11 19:37:56  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.4  1999/05/27 17:02:22  jimg
// Merge with alpha-3-0-0
//
// Revision 1.3.22.1  1999/05/20 21:39:54  edavis
// Fix spelling of COPYRIGHT and remove some #if 0 stuff.
//
// Revision 1.3  1998/04/21 17:13:45  jimg
// Fixes for warnings, etc
//
// Revision 1.2  1998/04/16 18:11:02  jimg
// Sequence support added by Reza

#endif
