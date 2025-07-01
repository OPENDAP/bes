// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

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
/// \file HDF5CFInt16.cc
/// \brief The implementation of mapping HDF5 16-bit integer to DAP int16 for the CF option 
///
/// In the future, this may be merged with the default option.
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#include <memory>
#include <libdap/InternalErr.h>
#include "HDF5CFInt16.h"
#include <BESDebug.h>
#include "h5common.h"

using namespace std;
using namespace libdap;

HDF5CFInt16::HDF5CFInt16(const string &n, const string &d) : Int16(n, d)
{

}

HDF5CFInt16::HDF5CFInt16(const string &n, const string &d,const string &d_f) : Int16(n, d),filename(d_f)
{
}

BaseType *HDF5CFInt16::ptr_duplicate()
{
    auto HDF5CFInt16_unique = make_unique<HDF5CFInt16>(*this);
    return HDF5CFInt16_unique.release();
}

bool HDF5CFInt16::read()
{
    BESDEBUG("h5","Coming to HDF5CFInt16 read "<<endl);

    if (read_p())
        return true;

    hid_t file_id = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if(file_id < 0) {
        string msg = "Fail to obtain the HDF5 file ID for the file " + dataset() +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
   
    hid_t dset_id = -1;
    dset_id = H5Dopen2(file_id,dataset().c_str(),H5P_DEFAULT);
    if(dset_id < 0) {
        H5Fclose(file_id);
        string msg = "Fail to obtain the HDF5 dataset ID for the variable " + dataset() +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    hid_t dtypeid = H5Dget_type(dset_id); 
    if(dtypeid < 0)  { 
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to obtain the datatype for the variable " + dataset() +".";
        throw InternalErr(__FILE__,__LINE__, msg); 
    }

    hid_t memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND);
    if (memtype < 0){
        H5Tclose(dtypeid);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Cannot obtain the native datatype for the variable " + dataset() +".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }
  
    try {

        dods_int16 buf;
        if(1 == H5Tget_size(memtype) && H5T_SGN_2 == H5Tget_sign(memtype)) {
            signed char buf2;
            get_data(dset_id,(void*)&buf2);
            buf =(short)buf2;
        }
        else 
            get_data(dset_id, (void *) &buf);
        
        set_read_p(true);
        set_value(buf);

        if(H5Tclose(memtype) < 0) {
            string msg = "Unable to close the memory datatype for the variable " + dataset() + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        if(H5Tclose(dtypeid) < 0) {
            string msg = "Unable to close the datatype id for the variable " + dataset() + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        // Release the handles.
        if (H5Dclose(dset_id) < 0) {
            string msg = "Unable to close the HDF5 dataset for the variable " + dataset() + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
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

