
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

#ifndef _dmrpp_d4group_h
#define _dmrpp_d4group_h 1

#include <string>

#include <libdap/D4Group.h>
#include "DmrppCommon.h"

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

class DmrppD4Group: public libdap::D4Group, public DmrppCommon {

public:
    DmrppD4Group(const std::string &n) : D4Group(n), DmrppCommon() { }
    DmrppD4Group(const std::string &n, const std::string &d) : D4Group(n, d), DmrppCommon() { }
    DmrppD4Group(const std::string &n, std::shared_ptr<DMZ> dmz) : D4Group(n), DmrppCommon(dmz) { }
    DmrppD4Group(const std::string &n, const std::string &d, std::shared_ptr<DMZ> dmz) : D4Group(n, d), DmrppCommon(dmz) { }
    DmrppD4Group(const DmrppD4Group &) = default;

    virtual ~DmrppD4Group() = default;

    DmrppD4Group &operator=(const DmrppD4Group &rhs);

    libdap::BaseType *ptr_duplicate() override {
        return new DmrppD4Group(*this);
    }

#if 1
    void set_send_p(bool state) override;
#endif


    virtual void dump(ostream & strm) const;
};

} // namespace dmrpp

#endif // _dmrpp_d4group_h
