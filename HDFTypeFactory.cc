
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of HDF_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2002,2003 OPeNDAP, IHDF.
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
// License along with this software; if not, write to the Free Software
// Foundation, IHDF., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, IHDF. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <string>

#include "HDFByte.h"
#include "HDFInt16.h"
#include "HDFUInt16.h"
#include "HDFInt32.h"
#include "HDFUInt32.h"
#include "HDFFloat32.h"
#include "HDFFloat64.h"
#include "HDFStr.h"
#include "HDFUrl.h"
#include "HDFArray.h"
#include "HDFStructure.h"
#include "HDFSequence.h"
#include "HDFGrid.h"
#include "HDFTypeFactory.h"
#include "debug.h"
#include <BESDebug.h>

Byte *HDFTypeFactory::NewByte(const string & n) const
{
    return new HDFByte(n, d_filename);
}

Int16 *HDFTypeFactory::NewInt16(const string & n) const
{
    return new HDFInt16(n, d_filename);
}

UInt16 *HDFTypeFactory::NewUInt16(const string & n) const
{
    return new HDFUInt16(n, d_filename);
}

Int32 *HDFTypeFactory::NewInt32(const string & n) const
{
    BESDEBUG("h4", "Inside HDFTypeFactory::NewInt32" << endl);
    return new HDFInt32(n, d_filename);
}

UInt32 *HDFTypeFactory::NewUInt32(const string & n) const
{
    return new HDFUInt32(n, d_filename);
}

Float32 *HDFTypeFactory::NewFloat32(const string & n) const
{
    return new HDFFloat32(n, d_filename);
}

Float64 *HDFTypeFactory::NewFloat64(const string & n) const
{
    return new HDFFloat64(n, d_filename);
}

Str *HDFTypeFactory::NewStr(const string & n) const
{
    return new HDFStr(n, d_filename);
}

Url *HDFTypeFactory::NewUrl(const string & n) const
{
    return new HDFUrl(n, d_filename);
}

Array *HDFTypeFactory::NewArray(const string & n, BaseType * v) const
{
    return new HDFArray(n, d_filename, v);
}

Structure *HDFTypeFactory::NewStructure(const string & n) const
{
    return new HDFStructure(n, d_filename);
}

Sequence *HDFTypeFactory::NewSequence(const string & n) const
{
    BESDEBUG("h4", "Inside HDFTypeFactory::NewSequence" << endl);
    return new HDFSequence(n, d_filename);
}

Grid *HDFTypeFactory::NewGrid(const string & n) const
{
    return new HDFGrid(n, d_filename);
}
