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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// #define DODS_DEBUG 
#include <string>

#include "HDF5Byte.h"
#include "HDF5Int16.h"
#include "HDF5UInt16.h"
#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Str.h"
#include "HDF5Url.h"
#include "HDF5Array.h"
#include "HDF5Structure.h"
#include "HDF5Sequence.h"
#include "HDF5Grid.h"
#include "HDF5GridEOS.h"
#ifdef CF
#include "HDF5ArrayEOS.h"
#endif
#include "HDF5TypeFactory.h"
#include "debug.h"

Byte *HDF5TypeFactory::NewByte(const string & n) const
{
    return new HDF5Byte(n);
}

Int16 *HDF5TypeFactory::NewInt16(const string & n) const
{
    return new HDF5Int16(n);
}

UInt16 *HDF5TypeFactory::NewUInt16(const string & n) const
{
    return new HDF5UInt16(n);
}

Int32 *HDF5TypeFactory::NewInt32(const string & n) const
{
    DBG(cerr << "Inside HDF5TypeFactory::NewInt32" << endl);
    return new HDF5Int32(n);
}

UInt32 *HDF5TypeFactory::NewUInt32(const string & n) const
{
    return new HDF5UInt32(n);
}

Float32 *HDF5TypeFactory::NewFloat32(const string & n) const
{
    return new HDF5Float32(n);
}

Float64 *HDF5TypeFactory::NewFloat64(const string & n) const
{
    return new HDF5Float64(n);
}

Str *HDF5TypeFactory::NewStr(const string & n) const
{
    DBG(cerr << ">HDF5TypeFactory::NewStr()" << endl);
    return new HDF5Str(n);
}

Url *HDF5TypeFactory::NewUrl(const string & n) const
{
    return new HDF5Url(n);
}

Array *HDF5TypeFactory::NewArray(const string & n, BaseType * v) const
{
    return new HDF5Array(n, v);
}

Structure *HDF5TypeFactory::NewStructure(const string & n) const
{
    return new HDF5Structure(n);
}

Sequence *HDF5TypeFactory::NewSequence(const string & n) const
{
    DBG(cerr << "Inside HDF5TypeFactory::NewSequence" << endl);
    return new HDF5Sequence(n);
}

Grid *HDF5TypeFactory::NewGrid(const string & n) const
{
    return new HDF5Grid(n);
}

Grid *HDF5TypeFactory::NewGridEOS(const string & n) const
{
    return new HDF5GridEOS(n);
}

#ifdef CF
Array *HDF5TypeFactory::NewArrayEOS(const string & n, BaseType * v) const
{
    return new HDF5ArrayEOS(n, v);
}
#endif
