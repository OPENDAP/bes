// This file is part of the hdf5_handler implementing for the CF-compliant
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

/////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS5CFMissLLArray.cc
/// \brief The implementation of the retrieval of the missing lat/lon values for HDF-EOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011 The HDF Group
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
    int* offset = NULL;
    int* count = NULL ;
    int* step = NULL;
    hsize_t* hoffset = NULL;
    hsize_t* hcount = NULL;
    hsize_t* hstep = NULL;
    int nelms = -1;

#if 0
cerr<<"coming to read function"<<endl;
cerr<<"file name " <<filename <<endl;
cerr <<"var name "<<varname <<endl;
#endif
    if (eos5_projcode != HE5_GCTP_GEO) 
        throw InternalErr (__FILE__, __LINE__,"The projection is not supported.");
                          

    if (rank < 0) 
        throw InternalErr (__FILE__, __LINE__,
                          "The number of dimension of the variable is negative.");

    else if (rank == 0) 
        nelms = 1;
    else {
        try {
            offset = new int[rank];
            count = new int[rank];
            step = new int[rank];
            hoffset = new hsize_t[rank];
            hcount = new hsize_t[rank];
            hstep = new hsize_t[rank];
        }

        catch (...) {
            HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
            throw InternalErr (__FILE__, __LINE__,
                          "Cannot allocate the memory for offset,count and setp.");
        }

        nelms = format_constraint (offset, step, count);

        for (int i = 0; i <rank; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }
    }

    float start = 0.0;
    float end   = 0.0;
    float *val  = NULL;
    try {
        val = new float[nelms];
    }
    catch (...) {
        HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);
        throw InternalErr (__FILE__, __LINE__, "Cannot allocate memory buffer.");
    }


    if (CV_LAT_MISS == cvartype) {
        
        if (HE5_HDFE_GD_UL == eos5_origin || HE5_HDFE_GD_UR == eos5_origin) {

            start = point_upper;
            end   = point_lower; 

        }
        else {// (gridorigin == HE5_HDFE_GD_LL || gridorigin == HE5_HDFE_GD_LR)
        
            start = point_lower;
            end = point_upper;
        }
           
        float lat_step = (end - start) /ydimsize;

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
        float lon_step = (end - start) /xdimsize;

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

    set_value ((dods_float32 *) val, nelms);
    if (val !=NULL) 
        delete[] (float*)val;

    HDF5CFUtil::ClearMem(offset,count,step,hoffset,hcount,hstep);

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


