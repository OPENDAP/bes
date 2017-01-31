
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

// (c) COPYRIGHT URI/MIT 1994-1996
// Please read the full copyright statement in the file COPYRIGHT.  
//
// Authors:
//      reza            Reza Nekovei (reza@intcomm.net)

// FreeFrom sub-class implementation for FFByte,...FFGrid.
// The files are patterned after the subcalssing examples 
// Test<type>.c,h files.
//
// ReZa 6/18/97

#ifndef _ffgrid_h
#define _ffgrid_h 1


#include "Grid.h"

using namespace libdap ;

class FFGrid: public Grid {
public:
    FFGrid(const string &n, const string &d);
    virtual ~FFGrid();
    
    virtual BaseType *ptr_duplicate();

    virtual bool read();

    virtual void transfer_attributes(AttrTable *at);
};

// $Log: FFGrid.h,v $
// Revision 1.7  2003/02/10 23:01:52  jimg
// Merged with 3.2.5
//
// Revision 1.6.2.1  2002/12/18 23:30:42  pwest
// gcc3.2 compile corrections, mainly regarding the using statement
//
// Revision 1.6  2000/10/11 19:37:56  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.5  1999/05/27 17:02:22  jimg
// Merge with alpha-3-0-0
//
// Revision 1.4.2.1  1999/05/20 21:41:12  edavis
// Fix spelling of COPYRIGHT and remove some #if 0 stuff.
//
// Revision 1.4  1999/05/04 02:55:36  jimg
// Merge with no-gnu
//
// Revision 1.3.12.1  1999/05/01 04:40:30  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.3  1998/04/21 17:13:51  jimg
// Fixes for warnings, etc
//
// Revision 1.2  1998/04/16 18:11:09  jimg
// Sequence support added by Reza

#endif





