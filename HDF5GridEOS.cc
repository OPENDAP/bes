// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "HDF5GridEOS.h"
#include "H5EOS.h"
#include "debug.h"


extern H5EOS eos;

BaseType *HDF5GridEOS::ptr_duplicate()
{
    return new HDF5GridEOS(*this);
}


HDF5GridEOS::HDF5GridEOS(const string &n, const string &d) : Grid(n, d)
{
    ty_id = -1;
    dset_id = -1;
}

HDF5GridEOS::~HDF5GridEOS()
{
}

bool HDF5GridEOS::read()
{
    if (read_p())               // nothing to do
        return false;

    // Read data array elements.
    array_var()->read();
    // Read map array elements.
    Map_iter p = map_begin();

    while (p != map_end()) {
        Array *a = dynamic_cast < Array * >(*p);
        if (!a)
	    throw InternalErr(__FILE__, __LINE__, "null pointer");
        read_dimension(a);
        ++p;
    }
    set_read_p(true);

    return false;
}

void HDF5GridEOS::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5GridEOS::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5GridEOS::get_did()
{
    return dset_id;
}

hid_t HDF5GridEOS::get_tid()
{
    return ty_id;
}

void HDF5GridEOS::read_dimension(Array * a)
{
    Array::Dim_iter d = a->dim_begin();
    int start = a->dimension_start(d, true);
    int stride = a->dimension_stride(d, true);
    int stop = a->dimension_stop(d, true);
    int count = ((stop - start) / stride) + 1;
    string dim_name = a->name();
#ifdef CF
    dim_name = eos.get_EOS_name(dim_name);
#endif
    int loc = eos.get_dimension_data_location(dim_name);
    DBG(cerr << "Dim name=" << dim_name << " location=" << loc << endl);
    if (loc >= 0) {
        a->set_read_p(true);
        dods_float32 *val =
	    get_dimension_data(eos.dimension_data[loc], start, stride,
                               stop, count);
        a->set_value(val, count);
        delete[]val;
    } 
    else {
        cerr << "Could not retrieve map data" << endl;
    }
}

dods_float32 *HDF5GridEOS::get_dimension_data(dods_float32 * buf,
                                              int start, int stride,
                                              int stop, int count)
{
    int i = 0;
    int j = 0;
    dods_float32 *dim_buf = NULL;
    DBG(cerr << ">get_dimension_data():stride=" << stride << " count=" <<
        count << endl);

    if (buf == NULL) {
        cerr << "HDF5GridEOS.cc::get_dimension_data(): argument buf is NULL."
	     << endl;
        return dim_buf;
    }

    dim_buf = new dods_float32[count];
    for (i = start; i <= stop; i = i + stride) {
        DBG(cerr << "=get_dimension_data():i=" << i << " j=" << j << endl);
        dim_buf[j] = buf[i];
	DBG(cerr << "=get_dimension_data():dim_buf[" << j << "] =" 
	    << dim_buf[j] << endl);
        j++;
    }
    if (count != j) {
        cerr << "HDF5GridEOS.cc::get_dimension_data(): index mismatch" <<
            endl;
    }
    DBG(cerr << "<get_dimension_data()" << endl);

    return dim_buf;
}
