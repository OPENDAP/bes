/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2009 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB

#include "HDFEOS2GeoCF1D.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>

#include <InternalErr.h>
#include <BESDebug.h>


bool HDFEOS2GeoCF1D::read()
{

    // Declaration of offset,count and step
    vector<int> offset;
    offset.resize(1);
    vector<int> count;
    count.resize(1);
    vector<int> step;
    step.resize(1);

    // Obtain offset,step and count from the client expression constraint
    int nelms = -1;
    nelms = format_constraint(&offset[0], &step[0], &count[0]);

    vector<double> val;
    val.resize(tnumelm);

    double step_v = (evalue - svalue)/((tnumelm-1)*1000);
//    double newsvalue = svalue/1000;
    val[0] = svalue/1000;
    for(int i = 1;i<tnumelm; i++)
        val[i] = val[i-1] + step_v;

    if (nelms == tnumelm) {
        set_value((dods_float64 *) &val[0], nelms);
    }
    else {
        vector<double>val_subset;
        val_subset.resize(nelms);
        for (int i = 0; i < count[0]; i++)
            val_subset[i] = val[offset[0] + step[0] * i];
        set_value((dods_float64 *) &val_subset[0], nelms);
    }
 
    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2GeoCF1D::format_constraint (int *offset, int *step, int *count)
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
        count[id] = ((stop - start) / stride) + 1;// count of elements
        nels *= count[id];// total number of values for variable

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
#endif
