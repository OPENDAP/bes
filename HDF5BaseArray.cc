// This file is part of hdf5_handler an HDF5 file handler for the OPeNDAP
// data server.

// Author: Muqun Yang <myang6@hdfgroup.org> 

// Copyright (c) 2011 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  
///////////////////////////////////////////////////////////////////////////////
/// \file HDF5BaseArray.cc
/// \brief Implementation of a helper class that aims to reduce code redundence for different special CF derived array class
/// For example, format_constraint has been called by different CF derived array class,
/// and write_nature_number_buffer has also be used by missing variables on both 
/// HDF-EOS5 and generic HDF5 products. 
/// 
/// This class converts HDF5 array type into DAP array.
/// 
/// \author Kent Yang       (myang6@hdfgroup.org)
/// Copyright (c) 2011 The HDF Group
/// 
/// All rights reserved.
///
/// 
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include <BESDebug.h>

#include "HDF5BaseArray.h"



BaseType *HDF5BaseArray::ptr_duplicate()
{
    return new HDF5BaseArray(*this);
}

// Read in an Array from either an SDS or a GR in an HDF file.
bool HDF5BaseArray::read()
{
    return false;
}


// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5BaseArray::format_constraint (int *offset, int *step, int *count)
{
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();

    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);


        // Check for illegical  constraint
        if (stride < 0 || start < 0 || stop < 0 || start > stop) {
            ostringstream oss;
            oss << "Array/Grid hyperslab indices are bad: [" << start <<
                                ":" << stride << ":" << stop << "]";
            throw Error (malformed_expr, oss.str ());
        }

        // Check for an empty constraint and use the whole dimension if so.
        if (start == 0 && stop == 0 && stride == 0) {
            start = dimension_start (p, false);
            stride = dimension_stride (p, false);
            stop = dimension_stop (p, false);
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1;      // count of elements
        nels *= count[id];              // total number of values for variable

        BESDEBUG ("h5",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

        id++;
        p++;
    }// while (p != dim_end ())

    return nels;
}

void HDF5BaseArray::write_nature_number_buffer(int rank, int tnumelm) {

    if (rank != 1) 
        throw InternalErr(__FILE__, __LINE__, "Currently the rank of the missing field should be 1");
    

    int *offset = new int[rank];
    int *count = new int[rank];
    int *step = new int[rank];
    int *val = 0;

    int nelms;

    try {

        // format_constraint throws on error;
        nelms = format_constraint(offset, step, count);

        // Since we always assign the the missing Z dimension as 32-bit
        // integer, so no need to check the type. The missing Z-dim is always
        // 1-D with natural number 1,2,3,....
        val = new int[nelms];

        if (nelms == tnumelm) {
            for (int i = 0; i < nelms; i++)
                val[i] = i;
            set_value((dods_int32 *) val, nelms);
        }
        else {
            for (int i = 0; i < count[0]; i++)
                val[i] = offset[0] + step[0] * i;
            set_value((dods_int32 *) val, nelms);
        }
    }
    catch (...) {
        delete[] offset;
        delete[] count;
        delete[] step;
        delete[] val;
        throw;
    }

    delete[] offset;
    delete[] count;
    delete[] step;
    delete[] val;

}


