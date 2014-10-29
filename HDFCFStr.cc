// This file is part of the hdf4_handler implementing for the CF-compliant
// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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

////////////////////////////////////////////////////////////////////////////////
/// \file HDFCFStr.cc
/// \brief The implementation of mapping  HDF4 one dimensional character arry to DAP String for the CF option
///
/// In the future, this may be merged with the default option.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////



#include <iostream>
#include <sstream>

#include "InternalErr.h"
#include "HDFCFStr.h"
#include <BESDebug.h>
#include "HDFCFUtil.h"


HDFCFStr::HDFCFStr(const int h4fd, int32 field_ref,const string &filename,const string &varname,const string &varnewname, bool is_vdata) 
      : Str(varnewname, filename),
        filename(filename),
        varname(varname),
        h4fd(h4fd),
        field_ref(field_ref),
        is_vdata(is_vdata)
{
}

HDFCFStr::~HDFCFStr()
{
}
BaseType *HDFCFStr::ptr_duplicate()
{
    return new HDFCFStr(*this);
}

bool HDFCFStr::read()
{

    BESDEBUG("h4","Coming to HDFCFStr read "<<endl);
    string check_pass_fileid_key_str="H4.EnablePassFileID";
    bool check_pass_fileid_key = false;
    check_pass_fileid_key = HDFCFUtil::check_beskeys(check_pass_fileid_key_str);

    // SDS
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

        //int32 sdid = h4fd;
        int32 sdsid = 0;

        int32 sdsindex = SDreftoindex (sdid, field_ref);
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

        int32 r = 0;
        r = SDgetinfo (sdsid, name, &sds_rank, dim_sizes,
                           &data_type, &n_attrs);
        if(r == FAIL) {
            SDendaccess(sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "SDgetinfo failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        if(sds_rank != 1) {
            SDendaccess(sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "The rank of string doesn't match with the rank of character array";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
 
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

        r = SDreaddata (sdsid, &offset32[0], &step32[0], &count32[0], &val[0]);
        if (r != 0) {
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "SDreaddata failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
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
        int32 vdref = field_ref;
        int32 vdata_id = VSattach (file_id, vdref, "r");
        if (vdata_id == -1) {
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "Vdata cannot be attached.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        int32 num_rec = VSelts(vdata_id);
        if (num_rec == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
 
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "The number of elements from this vdata cannot be obtained.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }


        int32 r = -1;

        // Seek the position of the starting point
        if (VSseek (vdata_id, 0) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "VSseek failed at " << 0;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        // Prepare the vdata field
        if (VSsetfields (vdata_id, varname.c_str ()) == -1) {
            VSdetach (vdata_id);
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "VSsetfields failed with the name " << varname;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
        vector <char> val;
        val.resize(num_rec);

        // Read the data
        r = VSread (vdata_id, (uint8 *) &val[0], num_rec,
                    FULL_INTERLACE);


        string final_str(val.begin(),val.end());
        set_value(final_str);
        if (VSdetach (vdata_id) == -1) {
            Vend (file_id);
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "VSdetach failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        if (Vend (file_id) == -1) {
            HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
            ostringstream eherr;
            eherr << "VSdetach failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

         HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
//#endif
    }
    return true;
}
