
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef _dmrpp_type_factory_h
#define _dmrpp_type_factory_h

#include <string>
#include <memory>

#include <libdap/D4BaseTypeFactory.h>

namespace dmrpp {

class DMZ;
/**
 * A factory for the DAP4 DmrppByte, ...,  types.
 *
 * @author James Gallagher
 */
class DmrppTypeFactory : public libdap::D4BaseTypeFactory {
    std::shared_ptr<DMZ> d_dmz;

public:
    DmrppTypeFactory() = default;
    DmrppTypeFactory(std::shared_ptr<DMZ> dmz) : d_dmz(dmz) { }

    virtual ~DmrppTypeFactory() = default;

    virtual BaseTypeFactory *ptr_duplicate() const { return new DmrppTypeFactory; }

    virtual libdap::BaseType *NewVariable(libdap::Type t, const std::string &name) const;

    virtual libdap::Byte *NewByte(const std::string &n = "") const;

    // New for DAP4
    virtual libdap::Int8 *NewInt8(const std::string &n = "") const;
    virtual libdap::Byte *NewUInt8(const std::string &n = "") const;
    virtual libdap::Byte *NewChar(const std::string &n = "") const;

    virtual libdap::Int16 *NewInt16(const std::string &n = "") const;
    virtual libdap::UInt16 *NewUInt16(const std::string &n = "") const;
    virtual libdap::Int32 *NewInt32(const std::string &n = "") const;
    virtual libdap::UInt32 *NewUInt32(const std::string &n = "") const;

    // New for DAP4
    virtual libdap::Int64 *NewInt64(const std::string &n = "") const;
    virtual libdap::UInt64 *NewUInt64(const std::string &n = "") const;

    virtual libdap::Float32 *NewFloat32(const std::string &n = "") const;
    virtual libdap::Float64 *NewFloat64(const std::string &n = "") const;

    virtual libdap::D4Enum *NewEnum(const std::string &n = "", libdap::Type type = libdap::dods_null_c) const;

    virtual libdap::Str *NewStr(const std::string &n = "") const;
    virtual libdap::Url *NewUrl(const std::string &n = "") const;
    virtual libdap::Url *NewURL(const std::string &n = "") const;

    virtual libdap::D4Opaque *NewOpaque(const std::string &n = "") const;

    virtual libdap::Array *NewArray(const std::string &n = "", libdap::BaseType *v = 0) const;

    virtual libdap::Structure *NewStructure(const std::string &n = "") const;
    virtual libdap::D4Sequence *NewD4Sequence(const std::string &n = "") const;

    virtual libdap::D4Group *NewGroup(const std::string &n = "") const;
};

} // namespace dmrpp

#endif // _dmrpp_type_factory_h
