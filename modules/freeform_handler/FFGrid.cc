
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

// (c) COPYRIGHT URI/MIT 1997-98
// Please read the full copyright statement in the file COPYRIGHT.  
//
// Authors: reza (Reza Nekovei)

// FreeFrom sub-class implementation for FFByte,...FFGrid.
// The files are patterned after the subcalssing examples 
// Test<type>.c,h files.
//
// ReZa 6/18/97

#include "config_ff.h"

static char rcsid[] not_used ={"$Id$"};


#include "FFGrid.h"
#include "util.h"

// protected

BaseType *
FFGrid::ptr_duplicate()
{
    return new FFGrid(*this);
}

// public

FFGrid::FFGrid(const string &n, const string &d) : Grid(n, d)
{
}

FFGrid::~FFGrid()
{
}

bool
FFGrid::read()
{
    if (read_p()) // nothing to do
        return true;

    // read array elements
    array_var()->read();

    // read maps elements
    Grid::Map_iter i = map_begin();
    while (i != map_end())
        (*i++)->read();

    set_read_p(true);

    return true;
}

/**
 * @see FFStructure
 * @param at
 */
void FFGrid::transfer_attributes(AttrTable *at)
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

// $Log: FFGrid.cc,v $
// Revision 1.9  2003/02/10 23:01:52  jimg
// Merged with 3.2.5
//
// Revision 1.8.2.1  2002/12/18 23:30:42  pwest
// gcc3.2 compile corrections, mainly regarding the using statement
//
// Revision 1.8  2000/10/11 19:37:56  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.7  1999/05/27 17:02:22  jimg
// Merge with alpha-3-0-0
//
// Revision 1.6.2.1  1999/05/20 21:41:07  edavis
// Fix spelling of COPYRIGHT and remove some #if 0 stuff.
//
// Revision 1.6  1999/05/04 02:55:36  jimg
// Merge with no-gnu
//
// Revision 1.5  1998/08/14 19:41:23  jimg
// Removed unused bool variable.
//
// Revision 1.4  1998/08/13 20:24:26  jimg
// Fixed read mfunc semantics
//
// Revision 1.3  1998/04/21 17:13:50  jimg
// Fixes for warnings, etc
//
// Revision 1.2  1998/04/16 18:11:08  jimg
// Sequence support added by Reza
