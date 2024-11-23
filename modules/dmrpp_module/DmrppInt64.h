
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

#ifndef _dmrpp_int64_h
#define _dmrpp_int64_h 1

#include <string>

#include <libdap/Int64.h>
#include "DmrppCommon.h"

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

class DmrppInt64: public libdap::Int64, public DmrppCommon {

public:
    DmrppInt64(const std::string &n) : libdap::Int64(n), DmrppCommon() { }
    DmrppInt64(const std::string &n, const std::string &d) : libdap::Int64(n, d), DmrppCommon() { }
    DmrppInt64(const std::string &n, std::shared_ptr<DMZ> dmz) : libdap::Int64(n), DmrppCommon(std::move(dmz)) { }
    DmrppInt64(const std::string &n, const std::string &d, std::shared_ptr<DMZ> dmz) : libdap::Int64(n, d), DmrppCommon(std::move(dmz)) { }
    DmrppInt64(const DmrppInt64 &) = default;

    ~DmrppInt64() override = default;

    DmrppInt64 &operator=(const DmrppInt64 &rhs);

    virtual libdap::BaseType *ptr_duplicate() {
        return new DmrppInt64(*this);
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

#endif // _dmrpp_int64_h
