// This file is part of the hdf4 data handler for the OPeNDAP data server.
// Copyright (c) The HDF Group.
// Copyright (c) 2005 OPeNDAP, Inc.
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
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820

/////////////////////////////////////////////////////////////////////////////

#include "config_hdf.h"

#include <libdap/InternalErr.h>
#include "HDFInt8.h"
using namespace libdap;
using namespace std;

HDFInt8::HDFInt8(const string & n, const string &d) : Int8(n, d)
{
}

HDFInt8::~HDFInt8() = default;

BaseType *HDFInt8::ptr_duplicate()
{
    return new HDFInt8(*this);
}

bool HDFInt8::read()
{
    throw InternalErr(__FILE__, __LINE__,
                      "Unimplemented read method called.");
}

