
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

#include <Byte.h>
#include <Int16.h>
#include <UInt16.h>
#include <Int32.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>
#include <Url.h>
#include <Array.h>
#include <Structure.h>
#include <ReadSequence.h>
#include <Grid.h>
#include <debug.h>

#include "ReadTypeFactory.h"

Byte *
ReadTypeFactory::NewByte(const string &n) const
{
    return new Byte(n);
}

Int16 *
ReadTypeFactory::NewInt16(const string &n) const
{
    return new Int16(n);
}

UInt16 *
ReadTypeFactory::NewUInt16(const string &n) const
{
    return new UInt16(n);
}

Int32 *
ReadTypeFactory::NewInt32(const string &n) const
{
    DBG(cerr << "Inside ReadTypeFactory::NewInt32" << endl);
    return new Int32(n);
}

UInt32 *
ReadTypeFactory::NewUInt32(const string &n) const
{
    return new UInt32(n);
}

Float32 *
ReadTypeFactory::NewFloat32(const string &n) const
{
    return new Float32(n);
}

Float64 *
ReadTypeFactory::NewFloat64(const string &n) const
{
    return new Float64(n);
}

Str *
ReadTypeFactory::NewStr(const string &n) const
{
    return new Str(n);
}

Url *
ReadTypeFactory::NewUrl(const string &n) const
{
    return new Url(n);
}

Array *
ReadTypeFactory::NewArray(const string &n , BaseType *v) const
{
    return new Array(n, v);
}

Structure *
ReadTypeFactory::NewStructure(const string &n) const
{
    return new Structure(n);
}

Sequence *
ReadTypeFactory::NewSequence(const string &n) const
{
    DBG(cerr << "Inside ReadTypeFactory::NewSequence" << endl);
    return new ReadSequence(n);
}

Grid *
ReadTypeFactory::NewGrid(const string &n) const
{
    return new Grid(n);
}

