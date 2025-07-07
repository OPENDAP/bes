/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the missing fields for some special NASA HDF4 data products.
// The products include TRMML2_V6,TRMML3B_V6,CER_AVG,CER_ES4,CER_CDAY,CER_CGEO,CER_SRB,CER_SYN,CER_ZAVG,OBPGL2,OBPGL3
// To know more information about these products,check HDFSP.h.
// Some third-dimension coordinate variable values are not provided.
// What we do here is to provide natural number series(1,2,3, ...) for
// these missing values. It doesn't make sense to visualize or analyze
// with vertical cross-section. One can check the data level by level.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFSPArrayAddCVField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include "mfhdf.h"
#include "hdf.h"
#include <libdap/InternalErr.h>
#include <BESInternalError.h>
#include <BESDebug.h>

using namespace libdap;
using namespace std;

bool
HDFSPArrayAddCVField::read ()
{

    BESDEBUG("h4","Coming to HDFSPArrayAddCVField read "<<endl);

    if (length() == 0)
        return true; 

    // Declaration of offset,count and step
    vector<int>offset;
    offset.resize(1);
    vector<int>count;
    count.resize(1);
    vector<int>step;
    step.resize(1);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(offset.data(),step.data(),count.data());

    if (sptype == TRMML3C_V6) {

        if (dtype != DFNT_FLOAT32) 
            throw BESInternalError("The Height datatype of TRMM CSH product should be float.",__FILE__,__LINE__);

        if (tnumelm != 19) 
            throw BESInternalError("The number of elements should be 19.", __FILE__, __LINE__);

        vector<float>total_val;
        total_val.resize(tnumelm);
        total_val[0] = 0.5;
        for (int i = 1; i<tnumelm;i++)
            total_val[i] = (float)i;

        if (nelms == tnumelm) {
            set_value ((dods_float32 *) total_val.data(), nelms);
        }
        else {
    
            vector<float>val;
            val.resize(nelms);
            
            for (int i = 0; i < nelms; i++)
                val[i] = total_val[offset[0] + step[0] * i];
            set_value ((dods_float32 *) val.data(), nelms);
        }
    }

    if (sptype == TRMML3S_V7) {

        
        if (dtype != DFNT_FLOAT32) 
            throw BESInternalError("The Height datatype of TRMM CSH product should be float.", __FILE__, __LINE__);

        if (tnumelm == 28)
            Obtain_trmm_v7_layer(nelms,offset,step);
        else if (tnumelm == 6)
            Obtain_trmml3s_v7_nthrash(nelms,offset,step);
        else {
            throw BESInternalError("This special coordinate variable is not supported.", __FILE__, __LINE__);
        }

    }

    if (sptype == TRMML2_V7) {

        if (dtype != DFNT_FLOAT32) {
            throw BESInternalError("The Height datatype of TRMM CSH product should be float.", __FILE__, __LINE__);
        }

        if (tnumelm == 28 && name =="nlayer")
            Obtain_trmm_v7_layer(nelms,offset,step);
        else {
            throw BESInternalError("This special coordinate variable is not supported.", __FILE__, __LINE__);
        }

    }
        
    return true;
}

void HDFSPArrayAddCVField:: Obtain_trmm_v7_layer(int nelms, vector<int>&offset,vector<int>&step) {


        vector<float>total_val;
        total_val.resize(tnumelm);
        for (int i = 0; i<20;i++)
            total_val[i] = (float)(0.5*(i+1));

        for (int i = 20; i<28;i++)
            total_val[i] = total_val[19]+(i-19);


    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) 
        set_value ((dods_float32 *) total_val.data(), nelms);
    else {

        vector<float>val;
        val.resize(nelms);
        
        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value ((dods_float32 *) val.data(), nelms);
    }
}


void HDFSPArrayAddCVField:: Obtain_trmml3s_v7_nthrash(int nelms, vector<int>&offset,vector<int>&step) {

    vector<float>total_val;
    total_val.resize(tnumelm);

    if (name =="nthrshZO") {
        
        total_val[0] = 0.1f;
        total_val[1] = 0.2f;
        total_val[2] = 0.3f;
        total_val[3] = 0.5f;
        total_val[4] = 0.75f;
        total_val[5] = 50.0f;
    }

    else if (name == "nthrshHB") {

        total_val[0] = 0.1f;
        total_val[1] = 0.2f;
        total_val[2] = 0.3f;
        total_val[3] = 0.5f;
        total_val[4] = 0.75f;
        total_val[5] = 0.9999f;
    }

    else if (name =="nthrshSRT") {

        total_val[0] = 1.5f;
        total_val[1] = 1.0f;
        total_val[2] = 0.8f;
        total_val[3] = 0.6f;
        total_val[4] = 0.4f;
        total_val[5] = 0.1f;
 
    }
    else 
        throw BESInternalError("Unsupported coordinate variable names.",__FILE__, __LINE__);

    // Since we always assign the the missing Z dimension as 32-bit
    // integer, so no need to check the type. The missing Z-dim is always
    // 1-D with natural number 1,2,3,....
    if (nelms == tnumelm) {
        set_value ((dods_float32 *) total_val.data(), nelms);
    }
    else {

        vector<float>val;
        val.resize(nelms);
        
        for (int i = 0; i < nelms; i++)
            val[i] = total_val[offset[0] + step[0] * i];
        set_value ((dods_float32 *) val.data(), nelms);
    }
}



// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFSPArrayAddCVField::format_constraint (int *offset, int *step, int *count)
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


