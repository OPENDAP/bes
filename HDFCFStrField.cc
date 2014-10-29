/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the HDF4 DFNT_CHAR >1D array and then send to DAP as a DAP string array for the CF option.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "InternalErr.h"
#include <BESDebug.h>
#include <BESLog.h>

#include "HDFCFUtil.h"
#include "HDFCFStrField.h"

using namespace std;


bool
HDFCFStrField::read ()
{

    BESDEBUG("h4","Coming to HDFCFStrField read "<<endl);

    string check_pass_fileid_key_str="H4.EnablePassFileID";
    bool check_pass_fileid_key = false;
    check_pass_fileid_key = HDFCFUtil::check_beskeys(check_pass_fileid_key_str);

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
        nelms = format_constraint (&offset[0], &step[0], &count[0]);

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
            sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
            if (sdid < 0) {
                ostringstream eherr;
                eherr << "File " << filename.c_str () << " cannot be open.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
        }
        else
            sdid = h4fd;

        int32 sdsid = 0;

        int32 sdsindex = SDreftoindex (sdid, fieldref);
        if (sdsindex == -1) {
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "SDS index " << sdsindex << " is not right.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        // Obtain this SDS ID.
        sdsid = SDselect (sdid, sdsindex);
        if (sdsid < 0) {
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "SDselect failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        int32 dim_sizes[H4_MAX_VAR_DIMS];
        int32 sds_rank, data_type, n_attrs;
        char  name[H4_MAX_NC_NAME];

        r = SDgetinfo (sdsid, name, &sds_rank, dim_sizes,
                           &data_type, &n_attrs);
        if(r == FAIL) {
            SDendaccess(sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "SDgetinfo failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        if(sds_rank != (rank+1)) {
            SDendaccess(sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "The rank of string doesn't match with the rank of character array";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
 
        }
        offset32[rank] = 0;
        count32[rank] = dim_sizes[rank];
        step32[rank] = 1;
        int32 last_dim_size = dim_sizes[rank];

        vector<char>val;
        val.resize(nelms*count32[rank]);

        r = SDreaddata (sdsid, &offset32[0], &step32[0], &count32[0], &val[0]);
        if (r != 0) {
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "SDreaddata failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        vector<string>final_val;
        final_val.resize(nelms);
        vector<char> temp_buf;
        temp_buf.resize(last_dim_size+1);
         
        // Since the number of the dimension for a string is reduced by 1,
        // the value of each string is the subset of the whole last dimension
        // of the original array.
        for (int i = 0; i<nelms;i++) { 
            strncpy(&temp_buf[0],&val[0]+last_dim_size*i,last_dim_size);
            temp_buf[last_dim_size]='\0';
            final_val[i] = &temp_buf[0];
        }
        set_value(&final_val[0],nelms);
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
               ostringstream eherr;
               eherr << "File " << filename.c_str () << " cannot be open.";
               throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
         }

        // Start the Vdata interface
        if (Vstart (file_id) < 0) {
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "This file cannot be open.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        // Attach the vdata
        int32 vdata_id = VSattach (file_id, fieldref, "r");
        if (vdata_id == -1) {
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "Vdata cannot be attached.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        int32 r = -1;

        // Seek the position of the starting point
        if (VSseek (vdata_id, (int32) offset32[0]) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "VSseek failed at " << offset32[0];
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        // Prepare the vdata field
        if (VSsetfields (vdata_id, fieldname.c_str ()) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "VSsetfields failed with the name " << fieldname;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        int32 vdfelms = fieldorder * count32[0] * step32[0];

        vector<char> val;
        val.resize(vdfelms);

         // Read the data
        r = VSread (vdata_id, (uint8 *) &val[0], 1+(count32[0] -1)* step32[0],
                    FULL_INTERLACE);

        if (r == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "VSread failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        vector<string>final_val;
        final_val.resize(nelms);

        vector<char> temp_buf;
        temp_buf.resize(fieldorder+1);
        for (int i = 0; i<nelms;i++) {
            strncpy(&temp_buf[0],&val[0]+fieldorder*i*step32[0],fieldorder);
            temp_buf[fieldorder]='\0';
            final_val[i] = &temp_buf[0];
        }
        set_value(&final_val[0],nelms);
        VSdetach(vdata_id);
        Vend(file_id);
        HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);

    }

    return false;
}

int
HDFCFStrField::format_constraint (int *offset, int *step, int *count)
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


