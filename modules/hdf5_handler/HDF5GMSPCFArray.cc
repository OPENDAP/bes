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
/// \file HDF5GMSPCFArray.cc
/// \brief The implementation of the retrieval of data values for special  HDF5 products
/// Currently this only applies to ACOS level 2 and OCO2 level 1B data.
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
#include "HDF5GMSPCFArray.h"

using namespace std;
using namespace libdap;

BaseType *HDF5GMSPCFArray::ptr_duplicate()
{
    auto HDF5GMSPCFArray_unique = make_unique<HDF5GMSPCFArray>(*this);
    return HDF5GMSPCFArray_unique.release();
}

bool HDF5GMSPCFArray::read()
{
    BESDEBUG("h5","Coming to HDF5GMSPCFArray read "<<endl);
    if(length() == 0)
        return true;

    read_data_NOT_from_mem_cache(false,nullptr);

    return true;
}

void HDF5GMSPCFArray::read_data_NOT_from_mem_cache(bool /*add_cache*/,void*/*buf*/) {

    BESDEBUG("h5","Coming to HDF5GMSPCFArray: read_data_NOT_from_mem_cache "<<endl);

    bool check_pass_fileid_key = HDF5RequestHandler::get_pass_fileid();

    vector<int64_t>offset;
    vector<int64_t>count;
    vector<int64_t>step;
    vector<hsize_t>hoffset;
    vector<hsize_t>hcount;
    vector<hsize_t>hstep;

    int64_t nelms = 0;

    if((otype != H5INT64 && otype !=H5UINT64) 
       || (dtype !=H5INT32)) { 
        string msg = "The datatype of the special product is not right.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    if (rank <= 0) { 
        string msg = "The number of dimension of the variable is <=0 for this array.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }
    else {

        offset.resize(rank);
        count.resize(rank);
        step.resize(rank);
        hoffset.resize(rank);
        hcount.resize(rank);
        hstep.resize(rank);
 

        nelms = format_constraint (offset.data(), step.data(), count.data());

        for (int64_t i = 0; i <rank; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }
    }

    hid_t dsetid = -1;
    hid_t dspace = -1;
    hid_t mspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;

    if(false == check_pass_fileid_key) {
        if ((fileid = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT))<0) {
            string msg = "HDF5 File " + filename + " cannot be opened. ";
            throw InternalErr (__FILE__, __LINE__, msg);
        }
    }

    if ((dsetid = H5Dopen(fileid,varname.c_str(),H5P_DEFAULT))<0) {
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "HDF5 dataset " + varname + " cannot be opened. ";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    if ((dspace = H5Dget_space(dsetid))<0) {
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "Space id of the HDF5 dataset " + varname + " cannot be obtained. ";
        throw InternalErr (__FILE__, __LINE__, msg);
    }


    if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                           hoffset.data(), hstep.data(),
                           hcount.data(), nullptr) < 0) {
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "The selection of hyperslab of the HDF5 dataset " + varname + " fails. ";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    mspace = H5Screate_simple(rank, (const hsize_t*)hcount.data(),nullptr);
    if (mspace < 0) {
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "The creation of the memory space of the  HDF5 dataset " + varname + "fails. ";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
        H5Sclose(mspace);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "Obtaining the datatype of the HDF5 dataset " + varname + " fails.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {
        H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "Obtaining the memory type of the HDF5 dataset " + varname + " fails. ";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    H5T_class_t ty_class = H5Tget_class(dtypeid);
    if (ty_class < 0) {
        H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "Obtaining the type class of the HDF5 dataset " + varname + " fails. ";
        throw InternalErr (__FILE__, __LINE__, msg);

    }

    if (ty_class !=H5T_INTEGER) {
        H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "The type class of the HDF5 dataset " + varname + " is not H5T_INTEGER. ";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    size_t ty_size = H5Tget_size(dtypeid);
    if (ty_size != H5Tget_size(H5T_STD_I64LE)) {
        H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "The type size of the HDF5 dataset " + varname + " is not right. ";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    hid_t read_ret = -1;


    vector<long long>orig_val;
    orig_val.resize(nelms);

    vector<int> val;
    val.resize(nelms);
 
    read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,orig_val.data());
    if (read_ret < 0) {
        H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Dclose(dsetid);
        HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);
        string msg = "Cannot read the HDF5 dataset " + varname + " with type of 64-bit integer.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    // Create "Time" or "Date" part of the original array. 
    // First get 10 power number of bits right
    int max_num = 1;
    for (int i = 0; i <numofdbits; i++) 
        max_num = 10 * max_num;

    int num_cut = 1;
    for (int i = 0; i<(sdbit-1) ; i++) 
        num_cut = 10 *num_cut;
    
    // Second generate the number based on the starting bit and number of bits
    // For example, number 1234, starting bit is 1, num of bits is 2

    // The final number is 34. The formula is
    //  (orig_val/pow(sbit-1)))%(pow(10,nbits))
    // In this example, 34 = (1234/1)%(100) = 34

    for (int64_t i = 0; i <nelms; i ++) 
        val[i] = (orig_val[i]/num_cut)%max_num;


    set_value_ll ((dods_int32 *)val.data(),nelms);
       
    H5Sclose(mspace);
    H5Tclose(dtypeid);
    H5Tclose(memtype);
    H5Sclose(dspace);
    H5Dclose(dsetid);
    HDF5CFUtil::close_fileid(fileid,check_pass_fileid_key);

    return;
}
