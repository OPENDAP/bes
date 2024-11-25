
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

#ifndef _dmrpp_byte_h
#define _dmrpp_byte_h 1

#include <string>

#include <libdap/Byte.h>
#include "DmrppCommon.h"

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

class DmrppByte: public libdap::Byte, public DmrppCommon {

public:
    DmrppByte(const std::string &n) : Byte(n), DmrppCommon() { }
    DmrppByte(const std::string &n, const std::string &d) : Byte(n, d), DmrppCommon() { }
    DmrppByte(const std::string &n, shared_ptr<DMZ> dmz) : Byte(n), DmrppCommon(std::move(dmz)) { }
    DmrppByte(const std::string &n, const std::string &d, shared_ptr<DMZ> dmz) : Byte(n, d), DmrppCommon(std::move(dmz)) { }
    DmrppByte(const DmrppByte &) = default;

    ~DmrppByte() override = default;

    DmrppByte &operator=(const DmrppByte &rhs);

    libdap::BaseType *ptr_duplicate() override {
        return new DmrppByte(*this);
    }

    bool read() override;
    void set_send_p(bool state) override;

    void print_dap4(libdap::XMLWriter &writer, bool constrained = false) override
    {
        DmrppCommon::print_dmrpp(writer, constrained);
    }

    void dump(ostream & strm) const override;
};

} // namespace dmrpp
#endif // _dmrpp_byte_h
