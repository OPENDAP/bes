
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef _dmrpp_d4enum_h
#define _dmrpp_d4enum_h 1

#include <string>

#include <libdap/D4Enum.h>
#include "DmrppCommon.h"

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

class DmrppD4Enum: public libdap::D4Enum, public DmrppCommon {

public:
    DmrppD4Enum(const std::string &n, const string &enum_type) : libdap::D4Enum(n, enum_type), DmrppCommon() { }
    DmrppD4Enum(const std::string &n, libdap::Type type) : libdap::D4Enum(n, type), DmrppCommon() { }
    DmrppD4Enum(const std::string &n, const std::string &d, libdap::Type type) : libdap::D4Enum(n, d, type), DmrppCommon() { }

    DmrppD4Enum(const DmrppD4Enum &) = default;

    virtual ~DmrppD4Enum() = default;

    DmrppD4Enum &operator=(const DmrppD4Enum &rhs);

    virtual libdap::BaseType *ptr_duplicate() {
        return new DmrppD4Enum(*this);
    }

    bool read() override;
    void set_send_p(bool state) override;

    virtual void print_dap4(libdap::XMLWriter &writer, bool constrained = false)
    {
        DmrppCommon::print_dmrpp(writer, constrained);
    }

    virtual void dump(ostream & strm) const;
};

} // namespace dmrpp

#endif // _dmrpp_d4enum_h
