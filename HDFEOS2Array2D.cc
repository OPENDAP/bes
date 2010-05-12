// -*- C++ -*-
//
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Copyright (c) 2008-2009 The HDF Group
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
//
//
// Author: Hyo-Kyung Lee
//         hyoklee@hdfgroup.org
//
/////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <memory>
#include <sstream>
#include <ctype.h>

#include "config_hdf.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>

#include <hdfclass.h>
#include <hcstream.h>

#include <escaping.h>
#include <Error.h>
#include <debug.h>

#include "HDFEOS2Array2D.h"
#include "HDFEOS.h"


extern HDFEOS eos;

BaseType *HDFEOS2Array2D::ptr_duplicate()
{
    return new HDFEOS2Array2D(*this);
}

HDFEOS2Array2D::HDFEOS2Array2D(const string & n, BaseType * v)
    : HDFArray(n, v)
{
    d_num_dim = 2;  
}

HDFEOS2Array2D::~HDFEOS2Array2D()
{
}


/// KENT: This code may not be used at all!! 2009/12/8,will test.
/// Currently the 2-D grid cases all have multiple groups. 
/// OPeNDAP clients simply cannot handle. Will go back to this
/// case later.
bool HDFEOS2Array2D::read()
{
  
    DBG(cerr << ">HDFEOS2Array2D::read(): " << name() << endl);

    if(!eos.is_orthogonal()){   // It must be a 2-D case.
    
        string dim_name = name();
        dim_name = eos.get_EOS_name(dim_name);
        int loc = eos.get_dimension_data_location(dim_name);
        int* offset = new int[d_num_dim];
        int* count = new int[d_num_dim];
        int* step = new int[d_num_dim];
        int nelms = format_constraint(offset, step, count);
        int count2 = eos.get_xdim_size() * eos.get_ydim_size();
    
        if(nelms ==  count2){    
            set_value(eos.dimension_data[loc], count2);
        }
        else{
            int *picks = new int[nelms];
            int total_elems = linearize_multi_dimensions(offset, step, count,
                                                         picks);
            dods_float32* slab = new dods_float32[nelms];
            for(int i=0; i < nelms; i++){
                dods_float32* f = (dods_float32 *)eos.dimension_data[loc];
                DBG(cerr << i << ":" << f[picks[i]] << endl);
                slab[i] = f[picks[i]];
            }
            set_value(slab,nelms);
            delete[] slab;
            delete[] picks;      
        }
        delete[] offset;
        delete[] count;
        delete[] step;
    
        return false;
    }
    else{
        throw InternalErr(__FILE__, __LINE__,
                          "This file does not use 2-D projection.");
        return true;
    }
}


int HDFEOS2Array2D::format_constraint(int *offset, int *step, int *count) {
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin();

    while (p != dim_end()) {

        int start = dimension_start(p, true);
        int stride = dimension_stride(p, true);
        int stop = dimension_stop(p, true);

        // Check for empty constraint
        if (stride <= 0 || start < 0 || stop < 0 || start > stop) {
            ostringstream oss;

            oss << "Array/Grid hyperslab indices are bad: [" << start <<
                ":" << stride << ":" << stop << "]";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1; // count of elements
        nels *= count[id]; // total number of values for variable

        DBG(cerr
            << "=format_constraint():"
            << "id=" << id << " offset=" << offset[id]
            << " step=" << step[id]
            << " count=" << count[id]
            << endl);

        id++;
        p++;
    }

    return nels;
}

int HDFEOS2Array2D::linearize_multi_dimensions(int *start,
                                               int *stride,
                                               int *count,
                                               int *picks)
{
    DBG(cerr << ">linearize_multi_dimensions()" << endl);
    int total = 1;
    int *dim = 0;
    int *temp_count = 0;
    try {
        int id = 0;
        dim = new int[d_num_dim];
        Dim_iter p2 = dim_begin();

        while (p2 != dim_end()) {
            int a_size = dimension_size(p2, false); // unconstrained size
            DBG(cerr << "dimension[" << id << "] = " << a_size << endl);
            dim[id] = a_size;
            total = total * a_size;
            ++id;
            ++p2;
        } // while()

        // Kent's contribution.
        temp_count = new int[d_num_dim];
        int temp_index;
        int i;
        int array_index = 0;
        int temp_count_dim = 0; // This variable changes when dim. is added. 
        int temp_dim = 1;

        for (i = 0; i < d_num_dim; i++)
            temp_count[i] = 1;

        int num_ele_so_far = 0;
        int total_ele = 1;
        for (i = 0; i < d_num_dim; i++)
            total_ele = total_ele * count[i];

        while (num_ele_so_far < total_ele) {
            // loop through the index 

            while (temp_count_dim < d_num_dim) {
                temp_index = (start[d_num_dim - 1 - temp_count_dim]
                              + (temp_count[d_num_dim - 1 - temp_count_dim] - 1)
                              * stride[d_num_dim - 1 -
                                       temp_count_dim]) * temp_dim;
                array_index = array_index + temp_index;
                temp_dim = temp_dim * dim[d_num_dim - 1 - temp_count_dim];
                temp_count_dim++;
            }

            picks[num_ele_so_far] = array_index;

            num_ele_so_far++;
            // index can be added 
            DBG(cerr << "number of element looped so far = " <<
                num_ele_so_far << endl);
            for (i = 0; i < d_num_dim; i++) {
                DBG(cerr << "temp_count[" << i << "]=" << temp_count[i] <<
                    endl);
            }
            DBG(cerr << "index so far " << array_index << endl);

            temp_dim = 1;
            temp_count_dim = 0;
            array_index = 0;

            for (i = 0; i < d_num_dim; i++) {
                if (temp_count[i] < count[i]) {
                    temp_count[i]++;
                    break;
                } 
                else { // We reach the end of the dimension, set it to 1 and
                    // increase the next level dimension.  
                    temp_count[i] = 1;
                }
            }
        }

        delete[] dim;
        delete[] temp_count;
    }
    catch(...) {
        delete[] dim;
        delete[] temp_count;
        throw;
    }

    DBG(cerr << "<linearize_multi_dimensions()" << endl);
    return total;
}
