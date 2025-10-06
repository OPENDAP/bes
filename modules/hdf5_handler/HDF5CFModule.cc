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
/// \file HDF5CFModule.cc
/// \brief This file includes the implementation to distinguish the different categories of HDF5 products for the CF option
///
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2023 The HDF Group
///
/// All rights reserved.

#include <libdap/InternalErr.h>
#include <BESInternalError.h>
#include "HDF5CFModule.h"
#include "h5apicompatible.h"

using namespace libdap;

H5CFModule check_module(hid_t fileid) {

    if (true == check_eos5(fileid)) 
        return HDF_EOS5;
    else if(true == check_jpss(fileid)) 
        return HDF5_JPSS;
    else 
        return HDF5_GENERAL;

}

bool check_eos5(hid_t file_id) {

    // check HDF-EOS5 group
    string eos5_check_group = "/HDFEOS INFORMATION";
    string eos5_check_attr  = "HDFEOSVersion";
    string eos5_dataset     = "StructMetadata.0";
    htri_t has_eos_group    = -1;
    bool   eos5_module_fields    = true;
    

    has_eos_group = H5Lexists(file_id,eos5_check_group.c_str(),H5P_DEFAULT);

    if (has_eos_group > 0){
  
        hid_t eos_group_id = -1;
        htri_t has_eos_attr = -1;

        // Open the group
        if((eos_group_id = H5Gopen(file_id, eos5_check_group.c_str(),H5P_DEFAULT))<0) {

            string msg = "cannot open the HDF5 group  ";
            msg += eos5_check_group;
            throw BESInternalError(msg,__FILE__, __LINE__);
        }

        // check the existence of the EOS5 attribute
        has_eos_attr = H5Aexists(eos_group_id, eos5_check_attr.c_str());

        if (has_eos_attr >0) {

            // All HDF-EOS5 conditions are fulfilled, return true
            // Otherwise, return false or throw an error.
            htri_t has_eos_dset = -1;

            // check the existence of the EOS5 dataset
            has_eos_dset = H5Lexists(eos_group_id,eos5_dataset.c_str(),H5P_DEFAULT);
            if (has_eos_dset >0) {
                // We still need to check if there are non-EOS5 fields that the 
                // current HDF-EOS5 module cannot handle.
                // If yes, the file cannot be handled by the HDF-EOS5 module since
                // the current module is very tight to the HDF-EOS5 model.
                // We will treat this file as a general HDF5 file.
                eos5_module_fields = check_eos5_module_fields(file_id);
                return eos5_module_fields;
            }
            else if(0 == has_eos_dset) 
                return false;
            else {
                string msg = "Fail to determine if the HDF5 dataset  ";
                msg += eos5_dataset;
                msg +=" exists ";
                H5Gclose(eos_group_id);
                throw BESInternalError(msg,__FILE__, __LINE__);
            }
        }
        else if(0 == has_eos_attr) 
                return false;
        else {
            string msg = "Fail to determine if the HDF5 attribute  ";
            msg += eos5_check_attr;
            msg +=" exists ";
            H5Gclose(eos_group_id);
            throw BESInternalError(msg,__FILE__, __LINE__);
        }
    }
    else if( 0 == has_eos_group) {
        return false;
    }
    else {
        string msg = "Fail to determine if the HDF5 group  ";
        msg += eos5_check_group;
        msg +=" exists ";
#if 0
//        H5Fclose(file_id);
#endif
        throw BESInternalError(msg,__FILE__, __LINE__);
    }
}
      
bool check_jpss(hid_t fileid) {
  // Currently not supported.
  return false;
}

bool check_eos5_module_fields(hid_t fileid){ 

    bool ret_value = true;
    string eos5_swath_group = "/HDFEOS/SWATHS";
    string eos5_grid_group  = "/HDFEOS/GRIDS";
    string eos5_zas_group   = "/HDFEOS/ZAS";
    bool swath_unsupported_dset = false;
    bool grid_unsupported_dset  = false;
    bool zas_unsupported_dset   = false;
    if(H5Lexists(fileid,eos5_swath_group.c_str(),H5P_DEFAULT)>0)
        swath_unsupported_dset = grp_has_dset(fileid,eos5_swath_group); 
    if(swath_unsupported_dset == true) 
        return false;
    else {
        if(H5Lexists(fileid,eos5_grid_group.c_str(),H5P_DEFAULT)>0)
            grid_unsupported_dset = grp_has_dset(fileid,eos5_grid_group);
        if(grid_unsupported_dset == true)
            return false;
        else {
            if(H5Lexists(fileid,eos5_zas_group.c_str(),H5P_DEFAULT)>0)
                zas_unsupported_dset = grp_has_dset(fileid,eos5_zas_group);
            if(zas_unsupported_dset == true) 
                return false;
        }
    }

    return ret_value;
}

bool grp_has_dset(hid_t fileid, const string & grp_path ) {

    bool ret_value = false;
    hid_t pid = -1;
    if((pid = H5Gopen(fileid,grp_path.c_str(),H5P_DEFAULT))<0){
        string msg = "Unable to open the HDF5 group ";
        msg += grp_path;
        throw BESInternalError(msg,__FILE__, __LINE__);
    }

    H5G_info_t g_info;

    if (H5Gget_info(pid, &g_info) < 0) {
        H5Gclose(pid);
        string msg = "Unable to obtain the HDF5 group info. for ";
        msg += grp_path;
        throw BESInternalError(msg,__FILE__, __LINE__);
    }
 
    hsize_t nelems = g_info.nlinks;

    for (hsize_t i = 0; i < nelems; i++) {

        // Obtain the object type
        H5O_info_t oinfo;
        if (H5OGET_INFO_BY_IDX(pid, ".", H5_INDEX_NAME, H5_ITER_NATIVE, i, &oinfo, H5P_DEFAULT) < 0) {
            string msg = "Cannot obtain the object info for the group";
            msg += grp_path;
            throw BESInternalError(msg,__FILE__, __LINE__);
        }

        if(oinfo.type == H5O_TYPE_DATASET) {
            ret_value = true;
            break;
        }
    }
    H5Gclose(pid);
    return ret_value;
}
