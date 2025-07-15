/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

// (c) COPYRIGHT URI/MIT 1994-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.

// Author: James Gallagher
//

#include "config_hdf.h"

#include <BESInternalError.h>
#include "HDFInt16.h"

using namespace libdap;
using namespace std;

HDFInt16::HDFInt16(const string &n, const string &d) : Int16(n, d)
{
}

HDFInt16::~HDFInt16() = default;

BaseType *HDFInt16::ptr_duplicate()
{
    return new HDFInt16(*this);
}

bool HDFInt16::read()
{
    throw BESInternalError("Unimplemented read method called.",__FILE__, __LINE__);
}

