// This file is part of the hdf4_handler implementing for the CF-compliant
// Copyright (c) The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

////////////////////////////////////////////////////////////////////////////////
/// \file HDFCFStr.cc
/// \brief The implementation of mapping  HDF4 one dimensional character arry to DAP String for the CF option
///
/// In the future, this may be merged with the default option.
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////



#include <iostream>
#include <sstream>

#include <BESInternalError.h>
#include "HDFCFStr.h"
#include <BESDebug.h>
#include "HDFCFUtil.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;
HDFCFStr::HDFCFStr(const int this_h4fd, int32 sds_field_ref,const string &h4_filename,const string &sds_varname,const string &sds_varnewname, bool is_h4_vdata) 
      : Str(sds_varnewname, h4_filename),
        filename(h4_filename),
        varname(sds_varname),
        h4fd(this_h4fd),
        field_ref(sds_field_ref),
        is_vdata(is_h4_vdata)
{
}

BaseType *HDFCFStr::ptr_duplicate()
{
    return new HDFCFStr(*this);
}

bool HDFCFStr::read()
{

    BESDEBUG("h4","Coming to HDFCFStr read "<<endl);
    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    // SDS
    if(false == is_vdata) {

        int32 sdid = -1;
        if(false == check_pass_fileid_key) {
            sdid = SDstart (filename.c_str (), DFACC_READ);
            if (sdid < 0) {
                string msg = "File " + filename + " cannot be open.";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
        }
        else
            sdid = h4fd;

        int32 sdsid = 0;

        int32 sdsindex = SDreftoindex (sdid, field_ref);
        if (sdsindex == -1) {
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            string msg = "SDS index " + to_string(sdsindex) + " is not right.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        // Obtain this SDS ID.
        sdsid = SDselect (sdid, sdsindex);
        if (sdsid < 0) {
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            string msg = "SDselect failed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        vector<int32> dim_sizes(H4_MAX_VAR_DIMS);
        int32 sds_rank;
        int32 data_type;
        int32 n_attrs;
        vector<char> name(H4_MAX_NC_NAME);

        int32 r = 0;
        r = SDgetinfo (sdsid, name.data(), &sds_rank, dim_sizes.data(),
                           &data_type, &n_attrs);
        if(r == FAIL) {
            SDendaccess(sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            string msg = "SDgetinfo failed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        if(sds_rank != 1) {
            SDendaccess(sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            string msg = "The rank of string doesn't match with the rank of character array.";
            throw BESInternalError(msg,__FILE__,__LINE__);
 
        }

        vector<int32>offset32;
        offset32.resize(1);
        vector<int32>count32;
        count32.resize(1);
        vector<int32>step32;
        step32.resize(1);
        offset32[0] = 0;
        count32[0]  = dim_sizes[0];
        step32[0]   = 1;

        vector<char>val;
        val.resize(count32[0]);

        r = SDreaddata (sdsid, offset32.data(), step32.data(), count32.data(), val.data());
        if (r != 0) {
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            string msg = "SDreaddata failed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        string final_str(val.begin(),val.end());
        set_value(final_str);
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
               string msg = "File " + filename +" cannot be open.";
               throw BESInternalError(msg,__FILE__,__LINE__);
            }
        }

        // Start the Vdata interface
        if (Vstart (file_id) < 0) {
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "Vstart failed. ";   
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        // Attach the vdata
        int32 vdref = field_ref;
        int32 vdata_id = VSattach (file_id, vdref, "r");
        if (vdata_id == -1) {
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "Vdata cannot be attached.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        int32 num_rec = VSelts(vdata_id);
        if (num_rec == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "The number of elements from this vdata cannot be obtained.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        //int32 r = -1; //unused variable. SBL 2/7/20

        // Seek the position of the starting point
        if (VSseek (vdata_id, 0) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "VSseek failed at position 0." ;
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        // Prepare the vdata field
        if (VSsetfields (vdata_id, varname.c_str ()) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "VSsetfields failed with the name " + varname + ".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
        vector <char> val;
        val.resize(num_rec);

        // Read the data
        if(VSread (vdata_id, (uint8 *) val.data(), num_rec,
                    FULL_INTERLACE) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "VSread failed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        string final_str(val.begin(),val.end());
        set_value(final_str);
        if (VSdetach (vdata_id) == -1) {
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "VSdetach failed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        if (Vend (file_id) == -1) {
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            string msg = "VSdetach failed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

         HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
    }
    return true;
}
