// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of nc_handler, a data handler for the OPeNDAP data
// server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <netcdf.h>

bool is_user_defined_type(int ncid, int type)
{
#if NETCDF_VERSION >= 4
    return type >= NC_FIRSTUSERTYPEID;

#if 0
    // Use the above - it's simpler and supported in the netcdf.h header. jhrg 8/2/18
    int ntypes;
    int typeids[NC_MAX_VARS];  // It's likely safe to assume there are
    // no more types than variables. jhrg
    // 2/9/12
    int err = nc_inq_typeids(ncid, &ntypes, typeids);
    if (err != NC_NOERR)
    throw Error(err, "Could not get the user defined type information.");

    for (int i = 0; i < ntypes; ++i) {
        if (type == typeids[i])
        return true;
    }

    return false;
#endif

#else
    return false;
#endif
}

