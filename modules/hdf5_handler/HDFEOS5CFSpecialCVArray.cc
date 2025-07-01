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

/////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS5CFSpecialCVArray.cc
/// \brief This class specifies the retrieval of special coordinate variable  values for HDF-EOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2023 The HDF Group
///
/// All rights reserved.

#include <iostream>
#include <memory>
#include <cassert>
#include <BESDebug.h>
#include <libdap/InternalErr.h>

#include "HDF5RequestHandler.h"
#include "HDFEOS5CFSpecialCVArray.h"

using namespace std;
using namespace libdap;

BaseType *HDFEOS5CFSpecialCVArray::ptr_duplicate()
{
    auto HDFEOS5CFSpecialCVArray_unique = make_unique<HDFEOS5CFSpecialCVArray>(*this);
    return HDFEOS5CFSpecialCVArray_unique.release();
}

bool HDFEOS5CFSpecialCVArray::read(){

    BESDEBUG("h5","Coming to HDFEOS5CFSpecialCVArray read "<<endl);

    read_data_NOT_from_mem_cache(false,nullptr);

    return true;
}

void HDFEOS5CFSpecialCVArray::read_data_NOT_from_mem_cache(bool /*add_cache*/, void*/*buf*/) {

    BESDEBUG("h5","Coming to HDFEOS5CFSpecialCVArray: read_data_NOT_from_mem_cache "<<endl);

    bool check_pass_fileid_key = HDF5RequestHandler::get_pass_fileid();

    vector<int64_t> offset;
    vector<int64_t> count;
    vector<int64_t>step;
    int64_t nelms = 0;

    if (rank <= 0) {
        string msg = "The number of dimension of the variable is <=0 for this array.";
        msg += "The variable name is " + varname + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    else {
        offset.resize(rank);
        count.resize(rank);
        step.resize(rank);
        nelms = format_constraint (offset.data(), step.data(), count.data());
    }

    if(false == check_pass_fileid_key) {
        if ((fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {
            string msg = "HDF5 File " + filename + " cannot be opened. "; 
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }
    string cv_name = HDF5CFUtil::obtain_string_after_lastslash(varname);
    if ("" == cv_name) {
        string msg = "Cannot obtain TES CV attribute.";
        msg += "The variable name is " + varname + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    string group_name = varname.substr(0,varname.size()-cv_name.size());
    
    size_t cv_name_sep_pos = cv_name.find_first_of('_',0);  

    if (string::npos == cv_name_sep_pos) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "Cannot obtain TES CV attribute.";
        msg += "The variable name is " + varname + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    string cv_attr_name = cv_name.substr(0,cv_name_sep_pos);

    htri_t swath_link_exist = H5Lexists(fileid,group_name.c_str(),H5P_DEFAULT);

    if (swath_link_exist <= 0) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "The TES swath link doesn't exist.";
        msg += "The variable name is " + varname + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    
    htri_t swath_exist = H5Oexists_by_name(fileid,group_name.c_str(),H5P_DEFAULT); 
    if (swath_exist <= 0) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "The TES swath link doesn't exist.";
        msg += "The variable name is " + varname + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    htri_t cv_attr_exist = H5Aexists_by_name(fileid,group_name.c_str(),cv_attr_name.c_str(),H5P_DEFAULT);
    if (cv_attr_exist <= 0) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "The TES swath CV attribute doesn't exist.";
        msg += "The variable name is " + varname + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    hid_t cv_attr_id = H5Aopen_by_name(fileid,group_name.c_str(),cv_attr_name.c_str(),H5P_DEFAULT,H5P_DEFAULT);
    if (cv_attr_id <0) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "Cannot obtain the TES CV attribute id.";
        msg += "The variable name is " + varname + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    hid_t attr_type = -1;
    if ((attr_type = H5Aget_type(cv_attr_id)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += cv_attr_name;
        H5Aclose(cv_attr_id);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    hid_t attr_space = -1;
    if ((attr_space = H5Aget_space(cv_attr_id)) < 0) {
        string msg = "cannot get the hdf5 dataspace id for the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    auto attr_num_elm = (int)(H5Sget_simple_extent_npoints(attr_space));
    if (0 == attr_num_elm ) {
        string msg = "cannot get the number for the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    if (attr_num_elm != (total_num_elm -1)) {
        string msg = "cannot get the number for the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    if (dtype != H5FLOAT32 || dtype != HDF5CFUtil::H5type_to_H5DAPtype(attr_type)) {
        string msg = "cannot get the number for the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    hid_t attr_mem_type = -1;
    if ((attr_mem_type = H5Tget_native_type(attr_type,H5T_DIR_ASCEND)) < 0) {
        string msg = "cannot get the attribute datatype for the attribute  ";
        msg += cv_attr_name;
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    if (nelms <= 0 || (total_num_elm -1) <=0 ||total_num_elm < 0) { 
        H5Tclose(attr_mem_type);
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "Number of elements must be greater than 0.";
        msg += "The variable name is " + varname + ".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    vector<float> val;
    val.resize(nelms);

    vector<float>orig_val;
    orig_val.resize(total_num_elm-1);
  
    vector<float>total_val;
    total_val.resize(total_num_elm);


    if (H5Aread(cv_attr_id,attr_mem_type, (void*)orig_val.data())<0){
        string msg = "cannot retrieve the value of  the attribute ";
        msg += cv_attr_name;
        H5Tclose(attr_mem_type);
        H5Tclose(attr_type);
        H5Aclose(cv_attr_id);
        H5Sclose(attr_space);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        throw BESInternalError(msg,__FILE__, __LINE__);
    }
 
    
    // Panoply cannot accept the same value for the coordinate variable.
    // So I have to create a unique value for the added value. 
    // Since the first data value is 1211.53 hpa, which is below the main sea level
    // already(1013.25 hpa), so I will just use more weight(0.9) for the first data
    // value and less weight(0.1) for the second data value to calculate. 
    // As far as I know, no real data at the added level(fill value -999.0 for data value),
    // it will not affect any visualization results.
    // The position is position of added_value, position of the first_value, position of the second_value.
    // So the equal weight equation is first_value = 0.5*(added_value+second_value);
    // Add the weight, first_value = 0.95*added_value + 0.05*second_value;
    // The approximate formula for the added_value is 
    // add_value = first_value*1.1 -  0.1*second_value;
    // KY 2012-2-20
   
    total_val[0] = 1.1F*orig_val[0] - 0.1F*orig_val[1];
    for (int i = 1; i < total_num_elm; i++)
        total_val[i] = orig_val[i-1];

  
    // Note: offset and step in this case will never be nullptr since the total number of elements will be
    // greater than 1. This is enforced in line 180. KY 2014-02-27
    for (int i = 0; i <nelms; i++)
        val[i] = total_val[offset[0]+i*step[0]];

    set_value_ll(val.data(), nelms);
    H5Tclose(attr_type);
    H5Tclose(attr_mem_type);
    H5Aclose(cv_attr_id);
    H5Sclose(attr_space);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
 
    return;

}
