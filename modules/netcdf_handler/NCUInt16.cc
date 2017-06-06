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


// (c) COPYRIGHT URI/MIT 1996
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      Reza       Reza Nekovei (rnekovei@ieee.org)

#include "config_nc.h"

static char rcsid[] not_used = { "$Id$" };

#include <netcdf.h>
#include <InternalErr.h>

#include "NCUInt16.h"

NCUInt16::NCUInt16(const string &n, const string &d) :
    UInt16(n, d)
{
}

NCUInt16::NCUInt16(const NCUInt16 &rhs) :
    UInt16(rhs)
{
}

NCUInt16::~NCUInt16()
{
}

NCUInt16 &
NCUInt16::operator=(const NCUInt16 &rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast<NCUInt16&> (*this) = rhs;

    return *this;
}

BaseType *
NCUInt16::ptr_duplicate()
{

    return new NCUInt16(*this);
}

bool NCUInt16::read()
{
    if (read_p()) // nothing to do
        return true;

    int ncid, errstat;
    errstat = nc_open(dataset().c_str(), NC_NOWRITE, &ncid); /* netCDF id */
    if (errstat != NC_NOERR) {
        string err = "Could not open the dataset's file (" + dataset() + ")";
        throw Error(errstat, err);
    }

    int varid; /* variable Id */
    errstat = nc_inq_varid(ncid, name().c_str(), &varid);
    if (errstat != NC_NOERR)
        throw Error(errstat, "Could not get variable ID.");

    short sht;
#if NETCDF_VERSION >= 4
    errstat = nc_get_var(ncid, varid, &sht);
#else
    size_t cor[MAX_NC_DIMS];    /* corner coordinates */
    int num_dim;                /* number of dim. in variable */
    nc_type datatype;           /* variable data type */
    errstat = nc_inq_var(ncid, varid, (char *)0, &datatype, &num_dim, (int *)0,
			(int *)0);
    if( errstat != NC_NOERR )
    {
	throw Error(errstat,string("Could not read information about the variable `") + name() + string("'."));
    }
    if( datatype != NC_SHORT )
    {
	throw InternalErr(__FILE__, __LINE__, "Entered NCUInt16::read() with non-UInt16 variable!");
    }

    for( int id = 0; id <= num_dim && id < MAX_NC_DIMS; id++ )
    {
	cor[id] = 0;
    }

    errstat = nc_get_var1_short( ncid, varid, cor, &sht ) ;
#endif
    if (errstat != NC_NOERR)
        throw Error(errstat, string("Could not read the variable `") + name() + string("'."));

    set_read_p(true);

    dods_uint16 uintg16 = (dods_uint16) sht;
    val2buf(&uintg16);

    if (nc_close(ncid) != NC_NOERR)
        throw InternalErr(__FILE__, __LINE__, "Could not close the dataset!");

    return true;
}
