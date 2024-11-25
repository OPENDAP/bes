
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

#ifndef _dmrpp_str_h
#define _dmrpp_str_h 1

#include <string>

#include <libdap/Str.h>
#include "DmrppCommon.h"

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

class DmrppStr: public libdap::Str, public DmrppCommon {

public:
    explicit DmrppStr(const std::string &n) : libdap::Str(n), DmrppCommon() { }
    DmrppStr(const std::string &n, const std::string &d) : libdap::Str(n, d), DmrppCommon() { }
    DmrppStr(const std::string &n, std::shared_ptr<DMZ> dmz) : libdap::Str(n), DmrppCommon(std::move(dmz)) { }
    DmrppStr(const std::string &n, const std::string &d, std::shared_ptr<DMZ> dmz) : libdap::Str(n, d), DmrppCommon(std::move(dmz)) { }
    DmrppStr(const DmrppStr &) = default;

    ~DmrppStr() override = default;

    DmrppStr &operator=(const DmrppStr &rhs);

    libdap::BaseType *ptr_duplicate() override {
        return new DmrppStr(*this);
    }

    bool read() override;
    void set_send_p(bool state) override;

    void print_dap4(libdap::XMLWriter &writer, bool constrained = false) override {
        DmrppCommon::print_dmrpp(writer, constrained);
    }

    void dump(ostream & strm) const override;
};

} // namespace dmrpp

#endif // _dmrpp_str_h
