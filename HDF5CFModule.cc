// This file is part of the hdf5_handler implementing for the CF-compliant
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

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFModule.cc
/// \brief This file includes the implementation to distinguish the different categories of HDF5 products for the CF option
///
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#include <InternalErr.h>
#include "HDF5CFModule.h"

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

    has_eos_group = H5Lexists(file_id,eos5_check_group.c_str(),H5P_DEFAULT);

    if (has_eos_group > 0){
  
        hid_t eos_group_id = -1;
        htri_t has_eos_attr = -1;

        // Open the group
        if((eos_group_id = H5Gopen(file_id, eos5_check_group.c_str(),H5P_DEFAULT))<0) {

            string msg = "cannot open the HDF5 group  ";
            msg += eos5_check_group;
//            H5Fclose(file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // check the existence of the EOS5 attribute
        has_eos_attr = H5Aexists(eos_group_id, eos5_check_attr.c_str());

        if (has_eos_attr >0) {

            // All HDF-EOS5 conditions are fulfilled, return true;
            // Otherwise, return false or throw an error.
            htri_t has_eos_dset = -1;

            // check the existence of the EOS5 dataset
            has_eos_dset = H5Lexists(eos_group_id,eos5_dataset.c_str(),H5P_DEFAULT);
            if (has_eos_dset >0) 
                return true;
            else if(0 == has_eos_dset) 
                return false;
            else {
                string msg = "Fail to determine if the HDF5 dataset  ";
                msg += eos5_dataset;
                msg +=" exists ";
                H5Gclose(eos_group_id);
//                H5Fclose(file_id);
                throw InternalErr(__FILE__, __LINE__, msg);
            }
        }
        else if(0 == has_eos_attr) 
                return false;
        else {
            string msg = "Fail to determine if the HDF5 attribute  ";
            msg += eos5_check_attr;
            msg +=" exists ";
            H5Gclose(eos_group_id);
 //           H5Fclose(file_id);
            throw InternalErr(__FILE__, __LINE__, msg);
        }
    }
    else if( 0 == has_eos_group) 
        return false;
    else {
        string msg = "Fail to determine if the HDF5 group  ";
        msg += eos5_check_group;
        msg +=" exists ";
//        H5Fclose(file_id);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
}
      
bool check_jpss(hid_t fileid) {
  // Currently not supported.
  return false;
}
