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
/// \file HDFEOS5CFMissLLArray.cc
/// \brief The implementation of the retrieval of the missing lat/lon values for HDF-EOS5 products
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
#include <Array.h>

#include "HDFEOS5CFMissLLArray.h"



BaseType *HDFEOS5CFMissLLArray::ptr_duplicate()
{
    return new HDFEOS5CFMissLLArray(*this);
}

bool HDFEOS5CFMissLLArray::read()
{
    int nelms = -1;
    vector<int>offset;
    vector<int>count;
    vector<int>step;

    if (eos5_projcode != HE5_GCTP_GEO) 
        throw InternalErr (__FILE__, __LINE__,"The projection is not supported.");
                          

    if (rank <=  0) 
       throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of this variable should be greater than 0");
    else {

         offset.resize(rank);
         count.resize(rank);
         step.resize(rank);
         nelms = format_constraint (&offset[0], &step[0], &count[0]);
    }

    if (nelms <= 0) 
       throw InternalErr (__FILE__, __LINE__,
                          "The number of elments is negative.");


    float start = 0.0;
    float end   = 0.0;

    vector<float>val;
    val.resize(nelms);
    

    if (CV_LAT_MISS == cvartype) {
        
        if (HE5_HDFE_GD_UL == eos5_origin || HE5_HDFE_GD_UR == eos5_origin) {

            start = point_upper;
            end   = point_lower; 

        }
        else {// (gridorigin == HE5_HDFE_GD_LL || gridorigin == HE5_HDFE_GD_LR)
        
            start = point_lower;
            end = point_upper;
        }

        if(ydimsize <=0) 
           throw InternalErr (__FILE__, __LINE__,
                          "The number of elments should be greater than 0.");
           
        float lat_step = (end - start) /ydimsize;

        // FIXME There are ways to get to this code where offset and step are still null
        // or have not been allocated
        // Now offset,step and val will always be valid. line 74 and 85 assure this.
        if ( HE5_HDFE_CENTER == eos5_pixelreg ) {
            for (int i = 0; i < nelms; i++)
                val[i] = ((float)(offset[0]+i*step[0] + 0.5f) * lat_step + start) / 1000000.0;
        }
        else { // HE5_HDFE_CORNER 
            for (int i = 0; i < nelms; i++)
                val[i] = ((float)(offset[0]+i * step[0])*lat_step + start) / 1000000.0;
        }

    }

    if (CV_LON_MISS == cvartype) {

        if (HE5_HDFE_GD_UL == eos5_origin || HE5_HDFE_GD_LL == eos5_origin) {

            start = point_left;
            end   = point_right; 

        }
        else {// (gridorigin == HE5_HDFE_GD_UR || gridorigin == HE5_HDFE_GD_LR)
        
            start = point_right;
            end = point_left;
        }

        if(xdimsize <=0) 
           throw InternalErr (__FILE__, __LINE__,
                          "The number of elments should be greater than 0.");
        float lon_step = (end - start) /xdimsize;

        // FIXME offset and step
        // fixed already.
        if (HE5_HDFE_CENTER == eos5_pixelreg) {
            for (int i = 0; i < nelms; i++)
                val[i] = ((float)(offset[0] + i *step[0] + 0.5f) * lon_step + start ) / 1000000.0;
        }
        else { // HE5_HDFE_CORNER 
            for (int i = 0; i < nelms; i++)
                val[i] = ((float)(offset[0]+i*step[0]) * lon_step + start) / 1000000.0;
        }

    }

#if 0
for (int i =0; i <nelms; i++) 
cerr <<"final data val "<< i <<" is " << val[i] <<endl;
#endif

    set_value ((dods_float32 *) &val[0], nelms);
    return false;
}

// parse constraint expr. and make hdf5 coordinate point location.
// return number of elements to read. 
int
HDFEOS5CFMissLLArray::format_constraint (int *offset, int *step, int *count)
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


