// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2011-2018 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file HDF5CFInt64.cc
/// \brief The implementation of mapping HDF5 64-bit integer to DAP4 Int64 for the CF option 
///
/// In the future, this may be merged with the default option.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#include "config_hdf5.h"

#include <BESDebug.h>
#include <libdap/InternalErr.h>
#include "HDF5CFInt64.h"
#include "h5common.h"
using namespace std;
using namespace libdap;

HDF5CFInt64::HDF5CFInt64(const string &n, const string &d) : Int64(n, d)
{
}

HDF5CFInt64::HDF5CFInt64(const string &n, const string &d,const string &d_f) : Int64(n, d),filename(d_f)
{
}
BaseType *HDF5CFInt64::ptr_duplicate()
{
    return new HDF5CFInt64(*this);
}

bool HDF5CFInt64::read()
{

    BESDEBUG("h5","Coming to HDF5CFInt64 read "<<endl);

    if (read_p())
        return true;

    hid_t file_id = H5Fopen(filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if(file_id < 0) {
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the HDF5 file ID .");
    }
   
    hid_t dset_id = -1;
    dset_id = H5Dopen2(file_id,dataset().c_str(),H5P_DEFAULT);

    if(dset_id < 0) {
        H5Fclose(file_id);
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the dataset .");
    }
    
    try {
        dods_int64 buf;
        get_data(dset_id, (void *) &buf);

        set_read_p(true);
        set_value(buf);

        // Release the handles.
        if (H5Dclose(dset_id) < 0) {
            throw InternalErr(__FILE__, __LINE__, "Unable to close the dset.");
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

