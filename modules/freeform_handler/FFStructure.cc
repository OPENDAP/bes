
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of ff_handler a FreeForm API handler for the OPeNDAP
// DAP2 data server.

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
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 1997-99
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors: reza (Reza Nekovei)

// FreeFrom sub-class implementation for FFByte,...FFGrid.
// The files are patterned after the subcalssing examples
// Test<type>.c,h files.
//
// ReZa 6/18/97

#include "config_ff.h"

static char rcsid[] not_used = {"$Id$"};


#include <string>

#include "InternalErr.h"
#include "FFStructure.h"

BaseType *
FFStructure::ptr_duplicate()
{
    return new FFStructure(*this);
}

FFStructure::FFStructure(const string &n, const string &d) : Structure(n, d)
{
}

FFStructure::~FFStructure()
{
}

bool
FFStructure::read()
{
    throw InternalErr(__FILE__, __LINE__, "Unimplemented read method called.");
}

void FFStructure::transfer_attributes(AttrTable *)
{
    throw InternalErr(__FILE__, __LINE__, "Unimplemented transfer_attribute() method called.");
}


