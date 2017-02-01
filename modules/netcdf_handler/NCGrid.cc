
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

#include <sstream>

#include <netcdf.h>

#include <Error.h>
#include <InternalErr.h>
#include <util.h>

#include "NCGrid.h"
#include <debug.h>

// protected


BaseType *
NCGrid::ptr_duplicate()
{
    return new NCGrid(*this);
}

// public

NCGrid::NCGrid(const string &n, const string &d) : Grid(n, d)
{
}

NCGrid::NCGrid(const NCGrid &rhs) : Grid(rhs)
{
}


NCGrid::~NCGrid()
{
}

NCGrid &
NCGrid::operator=(const NCGrid &rhs)
{
    if (this == &rhs)
        return *this;

    dynamic_cast<NCGrid&>(*this) = rhs;


    return *this;
}


bool
NCGrid::read()
{
    DBG(cerr << "In NCGrid::read" << endl);

    if (read_p()) // nothing to do
        return true;

    DBG(cerr << "In NCGrid, reading components for " << name() << endl);

    // read array elements
    if (array_var()->send_p() || array_var()->is_in_selection())
	   array_var()->read();

    // read maps elements
    for (Map_iter p = map_begin(); p != map_end(); ++p)
    	if ((*p)->send_p() || (*p)->is_in_selection())
    	    (*p)->read();

    set_read_p(true);

    return true;
}

/**
 * @see NCStructure
 * @param at
 */
void NCGrid::transfer_attributes(AttrTable *at)
{
    if (at) {
	array_var()->transfer_attributes(at);

	Map_iter map = map_begin();
	while (map != map_end()) {
	    (*map)->transfer_attributes(at);
	    map++;
	}
    }
}
