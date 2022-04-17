// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFSpecialCVArray.cc
/// \brief Implementation of the retrieval of special coordinate variable values(mostly from product specification) for general HDF5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#include "config_hdf5.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <BESDebug.h>
#include <libdap/InternalErr.h>

#include "HDF5GMCFSpecialCVArray.h"

using namespace std;
using namespace libdap;

BaseType *HDF5GMCFSpecialCVArray::ptr_duplicate()
{
    return new HDF5GMCFSpecialCVArray(*this);
}

bool HDF5GMCFSpecialCVArray::read()
{

    BESDEBUG("h5", "Coming to HDF5GMCFSpecialCVArray read "<<endl);

    read_data_NOT_from_mem_cache(false, nullptr);

    return true;
}

// This is according to https://storm.pps.eosdis.nasa.gov/storm/filespec.GPM.V1.pdf(section 5.32), the definition of nlayer
// The top of each layer is 0.5,1.0,....., 10.0,11.0.....18.0 km.
void HDF5GMCFSpecialCVArray::obtain_gpm_l3_layer(int nelms, vector<int>&offset, vector<int>&step, vector<int>&/*count*/)
{

    vector<float> total_val;
    total_val.resize(tnumelm);
    for (int i = 0; i < 20; i++)
        total_val[i] = 0.5 * (i + 1);

    for (int i = 20; i < 28; i++)
        total_val[i] = total_val[19] + (i - 19);

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        set_value((dods_float32 *) &total_val[0], nelms);
    }
    else {

        vector<float> val;
        val.resize(nelms);

        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value((dods_float32 *) &val[0], nelms);
    }
}

// This is according to 
// http://www.eorc.jaxa.jp/GPM/doc/product/format/en/03.%20GPM_DPR_L2_L3%20Product%20Format%20Documentation_E.pdf
// section 8.1. Number of layers at the fixed heights of 0.0-0.5km,0.5-1.0 km,.....
// Like obtain_gpm_l3_layer1, we use the top height value 0.5 km, 1.0 km,2km,.....,18 km.
// See also section 4.1.1 and 3.1.1 of http://www.eorc.jaxa.jp/GPM/doc/product/format/en/06.%20GPM_Combined%20Product%20Format_E.pdf
void HDF5GMCFSpecialCVArray::obtain_gpm_l3_layer2(int nelms, vector<int>&offset, vector<int>&step, vector<int>&/*count*/)
{

    vector<float> total_val;
    total_val.resize(tnumelm);
    for (int i = 0; i < 2; i++)
        total_val[i] = 0.5 * (i + 1);

    for (int i = 2; i < 19; i++)
        total_val[i] = total_val[1] + (i - 1);

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        set_value((dods_float32 *) &total_val[0], nelms);
    }
    else {

        vector<float> val;
        val.resize(nelms);

        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value((dods_float32 *) &val[0], nelms);
    }
}

void HDF5GMCFSpecialCVArray::obtain_gpm_l3_hgt(int nelms, vector<int>&offset, vector<int>&step, vector<int>&/*count*/)
{

    vector<float> total_val;
    total_val.resize(5);
    total_val[0] = 2;
    total_val[1] = 4;
    total_val[2] = 6;
    total_val[3] = 10;
    total_val[4] = 15;

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        set_value((dods_float32 *) &total_val[0], nelms);
    }
    else {

        vector<float> val;
        val.resize(nelms);

        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value((dods_float32 *) &val[0], nelms);
    }
}

void HDF5GMCFSpecialCVArray::obtain_gpm_l3_nalt(int nelms, vector<int>&offset, vector<int>&step, vector<int>&/*count*/)
{
    vector<float> total_val;
    total_val.resize(5);

    total_val[0] = 2;
    total_val[1] = 4;
    total_val[2] = 6;
    total_val[3] = 10;
    total_val[4] = 15;

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        set_value((dods_float32 *) &total_val[0], nelms);
    }
    else {

        vector<float> val;
        val.resize(nelms);

        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value((dods_float32 *) &val[0], nelms);
    }
}

void HDF5GMCFSpecialCVArray::read_data_NOT_from_mem_cache(bool /*add_cache*/, void*/*buf*/)
{

    BESDEBUG("h5", "Coming to HDF5GMCFSpecialCVArray: read_data_NOT_from_mem_cache "<<endl);
    // Here we still use vector just in case we need to tackle "rank>1" in the future.
    // Also we would like to keep it consistent with other similar handlings.

    vector<int> offset;
    vector<int> count;
    vector<int> step;

    int rank = 1;
    offset.resize(rank);
    count.resize(rank);
    step.resize(rank);

    int nelms = format_constraint(&offset[0], &step[0], &count[0]);

    if (GPMS_L3 == product_type || GPMM_L3 == product_type || GPM_L3_New == product_type) {
        if (varname == "nlayer" && 28 == tnumelm)
            obtain_gpm_l3_layer(nelms, offset, step, count);
        else if (varname == "nlayer" && 19 == tnumelm)
            obtain_gpm_l3_layer2(nelms, offset, step, count);
        else if (varname == "hgt" && 5 == tnumelm) {
            obtain_gpm_l3_hgt(nelms, offset, step, count);
        }
        else if (varname == "nalt" && 5 == tnumelm) obtain_gpm_l3_nalt(nelms, offset, step, count);
    }

    return;
}

