
// -*- mode: c++; c-basic-offset:4 -*-

// Copyright (c) 2010 OPeNDAP, Inc.
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

#ifndef xd_output_factory_h
#define xd_output_factory_h

#include <string>

#include "BaseTypeFactory.h"

class XDByte;
class XDInt16;
class XDUInt16;
class XDInt32;
class XDUInt32;
class XDFloat32;
class XDFloat64;
class XDStr;
class XDUrl;
class XDArray;
class XDStructure;
class XDSequence;
class XDGrid;

/** A factory for the XDByte, ..., XDGrid types.
    @author James Gallagher */
class XDOutputFactory : public libdap::BaseTypeFactory {
public:
    XDOutputFactory() {}
    virtual ~XDOutputFactory() {}

    virtual libdap::Byte *NewByte(const string &n = "") const;
    virtual libdap::Int16 *NewInt16(const string &n = "") const;
    virtual libdap::UInt16 *NewUInt16(const string &n = "") const;
    virtual libdap::Int32 *NewInt32(const string &n = "") const;
    virtual libdap::UInt32 *NewUInt32(const string &n = "") const;
    virtual libdap::Float32 *NewFloat32(const string &n = "") const;
    virtual libdap::Float64 *NewFloat64(const string &n = "") const;

    virtual libdap::Str *NewStr(const string &n = "") const;
    virtual libdap::Url *NewUrl(const string &n = "") const;

    virtual libdap::Array *NewArray(const string &n = "", libdap::BaseType *v = 0) const;
    virtual libdap::Structure *NewStructure(const string &n = "") const;
    virtual libdap::Sequence *NewSequence(const string &n = "") const;
    virtual libdap::Grid *NewGrid(const string &n = "") const;
};

#endif // xd_output_factory_h
