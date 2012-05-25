// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5Url.cc
/// \brief Implementation of generating DAP URL type for the default option.
///
/// \author James Gallagher <jgallagher@opendap.org>
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// \see String
////////////////////////////////////////////////////////////////////////////////


#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "HDF5Url.h"
#include "InternalErr.h"

HDF5Url::HDF5Url(const string &n, const string &d) : Url(n, d)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *HDF5Url::ptr_duplicate()
{
    return new HDF5Url(*this);
}

bool HDF5Url::read()
{
    hobj_ref_t rbuf;

    if (H5Dread(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
		&rbuf) < 0) {
	throw InternalErr(__FILE__, __LINE__, "H5Dread() failed.");
    }

    hid_t did_r = H5Rdereference(dset_id, H5R_OBJECT, &rbuf);
    char name[DODS_NAMELEN];
    if (did_r < 0){
	throw InternalErr(__FILE__, __LINE__, "H5Rdereference() failed.");
    }
    if (H5Iget_name(did_r, name, DODS_NAMELEN) < 0){
	throw InternalErr(__FILE__, __LINE__, "Unable to retrieve the name of the object.");
    }
    string reference = name;
    set_value(reference);

     // Release the handles.
    if (H5Tclose(ty_id) < 0) {
       throw InternalErr(__FILE__, __LINE__, "Unable to close the datatype.");
    }
    if (H5Dclose(dset_id) < 0) {
       throw InternalErr(__FILE__, __LINE__, "Unable to close the dset.");
    }


    return false;
}

void HDF5Url::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5Url::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5Url::get_did()
{
    return dset_id;
}

hid_t HDF5Url::get_tid()
{
    return ty_id;
}
