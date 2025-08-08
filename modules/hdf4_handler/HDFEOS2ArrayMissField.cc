/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the missing "third-dimension" values of the HDF-EOS2 Grid.
// Some third-dimension coordinate variable values are not provided.
// What we do here is to provide natural number series(1,2,3, ...) for
// these missing values. It doesn't make sense to visualize or analyze
// with vertical cross-section. One can check the data level by level.
//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB

#include "HDFEOS2ArrayMissField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>

#include <BESInternalError.h>
#include <BESDebug.h>

#include "HDFEOS2.h"

using namespace libdap;
using namespace std;
// Now we use the vector to replace new []. KY 2012-12-30
bool HDFEOS2ArrayMissGeoField::read()
{

    if (length() == 0)
        return true;
    // Declaration of offset,count and step
    vector<int> offset;
    offset.resize(rank);
    vector<int> count;
    count.resize(rank);
    vector<int> step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = -1;
    nelms = format_constraint(offset.data(), step.data(), count.data());

    vector<int> val;
    val.resize(nelms);

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        for (int i = 0; i < nelms; i++)
            val[i] = i;
        set_value((dods_int32 *) val.data(), nelms);
    }
    else {
        if (rank != 1) {
            throw BESInternalError("Currently the rank of the missing field should be 1.",__FILE__, __LINE__);
        }
        for (int i = 0; i < count[0]; i++)
            val[i] = offset[0] + step[0] * i;
        set_value((dods_int32 *) val.data(), nelms);
    }
 
    return false;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFEOS2ArrayMissGeoField::format_constraint (int *offset, int *step, int *count)
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
