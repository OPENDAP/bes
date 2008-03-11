/*-------------------------------------------------------------------------
 * Copyright (C) 2007	The HDF Group
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

#ifdef __GNUG__
#pragma implementation
#endif

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

HDF5ArrayEOS::HDF5ArrayEOS(const string & n, BaseType * v):Array(n, v)
{

}

HDF5ArrayEOS::~HDF5ArrayEOS()
{
}


bool HDF5ArrayEOS::read(const string & dataset)
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
        set_read_p(true);
        dods_float32 *val =
            get_dimension_data(eos.dimension_data[loc], start, stride,
                               stop, count);
        value(val);
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
