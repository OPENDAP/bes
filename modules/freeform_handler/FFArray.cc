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

// (c) COPYRIGHT URI/MIT 1997-99
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors: reza (Reza Nekovei)

// FreeFrom sub-class implementation for FFByte,...FFGrid.
// The files are patterned after the subcalssing examples
// Test<type>.c,h files.
//
// ReZa 6/18/97

#include "config_ff.h"

static char rcsid[] not_used = { "$Id$" };

#include <cstring>
#include <iostream>
#include <string>

#include <BESDebug.h>

#include <dods-datatypes.h>
#include <D4Attributes.h>
#include <D4Group.h>
#include <Error.h>
#include <InternalErr.h>

#include "FFArray.h"
#include "util_ff.h"
#include "util.h"

BaseType *
FFArray::ptr_duplicate()
{
    return new FFArray(*this);
}

FFArray::FFArray(const string &n, const string &d, BaseType *v, const string &iff) :
        Array(n, d, v), d_input_format_file(iff)
{
}

FFArray::~FFArray()
{
}

// parse constraint expr. and make coordinate point location for an array.
// return number of elements to read.

long FFArray::Arr_constraint(long *cor, long *step, long *edg, string *dim_nms, bool *has_stride)
{
    long start, stride, stop;

    int id = 0;
    long nels = 1;
    *has_stride = false;

    Array::Dim_iter i = dim_begin();
    while (i != dim_end()) {
        start = (long) dimension_start(i, true);
        stride = (long) dimension_stride(i, true);
        stop = (long) dimension_stop(i, true);
        string dimname = dimension_name(i);
#if 0
        // Check for empty constraint
        if (start + stop + stride == 0)
            return -1;
#endif
        // This code is a correct version of the above, but it _should_ never be called
        // since libdap::Vector::serialize() will never call Array::read() when length is
        // zero. Still, a server function might... jhrg 2/17/16
        if (length() == 0)
            return -1;

        dim_nms[id] = dimname;
        //	(void) strcpy(dim_nms[id], dimname.c_str());

        cor[id] = start;
        step[id] = stride;
        edg[id] = ((stop - start) / stride) + 1; // count of elements

        nels *= edg[id]; // total number of values for variable

        if (stride != 1)
            *has_stride = true;

        ++id;
        ++i;
    }
    return nels;
}

#if 0
// parse constraint expr. and make coordinate point location.
// return number of elements to read.

long FFArray::Seq_constraint(long *cor, long *step, long *edg, bool *has_stride)
{
    int start, stride, stop;
    int id = 0;
    long nels = 1;
    *has_stride = false;

    Array::Dim_iter i = dim_begin();
    while (i != dim_end()) {
        start = (long) dimension_start(i, true);
        stride = (long) dimension_stride(i, true);
        stop = (long) dimension_stop(i, true);
#if 0
        // Check for empty constraint
        if (start + stop + stride == 0)
            return -1;
#endif
        if (length() == 0)
            return -1;

        cor[id] = start;
        step[id] = stride;
        edg[id] = ((stop - start) / stride) + 1; // count of elements
        nels *= edg[id]; // total number of values for variable
        if (stride != 1)
            *has_stride = true;

        ++id;
        ++i;
    }
    return nels;
}
#endif

#if 0
static int hyper_get(void *dest, void *src, unsigned szof, const int dim_num, int index, const int dimsz[],
                     const long start[], const long edge[])
{
    long jump = 1;

    // The THEN part of this IF handles the cases where we are hyper-slabbing
    // any dimension *other* than the rightmost dimension. E.G. Suppose
    // a[10][10][10] is in SRC, for INDEX == 0 and 1 we do the code in the
    // THEN clause and for INDEX == 2 the ELSE clause is executed.

    // NOTE: I have added casts for src and dest from void * to char * since
    // ANSI C++ won't allow pointer arithmetic on void * variables. 4/17/98
    // jhrg

    if (dim_num != (index + 1)) {
        // number of lines, pages, etc to skip
        for (int i = dim_num - 1; i > index; i--)
            jump *= dimsz[i];

        for (int edge_tmp = 0; edge_tmp < edge[index]; edge_tmp++) {
            void *srctmp = ((char *) src + (start[index] + edge_tmp) * jump * szof);
            dest = (char *) dest + hyper_get(dest, srctmp, szof, dim_num, index + 1, dimsz, start, edge);
        }
        return (0);
    }
    else {
        // number of bytes to jump
        void *srctmp = (char *) src + start[index] * szof;
        memcpy(dest, srctmp, (size_t) edge[index] * szof);
        return (edge[index] * szof);
    }
}
#endif

#if 0
template<class T>
static void seq2vects(T * t, FFArray & array)
{
    bool has_stride;
    int ndim = array.dimensions();
    long *start = new long[ndim];
    long *stride = new long[ndim];
    long *edge = new long[ndim];

    long count = array.Seq_constraint(start, stride, edge, &has_stride);

    if (count != -1) { // non-null hyperslab
        T *t_hs = new T[count];
        int *dimsz = new int[array.dimensions()];

        int i = 0;
        Array::Dim_iter p = array.dim_begin();
        while (p != array.dim_end()) {
            dimsz[i] = array.dimension_size(p);
            ++i;
            ++p;
        }

        hyper_get(t_hs, t, array.var()->width(), ndim, 0, dimsz, start, edge);

        array.set_read_p(true); // reading is done
        array.val2buf((void *) t_hs); // put values in the buffer

        delete[] t_hs;
        delete[] dimsz;
    }
    else {
        array.set_read_p(true);
        array.val2buf((void *) t);
    }

    delete[] start;
    delete[] stride;
    delete[] edge;
}
#endif

// Read cardinal types and ctor types separately. Cardinal types are
// stored in arrays of the C++ data type while ctor types are stored
// in arrays of the DODS classes used to store those types.
//
// NB: Currently this code only reads arrays of Byte, Int32 and
// Float64. Str and Url as well as all the ctor  types are not
// supported.
//
// Throws an Error object if an error was detected.
// Returns true if more data still needs to be read, otherwise returns false.

bool FFArray::read()
{
    if (read_p()) // Nothing to do
        return true;

    bool has_stride;
    int ndims = dimensions();
    vector<string> dname(ndims);
    vector<long> start(ndims);
    vector<long> stride(ndims);
    vector<long> edge(ndims);
    long count = Arr_constraint(&start[0], &stride[0], &edge[0], &dname[0], &has_stride);

    if (!count) {
        throw Error(unknown_error, "Constraint returned an empty dataset.");
    }

    string output_format = makeND_output_format(name(), var()->type(), var()->width(),
            ndims, &start[0], &edge[0], &stride[0], &dname[0]);

    // For each cardinal-type variable, do the following:
    //     Use ff to read the data
    //     Store the (possibly constrained) data
    //     NB: extract_array throws an Error object to signal problems.

    switch (var()->type()) {
    case dods_byte_c:
        extract_array<dods_byte>(dataset(), d_input_format_file, output_format);
        break;

    case dods_int16_c:
        extract_array<dods_int16>(dataset(), d_input_format_file, output_format);
        break;

    case dods_uint16_c:
        extract_array<dods_uint16>(dataset(), d_input_format_file, output_format);
        break;

    case dods_int32_c:
        extract_array<dods_int32>(dataset(), d_input_format_file, output_format);
        break;

    case dods_uint32_c:
        extract_array<dods_uint32>(dataset(), d_input_format_file, output_format);
        break;

    case dods_float32_c:
        extract_array<dods_float32>(dataset(), d_input_format_file, output_format);
        break;

    case dods_float64_c:
        extract_array<dods_float64>(dataset(), d_input_format_file, output_format);
        break;

    default:
        throw InternalErr(__FILE__, __LINE__,
                (string) "FFArray::read: Unsupported array type " + var()->type_name() + ".");
    }

    return true;
}

// This template reads arrays of simple types into the Array object's _buf
// member. It returns true if successful, false otherwise.

template<class T>
bool FFArray::extract_array(const string &ds, const string &if_fmt, const string &o_fmt)
{
    vector<T> d(length());
    long bytes = read_ff(ds.c_str(), if_fmt.c_str(), o_fmt.c_str(), (char *) &d[0], width());

    BESDEBUG("ff", "FFArray::extract_array: Read " << bytes << " bytes." << endl);

    if (bytes == -1) {
        throw Error(unknown_error, "Could not read values from the dataset.");
    }
    else {
        set_read_p(true);
        set_value(d, d.size());
    }

    return true;
}
