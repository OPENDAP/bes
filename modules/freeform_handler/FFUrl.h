
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

// Freeform sub-class implementation for FFByte,...FFGrid.
// The files are patterned after the subcalssing examples 
// Test<type>.c,h files.
//
// ReZa 6/18/97

#ifndef _ffurl_h
#define _ffurl_h 1


#include "Url.h"

using namespace libdap ;

//extern Url * NewUrl(const string &n);

class FFUrl: public Url {
public:
    FFUrl(const string &n, const string &d);
    virtual ~FFUrl() {}

    virtual BaseType *ptr_duplicate();
    
    virtual bool read();
};

// $Log: FFUrl.h,v $
// Revision 1.6  2000/10/11 19:37:56  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.5  1999/05/27 17:02:23  jimg
// Merge with alpha-3-0-0
//
// Revision 1.4.2.1  1999/05/20 21:42:40  edavis
// Fix spelling of COPYRIGHT and remove some #if 0 stuff.
//
// Revision 1.4  1999/05/04 02:55:37  jimg
// Merge with no-gnu
//
// Revision 1.3.12.1  1999/05/01 04:40:31  brent
// converted old String.h to the new std C++ <string> code
//
// Revision 1.3  1998/04/21 17:14:05  jimg
// Fixes for warnings, etc
//
// Revision 1.2  1998/04/16 18:11:22  jimg
// Sequence support added by Reza

#endif

