// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of asciival, software which can return an XML data
// representation of the data read from a DAP server.

// Copyright (c) 2010 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// implementation for XDGrid. See XDByte.
//
// 3/12/98 jhrg

#include "config.h"

#include <iostream>
#include <string>

using namespace std;

#include <InternalErr.h>

#include <BESDebug.h>

// #define DODS_DEBUG

#include "XDGrid.h"
#include "XDArray.h"
#include "debug.h"
#include "get_xml_data.h"

using namespace xml_data;

BaseType *
XDGrid::ptr_duplicate()
{
    return new XDGrid(*this);
}

XDGrid::XDGrid(const string &n) :
    Grid(n)
{
}

XDGrid::XDGrid(Grid *grid) :
    Grid(grid->name()), XDOutput(grid)
{
    BaseType *bt = basetype_to_xd(grid->array_var());
    add_var(bt, libdap::array);
    // add_var makes a copy of the base type passed to it, so delete it here
    delete bt;
    bt = 0;

    Grid::Map_iter i = grid->map_begin();
    Grid::Map_iter e = grid->map_end();
    while (i != e) {
        bt = basetype_to_xd(*i);
        add_var(bt, maps);
        // add_var makes a copy of the base type passed to it, so delete it here
        delete bt;
        ++i;
    }

    BaseType::set_send_p(grid->send_p());
}

XDGrid::~XDGrid()
{
}

void XDGrid::print_xml_data(XMLWriter *writer, bool show_type)
{
    // General rule: If everything in the Grid (all maps plus the array)
    // is projected, then print as a Grid, else print as if the Gird is a
    // Structure.
    if (projection_yields_grid())
	start_xml_declaration(writer, "Grid"); // Start grid element
    else
	start_xml_declaration(writer, "Structure"); // Start structure element

    // Print the array and the maps, but use <Array> and not <Map>
    if (array_var()->send_p()) {
	dynamic_cast<XDArray&> (*array_var()).print_xml_data(writer, show_type);
    }

    Map_iter m = map_begin();
    while (m != map_end()) {
	if ((*m)->send_p()) {
	    if (projection_yields_grid())
		dynamic_cast<XDArray&> (**m).print_xml_map_data(writer, show_type);
	    else
		dynamic_cast<XDArray&> (**m).print_xml_data(writer, show_type);
	}
	++m;
    }

    // End the structure element
    end_xml_declaration(writer);
}
