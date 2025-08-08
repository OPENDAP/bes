/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF4 SDS values for a direct DMR-mapping DAP4 response.
// Each SDS will be mapped to a DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////


#include "HDFDMRArray_SPLL.h"
#include "hdf.h"
#include "mfhdf.h"
#include <iostream>
#include <sstream>
#include <libdap/debug.h>
#include <BESInternalError.h>
#include <BESDebug.h>

using namespace std;
using namespace libdap;


bool
HDFDMRArray_SPLL::read ()
{

    BESDEBUG("h4","Coming to HDFDMRArray_SPLL read "<<endl);

    // Declaration of offset,count and step
    vector<int>offset;
    offset.resize(sds_rank);
    vector<int>count;
    count.resize(sds_rank);
    vector<int>step;
    step.resize(sds_rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(offset.data(),step.data(),count.data());

    if (sp_type != 0) 
        throw BESInternalError("Currently we only support TRMM version 7.",__FILE__, __LINE__);
    vector<float> val;
    val.resize(nelms);
    for (int i = 0; i < nelms; ++i)
        val[i] = ll_start+offset[0]*ll_res+ll_res/2 + i*ll_res*step[0];
    set_value((dods_float32*)val.data(),nelms);
    return true;
}

// Standard way to pass the coordinates of the subsetted region from the client to the handlers
// Returns the number of elements 
int
HDFDMRArray_SPLL::format_constraint (int *offset, int *step, int *count)
{
    int nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();
    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);

        // Check for illegal  constraint
        if (start > stop) {
            ostringstream oss;
            oss << "Array/Grid hyperslab start point "<< start <<
                   " is greater than stop point " <<  stop <<".";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1;      // count of elements
        nels *= count[id];              // total number of values for variable

        BESDEBUG ("h4",
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


