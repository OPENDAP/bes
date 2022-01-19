/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2017 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDF5CFGeoCF1D.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>

#include <libdap/InternalErr.h>
#include <BESDebug.h>
using namespace std;
using namespace libdap;

// This now only applies to Sinusoidal projection. I need to handle LAMAZ and PS.
bool HDF5CFGeoCF1D::read()
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


void HDF5CFGeoCF1D::read_data_NOT_from_mem_cache(bool /*add_cache*/,void */*buf*/){
    //Not implement yet/
    return;


}
