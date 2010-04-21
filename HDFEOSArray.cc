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

#include "HDFEOSArray.h"
#include "HDFEOS.h"


using namespace std;
extern HDFEOS eos;

BaseType *HDFEOSArray::ptr_duplicate()
{
    return new HDFEOSArray(*this);
}

HDFEOSArray::HDFEOSArray(const string & n, const string &d, BaseType * v)
    : HDFArray(n, d, v)
{
}

HDFEOSArray::~HDFEOSArray()
{
}


bool HDFEOSArray::read()
{
    DBG(cerr << ">read(): " << name() << endl);

    Array::Dim_iter d = dim_begin();
    int start = dimension_start(d, true);
    int stride = dimension_stride(d, true);
    int stop = dimension_stop(d, true);
    int count = ((stop - start) / stride) + 1;
    string dim_name = name();

    dim_name = eos.get_EOS_name(dim_name);

    int loc = eos.get_dimension_data_location(dim_name);

    if (loc >= 0) {
        dods_float32 *val =
            get_dimension_data(eos.dimension_data[loc], start, stride,
                               stop, count);
	set_value(val, count);

        delete[]val;	
    } else {
        cerr << "Could not retrieve map data" << endl;
    }
    return false;
}


dods_float32 *HDFEOSArray::get_dimension_data(dods_float32 * buf,
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
        cerr << "HDFEOSArray::get_dimension_data(): index mismatch" <<
            endl;
    }
    return dim_buf;
}
