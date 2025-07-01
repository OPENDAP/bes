// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file HDF5CFStr.cc
/// \brief The implementation of mapping  HDF5 String to DAP String for the CF option
///
/// In the future, this may be merged with the default option.
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#include "config_hdf5.h"

#include <iostream>
#include <sstream>

#include <BESDebug.h>

#include <libdap/InternalErr.h>
#include "HDF5RequestHandler.h"
#include "h5cfdaputil.h"
#include "HDF5CFStr.h"
#include <hdf5.h>

using namespace std;
using namespace libdap;

HDF5CFStr::HDF5CFStr(const string &n, const string &d,const string &h5_varname) 
      : Str(n, d), varname(h5_varname)
{
}

BaseType *HDF5CFStr::ptr_duplicate()
{
    auto HDF5CFStr_unique = make_unique<HDF5CFStr>(*this);
    return HDF5CFStr_unique.release();
}

bool HDF5CFStr::read()
{

    BESDEBUG("h5","Coming to HDF5CFStr read "<<endl);
    hid_t fileid = -1;
    hid_t dsetid = -1;
    hid_t dspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;

    if ((fileid = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {
        string msg = "HDF5 File " + dataset() + " cannot be opened. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if ((dsetid = H5Dopen(fileid,varname.c_str(),H5P_DEFAULT))<0) {
        H5Fclose(fileid);
        string msg = "HDF5 dataset " + varname  + " cannot be opened. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if ((dspace = H5Dget_space(dsetid))<0) {
        H5Dclose(dsetid);
        H5Fclose(fileid);
        string msg = "Space id of the HDF5 dataset " + varname + " cannot be obtained. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (H5S_SCALAR != H5Sget_simple_extent_type(dspace)) {
        H5Dclose(dsetid);
        H5Fclose(fileid);
        string msg = " The HDF5 dataset " + varname + " is not scalar.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
        H5Sclose(dspace);
        H5Dclose(dsetid);
        H5Fclose(fileid);
        string msg = " Obtaining the datatype of the HDF5 dataset " + varname +" fails.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        H5Fclose(fileid);
        string msg = "Obtaining the memory type of the HDF5 dataset " + varname + " fails.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    htri_t is_vlen_str = H5Tis_variable_str(dtypeid);
    if (is_vlen_str > 0) {
        size_t ty_size = H5Tget_size(memtype);
        if (ty_size == 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            string msg = "Cannot obtain the size of the fixed size HDF5 string of the dataset" + varname + " fails.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
        vector <char> strval;
        strval.resize(ty_size);
        hid_t read_ret = -1;
        read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)strval.data());

        if (read_ret < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            string msg = "Cannot read the HDF5 dataset " + varname + " with the type of the HDF5 variable length string.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        char*temp_bp = strval.data();
        char*onestring = nullptr;
        string final_str ="";
               
        onestring = *(char**)temp_bp;
        if(onestring!=nullptr ) 
            final_str =string(onestring);
                 
        else // We will add a nullptr is onestring is nullptr.
            final_str="";

        if (""!=final_str) {
            herr_t ret_vlen_claim = 0;
            ret_vlen_claim = H5Dvlen_reclaim(memtype,dspace,H5P_DEFAULT,(void*)strval.data());
            if (ret_vlen_claim < 0){
                H5Tclose(memtype);
                H5Tclose(dtypeid);
                H5Sclose(dspace);
                H5Dclose(dsetid);
                H5Fclose(fileid);
                string msg = "Cannot reclaim the memory buffer of the HDF5 variable length string of the dataset ";
                msg = msg + varname + ".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
        }

        // If the string size is longer than the current netCDF JAVA
        // string and the "EnableDropLongString" key is turned on,
        // No string is generated.
        if (true == HDF5RequestHandler::get_drop_long_string()) {
            if( final_str.size() > NC_JAVA_STR_SIZE_LIMIT) 
                final_str = "";
        }
        set_value(final_str);
    }     

    else if (0 == is_vlen_str) {
        size_t ty_size = H5Tget_size(dtypeid);
        if (ty_size == 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            string msg = "Cannot obtain the size of the fixed size HDF5 string of the dataset ";
            msg = msg + varname + ".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        vector <char> strval;
        strval.resize(1+ty_size);
        hid_t read_ret = -1;
        read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)strval.data());
        if (read_ret < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            H5Dclose(dsetid);
            H5Fclose(fileid);
            string msg = "Cannot read the HDF5 dataset ";
            msg = msg + " with the type of the fixed size HDF5 string.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        string total_string(strval.begin(),strval.end());
        strval.clear();//release some memory

        // Need to trim the null parameters 
        size_t temp_pos;
        if (H5Tget_strpad(dtypeid) == H5T_STR_NULLTERM)
            temp_pos = total_string.find_first_of('\0');
        else if (H5Tget_strpad(dtypeid) == H5T_STR_SPACEPAD)
            temp_pos = total_string.find_last_not_of(' ')+1;
        else 
            temp_pos = total_string.find_last_not_of('0')+1;

        string trim_string = total_string.substr(0,temp_pos);

        // If the string size is longer than the current netCDF JAVA
        // string and the "EnableDropLongString" key is turned on,
        // No string is generated.
        if (true == HDF5RequestHandler::get_drop_long_string()) {
            if( trim_string.size() > NC_JAVA_STR_SIZE_LIMIT) 
                trim_string = "";
        }
        set_value(trim_string);
    }
    else {
        H5Tclose(memtype);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        H5Fclose(fileid);
        string msg = "H5Tis_variable_str returns negative value.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    } 

    H5Tclose(memtype);
    H5Tclose(dtypeid);
    H5Sclose(dspace);
    H5Dclose(dsetid);
    H5Fclose(fileid);


    return true;
}
