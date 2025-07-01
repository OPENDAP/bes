// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.
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

/// \file HDF5D4Enum.cc
/// \brief HDF5 Enum to DAP for the default option. 
/// 
///


#include "BESDebug.h"
#include <string>
#include <ctype.h>
#include <memory>

#include <libdap/InternalErr.h>
#include <BESInternalError.h>

#include "HDF5D4Enum.h"

using namespace std;
using namespace libdap;

HDF5D4Enum::HDF5D4Enum(const string & n, const string &vpath,const string &d, libdap::Type type) : D4Enum(n, d, type),var_path(vpath)
{
}

BaseType *HDF5D4Enum::ptr_duplicate()
{
    auto HDF5D4Enum_unique = make_unique<HDF5D4Enum>(*this);
    return HDF5D4Enum_unique.release();
}

bool HDF5D4Enum::read()
{
    if (read_p())
     return true;

    if (false == is_dap4()) {
        string msg = "HDF5 ENUM only maps to DAP4 ENUM. You are asking for a DAP2 response.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    hid_t file_id = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if(file_id < 0) {
        string msg = "Fail to obtain the HDF5 file ID for the file " + dataset() + ".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
   
    hid_t dset_id = -1;
    dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    if(dset_id < 0) {
        H5Fclose(file_id);
        string msg = "Fail to obtain the HDF5 dataset ID for the variable " + var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
    
    hid_t dtypeid = H5Dget_type(dset_id); 
    if(dtypeid < 0) { 
        H5Dclose(dset_id); 
        H5Fclose(file_id); 
        string msg = "Fail to obtain the datatype for the variable " + var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg); 
    } 

    if (H5T_ENUM != H5Tget_class(dtypeid)) {
        H5Dclose(dset_id); 
        H5Fclose(file_id); 
        string msg = "Fail to obtain the enum datatype class for the variable " + var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg); 
    }

    hid_t base_type = H5Tget_super(dtypeid);

    hid_t memtype = H5Tget_native_type(base_type, H5T_DIR_ASCEND);
    if (memtype < 0){
        close_objids(-1,base_type,dtypeid,dset_id,file_id);
        string msg = "Cannot obtain the native datatype of the enum base type for the variable " + var_path + ".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (H5Tget_class(base_type) != H5T_INTEGER) {
        close_objids(memtype,base_type,dtypeid,dset_id,file_id);
        string msg = "We currently only support the case when the enum's base type is an integer. The enum variable name is: "+var_path+".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    size_t dsize = H5Tget_size(base_type);
    if (dsize == 0){
        close_objids(memtype,base_type,dtypeid,dset_id,file_id);
        string msg = "Size of enum base data type is invalid. The enum variable name is: "+var_path+".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    H5T_sign_t sign = H5Tget_sign(base_type);
    if (sign < 0){
        close_objids(memtype,base_type,dtypeid,dset_id,file_id);
        string msg = "Sign of enum base data type is invalid. The enum variable name is: "+var_path+".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    switch (dsize) {

    case 1:
        if (sign == H5T_SGN_NONE) {
            uint8_t  val = 0;
            if (H5Dread(dset_id,H5T_NATIVE_UCHAR,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val)<0) {
                close_objids(memtype,base_type,dtypeid,dset_id,file_id);
                string msg = "Cannot read the enum data for the unsigned char base type. ";
                msg += "The enum variable name is: "+var_path+".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value(val);
        } 
        else {
            int8_t val = 0;
            if (H5Dread(dset_id,H5T_NATIVE_SCHAR,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val)<0) {
                close_objids(memtype,base_type,dtypeid,dset_id,file_id);
                string msg = "Cannot read the enum data for the signed char base type. ";
                msg += "The enum variable name is: "+var_path+".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value(val);
        }
        break;
    case 2:

        if (sign == H5T_SGN_NONE) {
            uint16_t val = 0;
            if (H5Dread(dset_id,H5T_NATIVE_USHORT,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val)<0) {
                close_objids(memtype,base_type,dtypeid,dset_id,file_id);
                string msg = "Cannot read the enum data for the unsigned 16-bit integer base type. ";
                msg += "The enum variable name is: "+var_path+".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value(val);
        } 
        else {
            int16_t val = 0;
            if (H5Dread(dset_id,H5T_NATIVE_SHORT,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val)<0) {
                close_objids(memtype,base_type,dtypeid,dset_id,file_id);
                string msg = "Cannot read the enum data for the 16-bit integer base type. ";
                msg += "The enum variable name is: "+var_path+".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value(val);
        }

        break;

    case 4:
        if (sign == H5T_SGN_NONE) {
            uint32_t val = 0;
            if (H5Dread(dset_id,H5T_NATIVE_UINT,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val)<0) {
                close_objids(memtype,base_type,dtypeid,dset_id,file_id);
                string msg = "Cannot read the enum data for the unsigned 32-bit integer base type. ";
                msg += "The enum variable name is: "+var_path+".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value(val);
 
        } 
        else {
            int32_t val = 0;
            if (H5Dread(dset_id,H5T_NATIVE_INT,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val)<0) {
                close_objids(memtype,base_type,dtypeid,dset_id,file_id);
                string msg = "Cannot read the enum data for the 32-bit integer base type. ";
                msg += "The enum variable name is: "+var_path+".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value(val);
 
        }
        break;

    case 8:
        if (sign == H5T_SGN_NONE) {
            uint64_t val = 0;
            if (H5Dread(dset_id,H5T_NATIVE_ULLONG,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val)<0) {
                close_objids(memtype,base_type,dtypeid,dset_id,file_id);
                string msg = "Cannot read the enum data for the unsigned 64-bit integer base type. ";
                msg += "The enum variable name is: "+var_path+".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value(val);
 
        } 
        else {
            int64_t val = 0;
            if (H5Dread(dset_id,H5T_NATIVE_LLONG,H5S_ALL,H5S_ALL,H5P_DEFAULT,&val)<0) {
                close_objids(memtype,base_type,dtypeid,dset_id,file_id);
                string msg = "Cannot read the enum data for the 64-bit integer base type. ";
                msg += "The enum variable name is: "+var_path+".";
                throw BESInternalError(msg,__FILE__,__LINE__);
            }
            set_value(val);
        }
        break;

    default: 
        close_objids(memtype,base_type,dtypeid,dset_id,file_id);
        string msg =  "The enum base type size is not within the currently supported values.";
        msg += "The enum variable name is: "+var_path+".";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    
    close_objids(memtype,base_type,dtypeid,dset_id,file_id);

    return true;
}

void HDF5D4Enum::close_objids(hid_t mem_type, hid_t base_type, hid_t dtype, hid_t dset_id, hid_t file_id) const {

    if (mem_type >0)
        H5Tclose(mem_type);
    if (base_type >0)
        H5Tclose(base_type);
    if (dtype >0)
        H5Tclose(dtype);
    if (dset_id >0)
        H5Dclose(dset_id);
    if (file_id >0)
        H5Fclose(file_id);

}
