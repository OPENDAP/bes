/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF4 SDS values for a direct DMR-mapping DAP4 response.
// Each SDS will be mapped to a DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////


#include "HDFDMRArray_SDS.h"
#include "hdf.h"
#include "mfhdf.h"
#include <iostream>
#include <sstream>
#include <libdap/debug.h>
#include <libdap/InternalErr.h>
#include <BESDebug.h>

using namespace std;
using namespace libdap;


bool
HDFDMRArray_SDS::read ()
{

    BESDEBUG("h4","Coming to HDFDMRArray_SDS read "<<endl);
    if (length() == 0)
        return true; 

    // Declaration of offset,count and step
    vector<int>offset;
    offset.resize(sds_rank);
    vector<int>count;
    count.resize(sds_rank);
    vector<int>step;
    step.resize(sds_rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(offset.data(),step.data(),count.data());
    BESDEBUG("h4","Coming to HDFDMRArray_SDS read "<<endl);

    int32 sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
    if (sdid < 0) {
        ostringstream eherr;
        eherr << "File " << filename.c_str () << " cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    BESDEBUG("h4","Coming to HDFDMRArray_SDS read "<<endl);
    BESDEBUG("h4","After SDstart "<<endl);
    // Obtain the SDS index based on the input sds reference number.
    int32 sdsindex = SDreftoindex (sdid, (int32) sds_ref);
    if (sdsindex == -1) {
        SDend(sdid);
        ostringstream eherr;
        eherr << "SDS index " << sdsindex << " is not right.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Obtain this SDS ID.
    int32 sdsid = SDselect (sdid, sdsindex);
    if (sdsid < 0) {
        SDend(sdid);
        ostringstream eherr;
        eherr << "SDselect failed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    vector<char>buf;
    buf.resize(nelms*sds_typesize);

    int32 r = SDreaddata (sdsid, offset.data(), step.data(), count.data(), buf.data());
    if (r != 0) {
	    SDendaccess (sdsid);
            SDend(sdid);
	    ostringstream eherr;
	    eherr << "SDreaddata failed";
	    throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    val2buf(buf.data());
    set_read_p(true);
    SDendaccess (sdsid);
    SDend(sdid);
    
    return true;
}
// Standard way to pass the coordinates of the subsetted region from the client to the handlers
// Returns the number of elements 
int
HDFDMRArray_SDS::format_constraint (int *offset, int *step, int *count)
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


