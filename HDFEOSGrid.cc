// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2008 The HDF Group
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

/////////////////////////////////////////////////////////////////////////////
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <vector>
// Include this on linux to suppres an annoying warning about multiple
// definitions of MIN and MAX.
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <mfhdf.h>
#include <hdfclass.h>
#include <hcstream.h>

#include "escaping.h"
#include "HDFEOSGrid.h"
#include "HDFArray.h"
#include "hdfutil.h"
#include "dhdferr.h"
#include "HDFEOS.h"
#include "Error.h"
#include "debug.h"
#ifdef USE_HDFEOS2_LIB
#include "HDFEOS2Array.h"
#endif
extern HDFEOS eos;


void LoadArrayFromSDS(HDFArray * ar, const hdf_sds & sds);
void LoadArrayFromGR(HDFArray * ar, const hdf_gri & gr);

HDFEOSGrid::HDFEOSGrid(const string &n, const string &d) : Grid(n, d)
{
}

HDFEOSGrid::~HDFEOSGrid()
{
}
BaseType *HDFEOSGrid::ptr_duplicate()
{
    return new HDFEOSGrid(*this);
}

// void LoadGridFromSDS(HDFEOSGrid * gr, const hdf_sds & sds);

// Build a vector of array_ce structs. This holds the constraint
// information *for each map* of the Grid.
vector < array_ce > HDFEOSGrid::get_map_constraints()
{
    vector < array_ce > a_ce_vec;

    // Load the array_ce vector with info about each map vector.
    for (Grid::Map_iter p = map_begin(); p != map_end(); ++p) {
        Array & a = dynamic_cast < Array & >(**p);
        Array::Dim_iter q = a.dim_begin();      // maps have only one dimension.
        int start = a.dimension_start(q, true);
        int stop = a.dimension_stop(q, true);
        int stride = a.dimension_stride(q, true);
        int edge = (int) ((stop - start) / stride) + 1;
        array_ce a_ce(a.name(), start, edge, stride);
        a_ce_vec.push_back(a_ce);
    }

    return a_ce_vec;
}

// Read in a Grid from an SDS in an HDF file.
bool HDFEOSGrid::read()
{
    int err = 0;
    int status = read_tagref(-1, -1, err);
    if (err)
        throw Error(unknown_error, "Could not read from dataset.");
    return status;
}

bool HDFEOSGrid::read_tagref(int32 tag, int32 ref, int &err)
{
  // cerr << ">HDFEOSGrid::read_tagref()" << endl; // <hyokyung 2008.11.13. 09:28:11>
  string hdf_file = dataset();
  string hdf_name = this->name();

#ifdef USE_HDFEOS2_LIB
  HDFEOS2Array *primary_array = dynamic_cast < HDFEOS2Array * >(array_var());    
#else  
  HDFArray *primary_array = dynamic_cast < HDFArray * >(array_var());    
#endif     

  primary_array->read();

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

void HDFEOSGrid::read_dimension(Array * a)
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
#if 1
	// This code was comented out but appears to be correct. The code
	// below using val2buf() leaks memory. This code passes all the
	// tests, however, So I'm using it. 4/9/2008 jhrg
        dods_float32 *val =
	    get_dimension_data(eos.dimension_data[loc], start, stride,
                               stop, count);
        a->set_value(val, count);
        delete[]val;
#else
       a->val2buf((void *)
          get_dimension_data(eos.dimension_data[loc], start, stride, stop, count));
#endif
    } 
    else {
        cerr << "Could not retrieve map data" << endl;
    }
}

dods_float32 *HDFEOSGrid::get_dimension_data(dods_float32 * buf,
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
