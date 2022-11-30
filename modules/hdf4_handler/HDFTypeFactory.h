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
#include <libdap/BaseTypeFactory.h>

using std::string ;

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

    @author James Gallagher, Kent Yang
    @see DDS */
class HDFTypeFactory:public libdap::BaseTypeFactory {
private:
    string d_filename ;
    HDFTypeFactory() = default;
public:
    explicit HDFTypeFactory( const string &filename ) : d_filename( filename ) {} 
    ~HDFTypeFactory() override = default;

    libdap::Byte *NewByte(const string & n = "") const override;
    libdap::Int16 *NewInt16(const string & n = "") const override;
    libdap::UInt16 *NewUInt16(const string & n = "") const override;
    libdap::Int32 *NewInt32(const string & n = "") const override;
    libdap::UInt32 *NewUInt32(const string & n = "") const override;
    libdap::Float32 *NewFloat32(const string & n = "") const override;
    libdap::Float64 *NewFloat64(const string & n = "") const override;

    libdap::Str *NewStr(const string & n = "") const override;
    libdap::Url *NewUrl(const string & n = "") const override;

    libdap::Array *NewArray(const string & n = "", libdap::BaseType * v = nullptr) const override;
    libdap::Structure *NewStructure(const string & n = "") const override;
    libdap::Sequence *NewSequence(const string & n = "") const override;
    libdap::Grid *NewGrid(const string & n = "") const override;
};

#endif

