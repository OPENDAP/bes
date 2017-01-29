
// -*- mode: c++; c-basic-offset:4 -*-

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

#ifndef ascii_output_factory_h
#define ascii_output_factory_h

#include <string>

#include "BaseTypeFactory.h"

class AsciiByte;
class AsciiInt16;
class AsciiUInt16;
class AsciiInt32;
class AsciiUInt32;
class AsciiFloat32;
class AsciiFloat64;
class AsciiStr;
class AsciiUrl;
class AsciiArray;
class AsciiStructure;
class AsciiSequence;
class AsciiGrid;

/** A factory for the AsciiByte, ..., AsciiGrid types.
    @author James Gallagher */
class AsciiOutputFactory : public BaseTypeFactory {
public:
    AsciiOutputFactory() {} 
    virtual ~AsciiOutputFactory() {}

    virtual Byte *NewByte(const string &n = "") const;
    virtual Int16 *NewInt16(const string &n = "") const;
    virtual UInt16 *NewUInt16(const string &n = "") const;
    virtual Int32 *NewInt32(const string &n = "") const;
    virtual UInt32 *NewUInt32(const string &n = "") const;
    virtual Float32 *NewFloat32(const string &n = "") const;
    virtual Float64 *NewFloat64(const string &n = "") const;

    virtual Str *NewStr(const string &n = "") const;
    virtual Url *NewUrl(const string &n = "") const;

    virtual Array *NewArray(const string &n = "", BaseType *v = 0) const;
    virtual Structure *NewStructure(const string &n = "") const;
    virtual Sequence *NewSequence(const string &n = "") const;
    virtual Grid *NewGrid(const string &n = "") const;
};

#endif // ascii_output_factory_h
