// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFSpecialCVArray.cc
/// \brief Implementation of the retrieval of special coordinate variable values(mostly from product specification) for general HDF5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#include "config_hdf5.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include <BESDebug.h>

#include "HDF5GMCFSpecialCVArray.h"


BaseType *HDF5GMCFSpecialCVArray::ptr_duplicate()
{
    return new HDF5GMCFSpecialCVArray(*this);
}

bool HDF5GMCFSpecialCVArray::read()
{

    // Here we still use vector just in case we need to tackle "rank>1" in the future.
    // Also we would like to keep it consistent with other similar handlings.

    vector<int>offset;
    vector<int>count;
    vector<int>step;

    int rank = 1;
    offset.resize(rank);
    count.resize(rank);
    step.resize(rank);
        
    int nelms = format_constraint (&offset[0], &step[0], &count[0]);

    if (GPMS_L3 == product_type || GPMM_L3 == product_type) {
        if(varname=="nlayer" && 28 == tnumelm) 
            obtain_gpm_l3_layer(nelms,offset,step,count);
        else if(varname=="hgt" && 5 == tnumelm)
            obtain_gpm_l3_hgt(nelms,offset,step,count);
        else if(varname=="nalt" && 5 == tnumelm)
            obtain_gpm_l3_nalt(nelms,offset,step,count);
    }
        

    return true;
}

void HDF5GMCFSpecialCVArray::obtain_gpm_l3_layer(int nelms,vector<int>&offset,vector<int>&step,vector<int>&count) {


    vector<float>total_val;
    total_val.resize(tnumelm);
    for (int i = 0; i<20;i++)
        total_val[i] = 0.5*(i+1);

    for (int i = 20; i<28;i++)
        total_val[i] = total_val[19]+(i-19);

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        set_value ((dods_float32 *) &total_val[0], nelms);
    }
    else {

        vector<float>val;
        val.resize(nelms);
        
        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value ((dods_float32 *) &val[0], nelms);
    }
}

void HDF5GMCFSpecialCVArray::obtain_gpm_l3_hgt(int nelms,vector<int>&offset,vector<int>&step,vector<int>&count) {


    vector<float>total_val;
    total_val.resize(5);
    total_val[0] = 2;
    total_val[1] = 4;
    total_val[2] = 6;
    total_val[3] = 10;
    total_val[4] = 15;

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        set_value ((dods_float32 *) &total_val[0], nelms);
    }
    else {

        vector<float>val;
        val.resize(nelms);
        
        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value ((dods_float32 *) &val[0], nelms);
    }
}

void HDF5GMCFSpecialCVArray::obtain_gpm_l3_nalt(int nelms,vector<int>&offset,vector<int>&step,vector<int>&count) {


    vector<float>total_val;
    total_val.resize(5);

    total_val[0] = 2;
    total_val[1] = 4;
    total_val[2] = 6;
    total_val[3] = 10;
    total_val[4] = 15;


    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        set_value ((dods_float32 *) &total_val[0], nelms);
    }
    else {

        vector<float>val;
        val.resize(nelms);
        
        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value ((dods_float32 *) &val[0], nelms);
    }
}


// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDF5GMCFSpecialCVArray::format_constraint (int *offset, int *step, int *count)
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
        }

        return nels;
}

