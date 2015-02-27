// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <BaseType.h>
#include <Structure.h>
#include <Array.h>
#include <Int32.h>
#include <Str.h>

#include <util.h>

#include "roi_utils.h"

namespace libdap {

void roi_bbox_valid_slice(BaseType *btp)
{
    // we know it's a Structure * and it has one element because the test above passed
    if (btp->type() != dods_structure_c)
        throw Error("In function roi(): Expected an Array of Structures for the slice information.");

    Structure *slice = static_cast<Structure*>(btp);

    Constructor::Vars_iter i = slice->var_begin();
    if (i == slice->var_end() || (*i)->name() != "start" || (*i)->type() != dods_int32_c)
        throw Error("In function roi(): Could not find valid 'start' field in slice information");

    ++i;
    if (i == slice->var_end() || (*i)->name() != "stop" || (*i)->type() != dods_int32_c)
        throw Error("In function roi(): Could not find valid 'stop' field in slice information");

    ++i;
    if (i == slice->var_end() || (*i)->name() != "name" || (*i)->type() != dods_str_c)
        throw Error("In function roi(): Could not find valid 'name' field in slice information");
}

/**
 * @brief Is the bound box valid?
 *
 * A Bounding Box is an Array of Structure variables that holds the
 * start and stop indices for each dimension of some dependent variable.
 * For each dimension of the dep. variable, there is one dimension in
 * the BBox Array. In addition to the start and stop indices, the
 * Structure holds the dimension's name.
 *
 * @todo Fix the error messages; they say 'in roi()'
 *
 * @param btp Pointer to the Array of Structure that holds the slice information
 * @return The number of slices in the slice array
 * @exception Error Thrown if the array si not valid.
 */
unsigned int roi_valid_bbox(BaseType *btp)
{
    if (!btp)
        throw InternalErr(__FILE__, __LINE__, "roi() function called with null slice array.");

    if (btp->type() != dods_array_c)
        throw Error("In function roi(): Expected last argument to be an Array of slices.");

    Array *slices = static_cast<Array*>(btp);
    if (slices->dimensions() != 1)
        throw Error("In function roi(): Expected last argument to be a one dimensional Array of slices.");

    int rank = slices->dimension_size(slices->dim_begin());
    for (int i = 0; i < rank; ++i) {
        roi_bbox_valid_slice(slices->var(i));
    }

    return rank;
}

/**
 * This method extracts values from one element of the slices Array of Structures.
 * It assumes that the input has been validated.
 *
 * @param slices
 * @param i
 * @param start
 * @param stop
 * @param name
 */
void roi_bbox_get_slice_data(Array *slices, unsigned int i, int &start, int &stop, string &name)
{
    BaseType *btp = slices->var(i);

    Structure *slice = static_cast<Structure*>(btp);
    Constructor::Vars_iter vi = slice->var_begin();

    start = static_cast<Int32*>(*vi++)->value();
    stop = static_cast<Int32*>(*vi++)->value();
    name = static_cast<Str*>(*vi++)->value();
}

} //namespace libdap
