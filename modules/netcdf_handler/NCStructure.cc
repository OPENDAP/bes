
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

static char rcsid[] not_used ={"$Id$"};

#include <vector>
#include <algorithm>

#include <netcdf.h>

#include <D4Attributes.h>
#include <util.h>
#include <InternalErr.h>

#include "nc_util.h"
#include "NCStructure.h"
#include "NCArray.h"

BaseType *
NCStructure::ptr_duplicate()
{
    return new NCStructure(*this);
}

NCStructure::NCStructure(const string &n, const string &d) : Structure(n, d)
{
}

NCStructure::NCStructure(const NCStructure &rhs) : Structure(rhs)
{
}

NCStructure::~NCStructure()
{
}

NCStructure &
NCStructure::operator=(const NCStructure &rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast<Structure&>(*this) = rhs; // run Structure assignment

    return *this;
}

/** When flattening a Structure, make sure to add an attribute to the
    new variables that indicate they are the product of translation. */

class AddAttribute: public unary_function<BaseType *, void> {

public:
    AddAttribute() {}

    void operator()(BaseType *a) {
        AttrTable *at;
        AttrTable::Attr_iter aiter;
        a->get_attr_table().find("translation", &at, &aiter);
        if (a->get_attr_table().attr_end() == aiter) {
            a->get_attr_table().append_attr("translation", "String",
                                            "\"flatten\"");
        }
    }
};

/**
 * Transfer attributes from a separately built DAS to the DDS. This method
 * overrides the implementation found in libdap to accommodate the special
 * characteristics of the NC handler's DAS object. The noteworthy feature
 * of this handler's DAS is that it lacks the specified structure that
 * provides an easy way to match DAS and DDS items. Instead this DAS is
 * flat.
 *
 * Because this handler produces a flat DAS, the nested structures (like
 * NCStructure) pass the entire top level AttrTable to each of their children
 * so that they can search for their attribute table. The default implementation
 * of this method would find the Structure's table and pass only that to the
 * children since it 'knows' that their tables would all be found within it.
 *
 * @param at An AttrTable for the entire DAS. Search this for attributes
 * by name.
 * @see NCSequence::transfer_attributes
 * @see NCGrid::transfer_attributes
 * @see NCArray::transfer_attributes
 */
void NCStructure::transfer_attributes(AttrTable *at)
{
    if (at) {
        Vars_iter var = var_begin();
        while (var != var_end()) {
            (*var)->transfer_attributes(at);
            var++;
        }
    }
}

/**
 * Custom version of the transform_to_dap4() method. This builds a NCStructure
 * and sets its parent to the container passed in as an argument. This has
 * to be specialized since the version provided by libdap calls 'new Strucutre(...)'
 * and not ptr_duplicate() since the latter will not transform the variables
 * held in this Structure (Sequence has this issue in general as well, but it does
 * not matter for this handler since netCDF files never hold sequence data).
 *
 * @param root Get stuff like dimensions from this Group
 * @param container The new variable will be part of this container (caller adds).
 * @return The new DAP4-ready variable.
 */
BaseType *
NCStructure::transform_to_dap4(D4Group *root, Constructor *container)
{
	Structure *dest = new NCStructure(name(), dataset());

	Constructor::transform_to_dap4(root, dest);
	dest->set_parent(container);

	return dest;
}

void NCStructure::do_structure_read(int ncid, int varid, nc_type datatype,
        vector<char> &values, bool has_values, int values_offset)
{
#if NETCDF_VERSION >= 4
  if (is_user_defined_type(ncid, datatype)) {
        char type_name[NC_MAX_NAME+1];
        size_t size;
        nc_type base_type;
        size_t nfields;
        int class_type;
        int errstat = nc_inq_user_type(ncid, datatype, type_name, &size, &base_type, &nfields, &class_type);
        if (errstat != NC_NOERR)
            throw InternalErr(__FILE__, __LINE__, "Could not get information about a user-defined type (" + long_to_string(errstat) + ").");

        switch (class_type) {
            case NC_COMPOUND: {
                if (!has_values) {
                    values.resize(size);
                    int errstat = nc_get_var(ncid, varid, &values[0] /*&values[0]*/);
                    if (errstat != NC_NOERR)
                        throw Error(errstat, string("Could not get the value for variable '") + name() + string("'"));
                    has_values = true;
                }

                for (size_t i = 0; i < nfields; ++i) {
                    char field_name[NC_MAX_NAME+1];
                    nc_type field_typeid;
                    size_t field_offset;
                    int field_ndims;
                    nc_inq_compound_field(ncid, datatype, i, field_name, &field_offset, &field_typeid, &field_ndims, 0);
                    if (is_user_defined_type(ncid, field_typeid)) {
                        // Interior user defined types have names, but not field_names
                        // so use the type name as the field name (matches the
                        // behavior of the ncdds.cc code).
                        nc_inq_compound_name(ncid, field_typeid, field_name);
                        NCStructure &ncs = dynamic_cast<NCStructure&>(*var(field_name));
                        ncs.do_structure_read(ncid, varid, field_typeid, values, has_values, field_offset + values_offset);
                    }
                    else if (var(field_name)->is_vector_type()) {
                        // Because the netcdf api reads data 'atomically' from
                        // compounds, this call works for both cardinal and
                        // array variables.
                        NCArray &child_array = dynamic_cast<NCArray&>(*var(field_name));
                        vector<size_t> cor(field_ndims);
                        vector<size_t> edg(field_ndims);
                        vector<ptrdiff_t> step(field_ndims);
                        bool has_stride;
                        long nels = child_array.format_constraint(&cor[0], &step[0], &edg[0], &has_stride);
                        child_array.do_array_read(ncid, varid, field_typeid,
                                values, has_values, field_offset + values_offset,
                                nels, &cor[0], &edg[0], &step[0], has_stride);
                    }
                    else if (var(field_name)->is_simple_type()) {
                        var(field_name)->val2buf(&values[0]  + field_offset + values_offset);
                    }
                    else {
                        throw InternalErr(__FILE__, __LINE__, "Expecting a netcdf user defined type or an array or a scalar.");
                    }

                    var(field_name)->set_read_p(true);
                }
                break;
            }

            case NC_VLEN:
                cerr << "in build_user_defined; found a vlen." << endl;
                break;
            case NC_OPAQUE:
                cerr << "in build_user_defined; found a opaque." << endl;
                break;
            case NC_ENUM:
                cerr << "in build_user_defined; found a enum." << endl;
                break;
            default:
                throw InternalErr(__FILE__, __LINE__, "Expected one of NC_COMPOUND, NC_VLEN, NC_OPAQUE or NC_ENUM");
        }
    }
    else
        throw InternalErr(__FILE__, __LINE__, "Found a DAP Structure bound to a non-user-defined type in the netcdf file " + dataset());
#else
        throw InternalErr(__FILE__, __LINE__, "Found a DAP Structure bound to a non-user-defined type in the netcdf file " + dataset());
#endif
}

bool NCStructure::read()
{
    if (read_p()) // nothing to do
        return true;

    int ncid;
    int errstat = nc_open(dataset().c_str(), NC_NOWRITE, &ncid); /* netCDF id */
    if (errstat != NC_NOERR)
        throw Error(errstat, "Could not open the dataset's file (" + dataset() + ")");

    int varid; /* variable Id */
    errstat = nc_inq_varid(ncid, name().c_str(), &varid);
    if (errstat != NC_NOERR)
        throw InternalErr(__FILE__, __LINE__, "Could not get variable ID for: " + name() + ". (error: " + long_to_string(errstat) + ").");

    nc_type datatype; /* variable data type */
    errstat = nc_inq_vartype(ncid, varid, &datatype);
    if (errstat != NC_NOERR)
        throw Error(errstat, "Could not read data type information about : " + name() + ". (error: " + long_to_string(errstat) + ").");

    // For Compound types, netcdf's nc_get_var() reads all of the structure's
    // values in one shot, including values for nested structures. Pass the
    // (reference to the) space for these in at the start of what may be a
    // series of recursive calls.
    vector<char> values;
    do_structure_read(ncid, varid, datatype, values, false /*has_values*/, 0 /*values_offset*/);

    set_read_p(true);

    if (nc_close(ncid) != NC_NOERR)
        throw InternalErr(__FILE__, __LINE__, "Could not close the dataset!");

    return true;
}


