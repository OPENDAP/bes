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
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  


#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "HDF5Grid.h"


BaseType *HDF5Grid::ptr_duplicate()
{
    return new HDF5Grid(*this);
}

HDF5Grid::HDF5Grid(const string & n, const string &d) : Grid(n, d)
{
    ty_id = -1;
    dset_id = -1;
}

HDF5Grid::~HDF5Grid()
{
}

bool HDF5Grid::read()
{
    if (read_p())               // nothing to do
        return false;

    // read array elements
    array_var()->read();
    // read maps' elements
    Map_iter p = map_begin();
    while (p != map_end()) {
        (*p)->read();
        ++p;
    }
    set_read_p(true);

    return false;
}

void HDF5Grid::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5Grid::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5Grid::get_did()
{
    return dset_id;
}

hid_t HDF5Grid::get_tid()
{
    return ty_id;
}
