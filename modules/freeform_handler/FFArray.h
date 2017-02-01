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

// FreeFrom sub-class implementation for FFByte,...FFGrid.
// The files are patterned after the subcalssing examples
// Test<type>.c,h files.
//
// ReZa 6/18/97

#ifndef _ffarray_h
#define _ffarray_h 1

#include <string>

#include "Array.h"

using namespace libdap;

class FFArray: public Array {
private:
    string d_input_format_file;


    long Arr_constraint(long *cor, long *step, long *edg, string *dim_nms, bool *has_stride);

    /** Read an array of simple types into this objects _buf field. */
    template<class T> bool extract_array(const string &ds, const string &if_fmt, const string &o_fmt);

public:
    FFArray(const string &n, const string &d, BaseType *v, const string &iff);
    virtual ~FFArray();

    virtual BaseType *ptr_duplicate();

    virtual bool read();
};

#endif
