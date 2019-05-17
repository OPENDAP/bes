
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

// Prototypes for the FreeForm server utility functions.
//
// jhrg 4/2/96

// $Id$

#ifndef _util_ff_h
#define _util_ff_h

#include "FreeFormCPP.h"

#include "BaseType.h"
#include "Error.h"
#include "InternalErr.h"
#include "dods-datatypes.h"

using namespace libdap ;

const int Msgt_size = 255;

void free_ff_char_vector(char **v, int len);

const string ff_types(Type dods_type);
int ff_prec(Type dods_type);

const string make_output_format(const string &name, Type type,
				const int width);

const string makeND_output_format(const string &name, Type type,
				  const int width, int ndim,
				  const long *start, const long *edge, const
				  long * stride, string *dname);

const string &format_extension(const string &new_extension = "");
const string &format_delimiter(const string &new_delimiter = "");

// *** Cruft? see .cc file
#if 0
const string find_ancillary_formats(const string &dataset,
				    const string &delimiter = format_delimiter(),
				    const string &extension = format_extension());
#endif
const string find_ancillary_rss_formats(const string &dataset,
					const string &delimiter = format_delimiter(),
					const string &extension = format_extension());

const string find_ancillary_rss_das(const string &dataset,
					const string &delimiter = format_delimiter(),
					const string &extension = format_extension());

int SetDodsDB(FF_STD_ARGS_PTR std_args, DATA_BIN_HANDLE dbin_h, char * Msgt);
long Records(const string &filename);

bool file_exist(const char * filename);

/*extern "C" */long read_ff(const char *dataset, const char *if_file, const char *o_format,
                        char *o_buffer, unsigned long size);

bool is_integer_type(BaseType *btp);
bool is_float_type(BaseType *btp);
dods_uint32 get_integer_value(BaseType *var) throw(InternalErr);
dods_float64 get_float_value(BaseType *var) throw(InternalErr);

string get_Regex_format_file(const string &filename);

#endif // _util_ff_h_






