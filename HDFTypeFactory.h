// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of nc_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef nc_type_factory_h
#define nc_type_factory_h

#include <string>

using std::string ;

#include "BaseTypeFactory.h"

using namespace libdap;

// Class declarations; Make sure to include the corresponding headers in the
// implementation file.

class HDFByte;
class HDFInt16;
class HDFUInt16;
class HDFInt32;
class HDFUInt32;
class HDFFloat32;
class HDFFloat64;
class HDFStr;
class HDFUrl;
class HDFArray;
class HDFStructure;
class HDFSequence;
class HDFGrid;

/** A factory for the netCDF client library types.

    @author James Gallagher
    @see DDS */
class HDFTypeFactory:public BaseTypeFactory {
private:
    string d_filename ;
    HDFTypeFactory() {}
public:
    HDFTypeFactory( const string &filename ) : d_filename( filename ) {} 
    virtual ~HDFTypeFactory() {}

    virtual Byte *NewByte(const string & n = "") const;
    virtual Int16 *NewInt16(const string & n = "") const;
    virtual UInt16 *NewUInt16(const string & n = "") const;
    virtual Int32 *NewInt32(const string & n = "") const;
    virtual UInt32 *NewUInt32(const string & n = "") const;
    virtual Float32 *NewFloat32(const string & n = "") const;
    virtual Float64 *NewFloat64(const string & n = "") const;

    virtual Str *NewStr(const string & n = "") const;
    virtual Url *NewUrl(const string & n = "") const;

    virtual Array *NewArray(const string & n = "", BaseType * v = 0) const;
    virtual Structure *NewStructure(const string & n = "") const;
    virtual Sequence *NewSequence(const string & n = "") const;
    virtual Grid *NewGrid(const string & n = "") const;
};

#endif

