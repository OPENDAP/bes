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

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5Str.cc
///
/// \brief The implementation of translating HDF5 string into DAP string for the default option.
/// 
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)
///
/// Copyright (c) 2007-2023 HDF Group
/// 
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////


#include "config_hdf5.h"


#include <string>
#include <ctype.h>
#include <memory>

#include <libdap/InternalErr.h>

#include "h5dds.h"
#include "HDF5Str.h"
#include "BESDebug.h"

using namespace std;
using namespace libdap;

HDF5Str::HDF5Str(const string & n, const string &vpath, const string &d) 
 : Str(n,d),var_path(vpath)
{
}

BaseType *HDF5Str::ptr_duplicate()
{
    auto HDF5Str_unique = make_unique<HDF5Str>(*this);
    return HDF5Str_unique.release();
}

bool HDF5Str::read()
{

    if (read_p())
        return true;

    hid_t file_id = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
    if(file_id < 0) {
        string msg = "Fail to obtain the HDF5 file ID for the file " + dataset() +".";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

    hid_t dset_id = -1;
    if(true == is_dap4())
        dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    else
        dset_id = H5Dopen2(file_id,name().c_str(),H5P_DEFAULT);

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

    size_t size = H5Tget_size(dtypeid);

    if (size == 0) {
        H5Tclose(dtypeid);
	H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to obtain the size of datatype for the variable " + var_path +".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    try {

        if(H5Tis_variable_str(dtypeid) >0){
            vector<string>finstrval;
            finstrval.resize(1);
            read_vlen_string(dset_id,1,nullptr,nullptr,nullptr,finstrval);
            string strval = finstrval[0];
            set_value(strval);
        }
          
        else { 
            vector<char>chr;
            chr.resize(size+1);
	    get_data(dset_id, (void *) chr.data());
	    set_read_p(true);
            string str(chr.begin(),chr.end());
	    set_value(str);
        }
        H5Tclose(dtypeid);
        H5Dclose(dset_id);
        H5Fclose(file_id);

   }

    catch(...) {
        H5Tclose(dtypeid);
	H5Dclose(dset_id);
        H5Fclose(file_id);
        throw;
    }


    return true;
}

