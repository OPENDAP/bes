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

// #define DODS_DEBUG 1
#include <netcdf.h>

#include <InternalErr.h>
#include "NCStr.h"

#include <debug.h>

NCStr::NCStr(const string &n, const string &d) :
    Str(n, d)
{
}

NCStr::NCStr(const NCStr &rhs) :
    Str(rhs)
{
}

NCStr::~NCStr()
{
}

NCStr &
NCStr::operator=(const NCStr &rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast<NCStr&> (*this) = rhs;

    return *this;
}

BaseType *
NCStr::ptr_duplicate()
{
    return new NCStr(*this);
}

// This method assumes that NC_CHAR variables with zero or one dimension will
// be represented as a DAP String. If there are two or more dimensions, then
// the variable is represented in an array of DAP Strings.
bool NCStr::read()
{
    if (read_p()) //has been done
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

    nc_type datatype; /* variable data type */
    int num_dim; /* number of dim. in variable */
    errstat = nc_inq_var(ncid, varid, (char *) 0, &datatype, &num_dim, (int *) 0, (int *) 0);
    if (errstat != NC_NOERR)
        throw Error(errstat, string("Could not read information about the variable `") + name() + string("'."));

#if NETCDF_VERSION == 3
    // This stuff is only relevant when the handler is linked to a netcdf3
    // library.
    if (datatype != NC_CHAR)
        throw InternalErr(__FILE__, __LINE__, "Entered String read method with non-string/char variable!");
#endif

    switch (datatype) {
        case NC_CHAR:
            if (num_dim == 1) {
                int dim_id;
                errstat = nc_inq_vardimid(ncid, varid, &dim_id);
                if (errstat != NC_NOERR)
                    throw Error(errstat, string("Could not read the dimension id of `") + name() + string("'."));
                size_t dim_size;
                errstat = nc_inq_dimlen(ncid, dim_id, &dim_size);
                if (errstat != NC_NOERR)
                    throw Error(errstat, string("Could not read  the dimension size of `") + name() + string("'."));

                char *charbuf = new char[dim_size + 1];
                // get the data
                size_t cor[1] = { 0 };
                size_t edg[1];
                edg[0] = dim_size;

                errstat = nc_get_vara_text(ncid, varid, cor, edg, charbuf);
                if (errstat != NC_NOERR) {
                    delete[] charbuf;
                    throw Error(errstat, string("Could not read data from the variable `") + name() + string("'."));
                }

                charbuf[dim_size] = '\0';
                // poke the data into the DAP string
                set_value(string(charbuf));

                delete[] charbuf;
            }
            else if (num_dim == 0) { // the variable is a scalar, so it's just one character.
                char *charbuf = new char[2];
                // get the data
                errstat = nc_get_var_text(ncid, varid, charbuf);
                if (errstat != NC_NOERR) {
                    delete[] charbuf;
                    throw Error(errstat, string("Could not read data from the variable `") + name() + string("'."));
                }

                charbuf[1] = '\0';
                // poke the data into the DAP string
                set_value(string(charbuf));

                delete[] charbuf;
            }
            else
                throw Error(string("Multidimensional character array found in string class while reading '") + name() + string("'."));

            break;
#if NETCDF_VERSION >= 4
        case NC_STRING: {
            size_t cor[MAX_NC_DIMS]; /* corner coordinates */
            for (int id = 0; id <= num_dim && id < MAX_NC_DIMS; id++)
                cor[id] = 0;

            // before using vector<>, this code used 'char **strpp = new char*[2];'
            // replaced this 'vector<char> strpp(sizeof(char*));' with...
            vector<char*> strpp(1);

            // get the data
            errstat = nc_get_var1_string(ncid, varid, cor, &strpp[0]);
            if (errstat != NC_NOERR) {
                throw Error(errstat, string("Could not read data from the variable `") + name() + string("'."));
            }

            // poke the data into the DAP string
            // replaced this 'set_value(string(*(char**)&strpp[0]));' with ...
            set_value(string(strpp[0]));

            nc_free_string(1, &strpp[0]);

            break;
        }
#endif
        default:
            throw InternalErr(__FILE__, __LINE__, "Entered String read method with an unrecognized datatype!");

    }

    return true;
}
