/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
// This is to support the HDF-EOS5 grid for the default option. 
// The grid mapping variable is declared here.
/////////////////////////////////////////////////////////////////////////////

#include "HDF5CFProj1D.h"
#include <iostream>
#include <sstream>
#include <libdap/debug.h>

#include <libdap/InternalErr.h>
#include <BESDebug.h>
using namespace std;
using namespace libdap;

bool HDF5CFProj1D::read()
{

    // Declaration of offset,count and step
    vector<int64_t> offset;
    offset.resize(1);
    vector<int64_t> count;
    count.resize(1);
    vector<int64_t> step;
    step.resize(1);

    // Obtain offset,step and count from the client expression constraint
    int64_t nelms = -1;
    nelms = format_constraint(offset.data(), step.data(), count.data());

    vector<double> val;
    val.resize(tnumelm);

    //Based on the HFRHANDLER-303, the number of element represents cells according
    //to the data scientist at LP DAAC.
    // Use meter instead of km. KY 2016-04-22
#if 0
    //double step_v = (evalue - svalue)/((tnumelm-1)*1000);
    //    double newsvalue = svalue/1000;
    //val[0] = svalue/1000;
    //double step_v = (evalue - svalue)/(tnumelm*1000);
#endif
    
    double step_v = (evalue - svalue)/tnumelm;
    val[0] = svalue;
    for(int64_t i = 1;i<tnumelm; i++)
        val[i] = val[i-1] + step_v;

    if (nelms == tnumelm) 
        set_value_ll((dods_float64 *) val.data(), nelms);
    else {
        vector<double>val_subset;
        val_subset.resize(nelms);
        for (int64_t i = 0; i < count[0]; i++)
            val_subset[i] = val[offset[0] + step[0] * i];
        set_value_ll((dods_float64 *) val_subset.data(), nelms);
    }
 
    return false;
}

int64_t
HDF5CFProj1D::format_constraint (int64_t *offset, int64_t *step, int64_t *count)
{
    int64_t nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();

    while (p != dim_end ()) {

        int64_t start = dimension_start_ll (p, true);
        int64_t stride = dimension_stride_ll (p, true);
        int64_t stop = dimension_stop_ll (p, true);

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

        BESDEBUG ("h5",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

        id++;
        p++;
    }// "while (p != dim_end ())"

    return nels;
}


