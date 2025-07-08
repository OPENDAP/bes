/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves HDF-EOS2 swath/grid  one dimensional character(DFNT_CHAR) array to DAP String for the CF option.
//  Authors:   Kent Yang <myang6@hdfgroup.org>  Eunsoo Seo
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#include "config.h"
#include "config_hdf.h"

#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESLog.h>

#include "HDFCFUtil.h"
#include "HDFEOS2CFStrField.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;

bool
HDFEOS2CFStrField::read ()
{

    BESDEBUG("h4","Coming to HDFEOS2CFStrField read "<<endl);

    if (length() == 0)
        return true; 

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();
    // Note that one dimensional character array is one string,
    // so the rank for character arrays should be rank from string+1 
    // offset32,step32 and count32 will be new subsetting parameters for
    // character arrays.
    vector<int32>offset32;
    offset32.resize(rank+1);
    vector<int32>count32;
    count32.resize(rank+1);
    vector<int32>step32;
    step32.resize(rank+1);
    int nelms = 1;

    if (rank != 0) {

        // Declare offset, count and step,
        vector<int>offset;
        offset.resize(rank);
        vector<int>count;
        count.resize(rank);
        vector<int>step;
        step.resize(rank);

        // Declare offset, count and step,
        // Note that one dimensional character array is one string,
        // so the rank for character arrays should be rank from string+1 
        // Obtain offset,step and count from the client expression constraint
        nelms = format_constraint (offset.data(), step.data(), count.data());

        // Assign the offset32,count32 and step32 up to the dimension rank-1.
        // Will assign the dimension rank later.
        for (int i = 0; i < rank; i++) {
            offset32[i] = (int32) offset[i];
            count32[i] = (int32) count[i];
            step32[i] = (int32) step[i];
        }
    }

    int32 (*openfunc) (char *, intn);
    intn (*closefunc) (int32);
    int32 (*attachfunc) (int32, char *);
    intn (*detachfunc) (int32);
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
    intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);


    // Define function pointers to handle the swath
    if (grid_or_swath == 0) {
        openfunc = GDopen;
        closefunc = GDclose;
        attachfunc = GDattach;
        detachfunc = GDdetach;
        fieldinfofunc = GDfieldinfo;
        readfieldfunc = GDreadfield;
 
    }
    else {
        openfunc = SWopen;
        closefunc = SWclose;
        attachfunc = SWattach;
        detachfunc = SWdetach;
        fieldinfofunc = SWfieldinfo;
        readfieldfunc = SWreadfield;
    }

    int32 gfid = -1;
    if (false == check_pass_fileid_key) {

        // Obtain the EOS object ID(either grid or swath)
        gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (gfid < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

    }
    else
        gfid = gsfd;

    int32 gsid = attachfunc (gfid, const_cast < char *>(objname.c_str ()));
    if (gsid < 0) {
        if (false == check_pass_fileid_key)
            closefunc(gfid);
        string msg = "Grid/Swath " + objname + " cannot be attached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // Initialize the temp. returned value.
    intn r = 0;
    int32 tmp_rank = 0;
    char tmp_dimlist[1024];
    int32 tmp_dims[rank+1];
    int32 field_dtype = 0;

    r = fieldinfofunc (gsid, const_cast < char *>(varname.c_str ()),
                &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
    if (r != 0) {
        detachfunc(gsid);
        if (false == check_pass_fileid_key)
            closefunc(gfid);
        string msg = "Field " + varname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    offset32[rank] = 0;
    count32[rank] = tmp_dims[rank];
    step32[rank] = 1;
    int32 last_dim_size = tmp_dims[rank];

    vector<char>val;
    val.resize(nelms*count32[rank]);

    r = readfieldfunc(gsid,const_cast<char*>(varname.c_str()),
                       offset32.data(), step32.data(), count32.data(), val.data());

    if (r != 0) {
        detachfunc(gsid);
        if (false == check_pass_fileid_key)
            closefunc(gfid);
        string msg = "swath or grid readdata failed.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    vector<string>final_val;
    final_val.resize(nelms);
    vector<char> temp_buf;
    temp_buf.resize(last_dim_size+1);
         
    // The array values of the last dimension should be saved as the
    // string.
    for (int i = 0; i<nelms;i++) { 
        strncpy(temp_buf.data(),val.data()+last_dim_size*i,last_dim_size);
        temp_buf[last_dim_size]='\0';
        final_val[i] = temp_buf.data();
    } 
    set_value(final_val.data(),nelms);

    detachfunc(gsid);
    if (false == check_pass_fileid_key)
        closefunc(gfid);

    return false;
}

int
HDFEOS2CFStrField::format_constraint (int *offset, int *step, int *count)
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
