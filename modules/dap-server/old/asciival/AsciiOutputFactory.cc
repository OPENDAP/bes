
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2005 OPeNDAP, Inc.
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


#include <string>

#include "AsciiByte.h"
#include "AsciiInt16.h"
#include "AsciiUInt16.h"
#include "AsciiInt32.h"
#include "AsciiUInt32.h"
#include "AsciiFloat32.h"
#include "AsciiFloat64.h"
#include "AsciiStr.h"
#include "AsciiUrl.h"
#include "AsciiArray.h"
#include "AsciiStructure.h"
#include "AsciiSequence.h"
#include "AsciiGrid.h"

#include "BaseTypeFactory.h"
#include "AsciiOutputFactory.h"

#include "debug.h"

Byte *
AsciiOutputFactory::NewByte(const string &n ) const 
{ 
    return new AsciiByte(n);
}

Int16 *
AsciiOutputFactory::NewInt16(const string &n ) const 
{ 
    return new AsciiInt16(n); 
}

UInt16 *
AsciiOutputFactory::NewUInt16(const string &n ) const 
{ 
    return new AsciiUInt16(n);
}

Int32 *
AsciiOutputFactory::NewInt32(const string &n ) const 
{ 
    DBG(cerr << "Inside AsciiOutputFactory::NewInt32" << endl);
    return new AsciiInt32(n);
}

UInt32 *
AsciiOutputFactory::NewUInt32(const string &n ) const 
{ 
    return new AsciiUInt32(n);
}

Float32 *
AsciiOutputFactory::NewFloat32(const string &n ) const 
{ 
    return new AsciiFloat32(n);
}

Float64 *
AsciiOutputFactory::NewFloat64(const string &n ) const 
{ 
    return new AsciiFloat64(n);
}

Str *
AsciiOutputFactory::NewStr(const string &n ) const 
{ 
    return new AsciiStr(n);
}

Url *
AsciiOutputFactory::NewUrl(const string &n ) const 
{ 
    return new AsciiUrl(n);
}

Array *
AsciiOutputFactory::NewArray(const string &n , BaseType *v) const 
{ 
    return new AsciiArray(n, v);
}

Structure *
AsciiOutputFactory::NewStructure(const string &n ) const 
{ 
    return new AsciiStructure(n);
}

Sequence *
AsciiOutputFactory::NewSequence(const string &n ) const 
{
    DBG(cerr << "Inside AsciiOutputFactory::NewSequence" << endl);
    return new AsciiSequence(n);
}

Grid *
AsciiOutputFactory::NewGrid(const string &n ) const 
{ 
    return new AsciiGrid(n);
}
