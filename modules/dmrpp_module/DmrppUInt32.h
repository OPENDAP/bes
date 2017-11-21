
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

#ifndef _dmrpp_uint32_h
#define _dmrpp_uint32_h 1

#include <string>

#include <UInt32.h>
#include "DmrppCommon.h"

namespace dmrpp {

class DmrppUInt32: public libdap::UInt32, public DmrppCommon {
    void _duplicate(const DmrppUInt32 &ts);

public:
    DmrppUInt32(const std::string &n);
    DmrppUInt32(const std::string &n, const std::string &d);
    DmrppUInt32(const DmrppUInt32 &rhs);

    virtual ~DmrppUInt32() {}

    DmrppUInt32 &operator=(const DmrppUInt32 &rhs);

    virtual libdap::BaseType *ptr_duplicate();

    virtual bool read();

    virtual void dump(ostream & strm) const;
};

} // namespace dmrpp

#endif // _dmrpp_uint32_h
