/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB

#include "HDFEOS2GeoCF1D.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>

#include <BESDebug.h>

using namespace libdap;

bool HDFEOS2GeoCF1D::read()
{
    if (length() == 0)
        return true; 

    // Declaration of offset,count and step
    vector<int> offset;
    offset.resize(1);
    vector<int> count;
    count.resize(1);
    vector<int> step;
    step.resize(1);

    // Obtain offset,step and count from the client expression constraint
    int nelms = -1;
    nelms = format_constraint(offset.data(), step.data(), count.data());

    vector<double> val;
    val.resize(tnumelm);

    //HFRHANDLER-303, the number of element represents cells according
    //to the data scientist at LP DAAC.


    double step_v = (evalue - svalue)/tnumelm;

    // Use meter instead of km. KY 2016-04-22
    val[0] = svalue;
    for(int i = 1;i<tnumelm; i++)
        val[i] = val[i-1] + step_v;

    if (nelms == tnumelm) {
        set_value((dods_float64 *) val.data(), nelms);
    }
    else {
        vector<double>val_subset;
        val_subset.resize(nelms);
        for (int i = 0; i < count[0]; i++)
            val_subset[i] = val[offset[0] + step[0] * i];
        set_value((dods_float64 *) val_subset.data(), nelms);
    }
 
    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2GeoCF1D::format_constraint (int *offset, int *step, int *count)
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


#endif
