
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


#include "FFInt16.h"
#include "util_ff.h"

#include <cstring>

extern long BufPtr;
extern char *BufVal;

FFInt16::FFInt16(const string &n, const string &d) : Int16(n, d)
{
}

BaseType *
FFInt16::ptr_duplicate(){

    return new FFInt16(*this);
}

bool
FFInt16::read()
{
    if (read_p()) // nothing to do
		return true;

    if (BufVal) { // data in cache
		char * ptr = BufVal+BufPtr;
	
		dods_int16 align;
		if (width() > sizeof(align))
			throw InternalErr(__FILE__, __LINE__, "Int16 size.");		
		
		memcpy((void*)&align, (void *)ptr, width());
	
		val2buf((void *) &align);
		set_read_p(true);
	
	#ifdef TEST
		dods_int16 *tmo;
		tmo = (dods_int16 *)ptr;
		printf("\n%ld offset=%ld\n",*tmo,BufPtr);
	#endif 
	
		BufPtr += width();

		return true;
    }

	return false;
}

// $Log: FFInt16.cc,v $
// Revision 1.2  2000/10/11 19:37:56  jimg
// Moved the CVS log entries to the end of files.
// Changed the definition of the read method to match the dap library.
// Added exception handling.
// Added exceptions to the read methods.
//
// Revision 1.1  1999/05/19 17:52:41  jimg
// Added
//
