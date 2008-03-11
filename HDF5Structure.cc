/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf5 data handler for the OPeNDAP data server.
//
// Copyright (c) 2005 OPeNDAP, Inc.
// Copyright (c) 2007 HDF Group, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//         Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

#ifdef _GNUG_
#pragma implementation
#endif

// #define DODS_DEBUG

#include <string>
#include <ctype.h>
#include "config_hdf5.h"
#include "hdf5.h"
#include "h5dds.h"
#include "HDF5Structure.h"
#include "InternalErr.h"
#include "debug.h"

BaseType *HDF5Structure::ptr_duplicate()
{
    return new HDF5Structure(*this);
}

HDF5Structure::HDF5Structure(const string & n):Structure(n)
{
    ty_id = -1;
    dset_id = -1;
    array_index = 0;
    array_size = 1;
    array_entire_size = 1;
}

HDF5Structure::~HDF5Structure()
{
}

HDF5Structure & HDF5Structure::operator=(const HDF5Structure & rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast < Structure & >(*this) = rhs;  // run Structure assignment


    return *this;
}

bool HDF5Structure::read(const string & dataset)
{

    int i = 0;
    int err = 0;
    Constructor::Vars_iter q;

    DBG(cerr
        << ">read() dataset=" << dataset
        << " array_index= " << array_index << endl);

    if (read_p())
        return false;


    // Read each member in the structure.
    for (q = var_begin(); err == 0 && q != var_end(); ++q, ++i) {
        DBG(cerr << "=read() i=" << i << endl);
        BaseType *p = dynamic_cast < BaseType * >(*q);
        p->read(dataset);
    }

    set_read_p(true);
    return false;
}

void HDF5Structure::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5Structure::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5Structure::get_did()
{
    return dset_id;
}

hid_t HDF5Structure::get_tid()
{
    return ty_id;
}

void HDF5Structure::set_array_index(int i)
{
    array_index = i;
}

int HDF5Structure::get_array_index()
{
    return array_index;
}

void HDF5Structure::set_array_size(int i)
{
    array_size = i;
}

int HDF5Structure::get_array_size()
{
    return array_size;
}

void HDF5Structure::set_entire_array_size(int i)
{
    array_entire_size = i;
}

int HDF5Structure::get_entire_array_size()
{
    return array_entire_size;
}
