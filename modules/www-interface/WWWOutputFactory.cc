
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of www_int.

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
// 
// This is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
// more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.


#include <string>

#include "WWWByte.h"
#include "WWWInt16.h"
#include "WWWUInt16.h"
#include "WWWInt32.h"
#include "WWWUInt32.h"
#include "WWWFloat32.h"
#include "WWWFloat64.h"
#include "WWWStr.h"
#include "WWWUrl.h"
#include "WWWArray.h"
#include "WWWStructure.h"
#include "WWWSequence.h"
#include "WWWGrid.h"

#include <libdap/BaseTypeFactory.h>
#include "WWWOutputFactory.h"

#include <libdap/debug.h>

Byte *
WWWOutputFactory::NewByte(const string &n ) const 
{ 
    return new WWWByte(n);
}

Int16 *
WWWOutputFactory::NewInt16(const string &n ) const 
{ 
    return new WWWInt16(n); 
}

UInt16 *
WWWOutputFactory::NewUInt16(const string &n ) const 
{ 
    return new WWWUInt16(n);
}

Int32 *
WWWOutputFactory::NewInt32(const string &n ) const 
{ 
    DBG(cerr << "Inside WWWOutputFactory::NewInt32" << endl);
    return new WWWInt32(n);
}

UInt32 *
WWWOutputFactory::NewUInt32(const string &n ) const 
{ 
    return new WWWUInt32(n);
}

Float32 *
WWWOutputFactory::NewFloat32(const string &n ) const 
{ 
    return new WWWFloat32(n);
}

Float64 *
WWWOutputFactory::NewFloat64(const string &n ) const 
{ 
    return new WWWFloat64(n);
}

Str *
WWWOutputFactory::NewStr(const string &n ) const 
{ 
    return new WWWStr(n);
}

Url *
WWWOutputFactory::NewUrl(const string &n ) const 
{ 
    return new WWWUrl(n);
}

Array *
WWWOutputFactory::NewArray(const string &n , BaseType *v) const 
{ 
    return new WWWArray(n, v);
}

Structure *
WWWOutputFactory::NewStructure(const string &n ) const 
{ 
    return new WWWStructure(n);
}

Sequence *
WWWOutputFactory::NewSequence(const string &n ) const 
{
    DBG(cerr << "Inside WWWOutputFactory::NewSequence" << endl);
    return new WWWSequence(n);
}

Grid *
WWWOutputFactory::NewGrid(const string &n ) const 
{ 
    return new WWWGrid(n);
}
