// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file HDF5PathFinder.cc
/// \brief Implementation of finding and breaking a cycle in the HDF group.
/// Used to handle the rare case for thed default option.
///
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
///////////////////////////////////////////////////////////////////////////////

#include "BESDebug.h"
#include "HDF5PathFinder.h"

using namespace std;


bool HDF5PathFinder::add(const string & id, const string & name)
{
    BESDEBUG("h5", ">add(): id is:" << id << "   name is:" << name << endl);
    if (!visited(id)) {

        id_to_name_map[id] = name;
        return true;
    } else {
        BESDEBUG("h5", "=add(): already added." << endl);
        return false;
    }
}


bool HDF5PathFinder::visited(const string & id)
{
    string str = id_to_name_map[id];
    if (!str.empty()) {
        return true;
    } else {
        return false;
    }
}

string HDF5PathFinder::get_name(const string & id)
{
    return id_to_name_map[id];
}
