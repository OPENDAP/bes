// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Kent Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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

/// \file HDF5Byte.cc
/// \brief Implementation of mapping HDF5 byte to DAP Byte for the default option.
/// 
///
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)
///

#include <string>
#include <ctype.h>
#include <memory>

#include "config_hdf5.h"
#include "BESDebug.h"
#include <libdap/InternalErr.h>
#include <BESInternalError.h>
#include "h5dds.h"
#include "HDF5Byte.h"


using namespace std;
using namespace libdap;


BaseType *HDF5Byte::ptr_duplicate()
{
    auto HDF5Byte_unique = make_unique<HDF5Byte>(*this);
    return HDF5Byte_unique.release();
}

bool HDF5Byte::read()
{
    if (read_p())
	return true;

    // In the default option, the dataset() returns the file name. Note: this is not consistent with the corresponding CF option.
    // The dataset() will return the full path of the variable for the CF option scalar type. See HDF5CFByte.cc. 
    // Make dataset() return the same kind of object name is a low-priority TODO task. This also applies to HDF5Int16.cc, HDF5Float32.cc, etc.

    hid_t file_id = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if(file_id < 0) {
        string msg = "Fail to obtain the HDF5 file ID for the file " + dataset() +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
   
    // We don't need to distingush dap4 from dap2 since var_path is the variable's absolute path.
    hid_t dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
#if 0
    hid_t dset_id = -1;
    if(true == is_dap4()) 
        dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    else 
        dset_id = H5Dopen2(file_id,name().c_str(),H5P_DEFAULT);
#endif

    if(dset_id < 0) {
        H5Fclose(file_id);
        string msg = "Fail to obtain the HDF5 dataset ID for the variable " + var_path +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
    

    try {
	dods_byte buf;
	get_data(dset_id, (void *) &buf);
	set_read_p(true);
	set_value(buf);

        if (H5Dclose(dset_id) < 0) {
            string msg = "Unable to close the HDF5 dataset " + var_path +".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        H5Fclose(file_id);
    }

    catch(...) {
        H5Dclose(dset_id);
        H5Fclose(file_id);
        throw;
    }

    return true;
}

