
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

#include <Int64.h>
#include "DmrppCommon.h"

class DmrppInt64: public libdap::Int64, public DmrppCommon {
    void _duplicate(const DmrppInt64 &ts);

public:
    DmrppInt64(const std::string &n);
    DmrppInt64(const std::string &n, const std::string &d);
    DmrppInt64(const DmrppInt64 &rhs);

    virtual ~DmrppInt64() {}

    DmrppInt64 &operator=(const DmrppInt64 &rhs);

    virtual libdap::BaseType *ptr_duplicate();

    virtual bool read();

    virtual void dump(ostream & strm) const;
};

#endif // _dmrpp_int64_h
