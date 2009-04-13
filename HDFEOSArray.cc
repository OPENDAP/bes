/*-------------------------------------------------------------------------
 * Copyright (C) 2008	The HDF Group
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

// #define DODS_DEBUG
// #define CF

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

#include "dhdferr.h"
#include "HDFEOSArray.h"
#include "HDFEOS.h"


using namespace std;
extern HDFEOS eos;

BaseType *HDFEOSArray::ptr_duplicate()
{
    return new HDFEOSArray(*this);
}

HDFEOSArray::HDFEOSArray(const string & n, const string &d, BaseType * v)
    : Array(n, d, v)
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
#ifdef CF
    dim_name = eos.get_EOS_name(dim_name);
#endif
    int loc = eos.get_dimension_data_location(dim_name);

    if (loc >= 0) {
        // set_value() will call this function.
        // set_read_p(true); <hyokyung 2008.07.18. 13:40:51>
        dods_float32 *val =
            get_dimension_data(eos.dimension_data[loc], start, stride,
                               stop, count);
	// We need to use Vector::set_value() instead of Vector::value() since 
	// Vector::value(dods_float32* b) function doesn't check if _buf is null
	// and the _buf needs memory allocation.
	set_value(val, count);
	//value(val); // <hyokyung 2008.07.18. 13:40:42>
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
