
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

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors: James Gallagher

#include "config_ff.h"

static char rcsid[] not_used ={"$Id$"};

#include "FFFloat32.h"
#include "util_ff.h"
#include "util.h"

#include <cstring>

extern long BufPtr;
extern char *BufVal;

FFFloat32::FFFloat32(const string &n, const string &d) : Float32(n, d)
{
}

BaseType *
FFFloat32::ptr_duplicate()
{
    return new FFFloat32(*this); // Copy ctor calls duplicate to do the work
}
 
bool
FFFloat32::read()
{
    if (read_p()) // nothing to do
		return true;
  
    if(BufVal) { // data in cache
		char *ptr = BufVal+BufPtr;
	
		dods_float32 align;
		if (width() > sizeof(align))
			throw InternalErr(__FILE__, __LINE__, "Float32 size.");		
		memcpy((void*)&align, (void *)ptr, width());
	
		val2buf((void *) &align);
		set_read_p(true);
	
		BufPtr += width();

		return true;
    }
    
	return false;
}
