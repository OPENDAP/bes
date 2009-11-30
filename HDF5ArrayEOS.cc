// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2007,2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

// #define DODS_DEBUG

#include <iostream>
#include <memory>
#include <sstream>
#include <ctype.h>

#include "config_hdf5.h"
#include "debug.h"
#include "Error.h"
#include "InternalErr.h"
#include "HDF5ArrayEOS.h"
#include "H5EOS.h"
#include "h5dds.h"

using namespace std;
extern H5EOS eos;

BaseType *HDF5ArrayEOS::ptr_duplicate()
{
    return new HDF5ArrayEOS(*this);
}

HDF5ArrayEOS::HDF5ArrayEOS(const string & n, const string &d, BaseType * v)
    : Array(n, d, v)
{
}

HDF5ArrayEOS::~HDF5ArrayEOS()
{
}


bool HDF5ArrayEOS::read()
{
    DBG(cerr << ">read(): " << name() << endl);

    Array::Dim_iter d = dim_begin();
    int start = dimension_start(d, true);
    int stride = dimension_stride(d, true);
    int stop = dimension_stop(d, true);
    int count = ((stop - start) / stride) + 1;
    string dim_name = name();
#ifdef CF
    dim_name = eos.get_EOS_name(dim_name);
#endif
    int loc = eos.get_dimension_data_location(dim_name);

    if (loc >= 0) {
        // set_value() will call this function.
        // set_read_p(true); 
        dods_float32 *val =
            get_dimension_data(eos.dimension_data[loc], start, stride,
                               stop, count);
	// We need to use Vector::set_value() instead of Vector::value() since 
	// Vector::value(dods_float32* b) function doesn't check if _buf is null
	// and the _buf needs memory allocation.
	set_value(val, count);
	// value(val); 
        delete[]val;	
    } else {
        cerr << "Could not retrieve map data" << endl;
    }
    return false;
}

// public functions to set all parameters needed in read function.

void HDF5ArrayEOS::set_memneed(size_t need)
{
    d_memneed = need;
}

void HDF5ArrayEOS::set_numdim(int ndims)
{
    d_num_dim = ndims;
}

void HDF5ArrayEOS::set_numelm(int nelms)
{
    d_num_elm = nelms;
}


dods_float32 *HDF5ArrayEOS::get_dimension_data(dods_float32 * buf,
                                               int start, int stride,
                                               int stop, int count)
{
    int i = 0;
    int j = 0;
    dods_float32 *dim_buf = NULL;
    dim_buf = new dods_float32[count];
    for (i = start; i <= stop; i = i + stride) {
        dim_buf[j] = buf[i];
        j++;
    }
    if (count != j) {
        cerr << "HDF5ArrayEOS::get_dimension_data(): index mismatch" <<
            endl;
    }
    return dim_buf;
}
