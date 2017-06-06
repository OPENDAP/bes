
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

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


#include <string>

#include "XDByte.h"
#include "XDInt16.h"
#include "XDUInt16.h"
#include "XDInt32.h"
#include "XDUInt32.h"
#include "XDFloat32.h"
#include "XDFloat64.h"
#include "XDStr.h"
#include "XDUrl.h"
#include "XDArray.h"
#include "XDStructure.h"
#include "XDSequence.h"
#include "XDGrid.h"

#include "BaseTypeFactory.h"
#include "XDOutputFactory.h"

#include "debug.h"

Byte *
XDOutputFactory::NewByte(const string &n ) const 
{ 
    return new XDByte(n);
}

Int16 *
XDOutputFactory::NewInt16(const string &n ) const 
{ 
    return new XDInt16(n); 
}

UInt16 *
XDOutputFactory::NewUInt16(const string &n ) const 
{ 
    return new XDUInt16(n);
}

Int32 *
XDOutputFactory::NewInt32(const string &n ) const 
{ 
    DBG(cerr << "Inside XDOutputFactory::NewInt32" << endl);
    return new XDInt32(n);
}

UInt32 *
XDOutputFactory::NewUInt32(const string &n ) const 
{ 
    return new XDUInt32(n);
}

Float32 *
XDOutputFactory::NewFloat32(const string &n ) const 
{ 
    return new XDFloat32(n);
}

Float64 *
XDOutputFactory::NewFloat64(const string &n ) const 
{ 
    return new XDFloat64(n);
}

Str *
XDOutputFactory::NewStr(const string &n ) const 
{ 
    return new XDStr(n);
}

Url *
XDOutputFactory::NewUrl(const string &n ) const 
{ 
    return new XDUrl(n);
}

Array *
XDOutputFactory::NewArray(const string &n , BaseType *v) const 
{ 
    return new XDArray(n, v);
}

Structure *
XDOutputFactory::NewStructure(const string &n ) const 
{ 
    return new XDStructure(n);
}

Sequence *
XDOutputFactory::NewSequence(const string &n ) const 
{
    DBG(cerr << "Inside XDOutputFactory::NewSequence" << endl);
    return new XDSequence(n);
}

Grid *
XDOutputFactory::NewGrid(const string &n ) const 
{ 
    return new XDGrid(n);
}
