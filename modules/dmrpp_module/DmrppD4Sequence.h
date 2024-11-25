
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

#ifndef _dmrpp_d4sequence_h
#define _dmrpp_d4sequence_h 1

#include <string>

#include <libdap/D4Sequence.h>
#include "DmrppCommon.h"

namespace dmrpp {

class DmrppD4Sequence: public libdap::D4Sequence, public DmrppCommon {

public:
    DmrppD4Sequence(const std::string &n) : D4Sequence(n), DmrppCommon() { }
    DmrppD4Sequence(const std::string &n, const std::string &d) : D4Sequence(n, d), DmrppCommon() { }
    DmrppD4Sequence(const std::string &n, std::shared_ptr<DMZ> dmz) : D4Sequence(n), DmrppCommon(std::move(dmz)) { }
    DmrppD4Sequence(const std::string &n, const std::string &d, std::shared_ptr<DMZ> dmz) : D4Sequence(n, d), DmrppCommon(std::move(dmz)) { }
    DmrppD4Sequence(const DmrppD4Sequence &) = default;

    ~DmrppD4Sequence() override = default;

    DmrppD4Sequence &operator=(const DmrppD4Sequence &rhs);

    libdap::BaseType *ptr_duplicate() override {
        return new DmrppD4Sequence(*this);
    }

    bool read() override;

    void dump(ostream & strm) const override;
};

} // namespace dmrpp

#endif // _dmrpp_d4sequence_h
