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


// (c) COPYRIGHT URI/MIT 1994-1996
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      reza            Reza Nekovei (reza@intcomm.net)

// netCDF sub-class implementation for NCByte,...NCGrid.
// The files are patterned after the subcalssing examples
// Test<type>.c,h files.
//
// ReZa 1/12/95

#include "config_nc.h"

static char rcsid[] not_used = { "$Id$" };

#include <netcdf.h>

#include <InternalErr.h>
#include <util.h>

#include "NCByte.h"

// This `helper function' creates a pointer to the a NCByte and returns
// that pointer. It takes the same arguments as the class's ctor. If any of
// the variable classes are subclassed (e.g., to make a new Byte like
// HDFByte) then the corresponding function here, and in the other class
// definition files, needs to be changed so that it creates an instnace of
// the new (sub)class. Continuing the earlier example, that would mean that
// NewByte() would return a HDFByte, not a Byte.
//
// It is important that these function's names and return types do not change
// - they are called by the parser code (for the dds, at least) so if their
// names changes, that will break.
//
// The declarations for these fuctions (in util.h) should *not* need
// changing.


NCByte::NCByte(const string &n, const string &d) :
    Byte(n, d)
{
}

NCByte::NCByte(const NCByte &rhs) :
    Byte(rhs)
{
}

NCByte::~NCByte()
{
}

NCByte &
NCByte::operator=(const NCByte &rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast<NCByte&> (*this) = rhs;

    return *this;
}

BaseType *
NCByte::ptr_duplicate()
{
    return new NCByte(*this);
}

bool NCByte::read() {
    if (read_p()) // already done
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
        throw InternalErr(__FILE__, __LINE__,
                "Could not get variable ID for: " + name() + ". (error: " + long_to_string(errstat) + ").");

    dods_byte Dbyte;
#if NETCDF_VERSION >= 4
    errstat = nc_get_var(ncid, varid, &Dbyte);
#else
    size_t cor[MAX_NC_DIMS]; /* corner coordinates */
    int num_dim; /* number of dim. in variable */
    nc_type datatype; /* variable data type */
    errstat = nc_inq_var(ncid, varid, (char *) 0, &datatype, &num_dim, (int *) 0, (int *) 0);
    if (errstat != NC_NOERR) {
        throw Error(errstat, string("Could not read information about the variable `") + name() + string("'."));
    }
    if (datatype != NC_BYTE) {
        throw InternalErr(__FILE__, __LINE__, "Entered NCByte::read() with non-byte variable!");
    }

    for (int id = 0; id <= num_dim && id < MAX_NC_DIMS; id++) {
        cor[id] = 0;
    }

    errstat = nc_get_var1_uchar(ncid, varid, cor, &Dbyte);
#endif

    if (errstat != NC_NOERR)
        throw Error(errstat, string("Could not read the variable '") + name() + string("'."));

    set_read_p(true);

    val2buf(&Dbyte);

    if (nc_close(ncid) != NC_NOERR)
        throw InternalErr(__FILE__, __LINE__, "Could not close the dataset!");

    return true;
}

