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

/// \file HDF5Int16.cc
/// \brief Implementation of mappign HDF5 signed 16-bit integer to DAP for the default option. 
/// 
///
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)
///


#include "config_hdf5.h"
#include "BESDebug.h"
#include <string>
#include <ctype.h>
#include <memory>

#include <libdap/InternalErr.h>

#include "h5dds.h"
#include "HDF5Int16.h"

using namespace std;
using namespace libdap;

#if 0
typedef struct s2_int16_t {
    /// Buffer for a 16 bit integer in compound data
    dods_int16 a;
} s2_int16_t;

#endif

HDF5Int16::HDF5Int16(const string & n, const string &vpath,const string &d) : Int16(n, d),var_path(vpath)
{
}

BaseType *HDF5Int16::ptr_duplicate()
{
    auto HDF5Int16_unique = make_unique<HDF5Int16>(*this);
    return HDF5Int16_unique.release();
}

bool HDF5Int16::read()
{
    if (read_p())
     return true;

    hid_t file_id = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if(file_id < 0) {
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the HDF5 file ID .");
    }
   
    hid_t dset_id = -1;
    if(true == is_dap4())
        dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    else
        dset_id = H5Dopen2(file_id,name().c_str(),H5P_DEFAULT);

    if(dset_id < 0) {
        H5Fclose(file_id);
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the datatype .");
    }
    
    hid_t dtypeid = H5Dget_type(dset_id); 
    if(dtypeid < 0) { 
        H5Dclose(dset_id); 
        H5Fclose(file_id); 
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the datatype ."); 
    } 

    hid_t memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND);

    if (memtype < 0){
        H5Tclose(dtypeid);
        H5Dclose(dset_id); 
        H5Fclose(file_id); 
        throw InternalErr(__FILE__, __LINE__, "Cannot obtain the native datatype.");
    }

    try {
        if(false == is_dap4()) {
            if (1 == H5Tget_size(memtype) && H5T_SGN_2 == H5Tget_sign(memtype)) {
                dods_int16 buf;
                signed char buf2; // Needs to be corrected with signed int8 buffer.
                get_data(dset_id, (void *) &buf2);
                buf = (short) buf2;
                set_read_p(true);
                set_value(buf);
            }
            else if (get_dap_type(memtype,false) == "Int16") {
                dods_int16 buf;
                get_data(dset_id, (void *) &buf);
                set_read_p(true);
                set_value(buf);
            }
        }
        else {
            dods_int16 buf;
            get_data(dset_id, (void *) &buf);
            set_read_p(true);
            set_value(buf);
        }
         
        // Release the handles.
        if (H5Tclose(memtype) < 0) {
            throw InternalErr(__FILE__, __LINE__, "Unable to close the datatype.");
        }
        H5Tclose(dtypeid);
        if (H5Dclose(dset_id) < 0) {
            throw InternalErr(__FILE__, __LINE__, "Unable to close the dset.");
        }

        H5Fclose(file_id);
    }

    catch(...) {
        H5Tclose(memtype);
        H5Tclose(dtypeid);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        throw;
    }
    return true;
}

