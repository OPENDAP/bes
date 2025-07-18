/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the HDF4 DFNT_CHAR >1D array and then send to DAP as a DAP string array. 
// This file is used when the CF option of the handler is turned on.
//  Authors:   Kent Yang <myang6@hdfgroup.org>  
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

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
#include "HDFCFStrField.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;


bool
HDFCFStrField::read ()
{

    BESDEBUG("h4","Coming to HDFCFStrField read "<<endl);
    if(length() == 0)
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


    // Initialize the temp. returned value.
    int32 r = 0;

    // First SDS
    if(false == is_vdata) {

        int32 sdid = -1;
        if(false == check_pass_fileid_key) {
            sdid = SDstart (filename.c_str (), DFACC_READ);
            if (sdid < 0) {
                string msg =  "File " + filename + " cannot be open.";
                throw BESInternalError(msg, __FILE__, __LINE__);
            }
        }
        else
            sdid = h4fd;

        int32 sdsid = 0;

        int32 sdsindex = SDreftoindex (sdid, fieldref);
        if (sdsindex == -1) {
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            string msg = "SDS index " + to_string(sdsindex) + " is not right.";
            throw BESInternalError(msg, __FILE__, __LINE__);
        }

        // Obtain this SDS ID.
        sdsid = SDselect (sdid, sdsindex);
        if (sdsid < 0) {
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError ("SDselect failed.",__FILE__, __LINE__);
        }

        int32 dim_sizes[H4_MAX_VAR_DIMS];
        int32 sds_rank;
        int32  data_type;
        int32  n_attrs;
        char  name[H4_MAX_NC_NAME];

        r = SDgetinfo (sdsid, name, &sds_rank, dim_sizes,
                           &data_type, &n_attrs);
        if(r == FAIL) {
            SDendaccess(sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError ("SDgetinfo failed.",__FILE__, __LINE__);
        }

        if(sds_rank != (rank+1)) {
            SDendaccess(sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            string msg = "The rank of string doesn't match with the rank of character array";
            throw BESInternalError (msg, __FILE__, __LINE__);
 
        }
        offset32[rank] = 0;
        count32[rank] = dim_sizes[rank];
        step32[rank] = 1;
        int32 last_dim_size = dim_sizes[rank];

        vector<char>val;
        val.resize(nelms*count32[rank]);

        r = SDreaddata (sdsid, offset32.data(), step32.data(), count32.data(), val.data());
        if (r != 0) {
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw BESInternalError ("SDreaddata failed.",__FILE__, __LINE__);
        }

        vector<string>final_val;
        final_val.resize(nelms);
        vector<char> temp_buf;
        temp_buf.resize(last_dim_size+1);
         
        // Since the number of the dimension for a string is reduced by 1,
        // the value of each string is the subset of the whole last dimension
        // of the original array.
        for (int i = 0; i<nelms;i++) { 
            strncpy(temp_buf.data(),val.data()+last_dim_size*i,last_dim_size);
            temp_buf[last_dim_size]='\0';
            final_val[i] = temp_buf.data();
        }
        set_value(final_val.data(),nelms);
        SDendaccess(sdsid);
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);

    }
    else {

         int32 file_id = -1;

         if(true == check_pass_fileid_key)
            file_id = h4fd;
         else {
            // Open the file
            file_id = Hopen (filename.c_str (), DFACC_READ, 0);
            if (file_id < 0) {
               string msg = "File " + filename + " cannot be open.";
               throw BESInternalError (msg,__FILE__, __LINE__);
            }
         }

        // Start the Vdata interface
        if (Vstart (file_id) < 0) {
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            throw BESInternalError ("Vstart failed.", __FILE__, __LINE__);
        }

        // Attach the vdata
        int32 vdata_id = VSattach (file_id, fieldref, "r");
        if (vdata_id == -1) {
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            throw BESInternalError ("Vdata cannot be attached.",__FILE__, __LINE__);
        }


        // Seek the position of the starting point
        if (VSseek (vdata_id, offset32[0]) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "VSseek failed at " +to_string(offset32[0]) + ".";
            throw BESInternalError (msg, __FILE__, __LINE__);
        }

        // Prepare the vdata field
        if (VSsetfields (vdata_id, fieldname.c_str ()) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "VSsetfields failed with the name " + fieldname +".";
            throw BESInternalError (msg,__FILE__, __LINE__);
        }

        int32 vdfelms = fieldorder * count32[0] * step32[0];

        vector<char> val;
        val.resize(vdfelms);

         // Read the data
        r = VSread (vdata_id, (uint8 *) val.data(), 1+(count32[0] -1)* step32[0],
                    FULL_INTERLACE);

        if (r == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            throw BESInternalError ("VSread failed. ",__FILE__, __LINE__);
        }

        vector<string>final_val;
        final_val.resize(nelms);

        vector<char> temp_buf;
        temp_buf.resize(fieldorder+1);
        for (int i = 0; i<nelms;i++) {
            strncpy(temp_buf.data(),val.data()+fieldorder*i*step32[0],fieldorder);
            temp_buf[fieldorder]='\0';
            final_val[i] = temp_buf.data();
        }
        set_value(final_val.data(),nelms);
        VSdetach(vdata_id);
        Vend(file_id);
        HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);

    }

    return false;
}

int
HDFCFStrField::format_constraint (int *offset, int *step, int *count)
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
    }// while 

    return nels;
}


#if 0
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

