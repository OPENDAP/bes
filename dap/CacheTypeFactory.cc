
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

#if 0
#include "TestByte.h"
#include "TestInt16.h"
#include "TestUInt16.h"
#include "TestInt32.h"
#include "TestUInt32.h"
#include "TestFloat32.h"
#include "TestFloat64.h"
#include "TestStr.h"
#include "TestUrl.h"
#include "TestArray.h"
#include "TestStructure.h"
#include "TestSequence.h"
#include "TestGrid.h"
#endif

#include "CachedSequence.h"

#include "CacheTypeFactory.h"
#include "debug.h"

using namespace std;
using namespace libdap;

#if 0
Byte *
TestTypeFactory::NewByte(const string &n ) const 
{ 
    return new TestByte(n);
}

Int16 *
TestTypeFactory::NewInt16(const string &n ) const 
{ 
    return new TestInt16(n); 
}

UInt16 *
TestTypeFactory::NewUInt16(const string &n ) const 
{ 
    return new TestUInt16(n);
}

Int32 *
TestTypeFactory::NewInt32(const string &n ) const 
{ 
    DBG(cerr << "Inside TestTypeFactory::NewInt32" << endl);
    return new TestInt32(n);
}

UInt32 *
TestTypeFactory::NewUInt32(const string &n ) const 
{ 
    return new TestUInt32(n);
}

Float32 *
TestTypeFactory::NewFloat32(const string &n ) const 
{ 
    return new TestFloat32(n);
}

Float64 *
TestTypeFactory::NewFloat64(const string &n ) const 
{ 
    return new TestFloat64(n);
}

Str *
TestTypeFactory::NewStr(const string &n ) const 
{ 
    return new TestStr(n);
}

Url *
TestTypeFactory::NewUrl(const string &n ) const 
{ 
    return new TestUrl(n);
}

Array *
TestTypeFactory::NewArray(const string &n , BaseType *v) const 
{ 
    return new TestArray(n, v);
}

Structure *
TestTypeFactory::NewStructure(const string &n ) const 
{ 
    return new TestStructure(n);
}
#endif

Sequence *
CacheTypeFactory::NewSequence(const string &n ) const
{
    DBGN(cerr << __PRETTY_FUNCTION__ << endl);
    return new CachedSequence(n);
}

#if 0
Grid *
TestTypeFactory::NewGrid(const string &n ) const 
{ 
    return new TestGrid(n);
}
#endif
